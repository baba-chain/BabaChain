// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pos.h>

#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <key.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <util/time.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pos_consensus_tests, TestingSetup)

// Helper function to create a test stake input
CStakeInput CreateTestStakeInput(CAmount amount = 1000 * COIN, int64_t age = 8 * 60 * 60 + 1)
{
    COutPoint outpoint(GetRandHash(), 0);
    int64_t stakeTime = GetTime() - age;
    uint256 blockHash = GetRandHash();
    return CStakeInput(outpoint, amount, stakeTime, blockHash);
}

// Helper function to create a test block index
std::unique_ptr<CBlockIndex> CreateTestBlockIndex(int64_t blockTime = GetTime())
{
    auto pindex = std::make_unique<CBlockIndex>();
    pindex->nTime = blockTime;
    pindex->nBits = 0x1d00ffff; // Standard difficulty
    pindex->nHeight = 100;
    return pindex;
}

BOOST_AUTO_TEST_CASE(stake_input_validation_test)
{
    CPoSValidator validator;
    const Consensus::Params& params = Params().GetConsensus();
    
    // Test valid stake input
    CStakeInput validStake = CreateTestStakeInput();
    BOOST_CHECK(!validStake.IsNull());
    BOOST_CHECK(validStake.nValue >= params.nMinStakeAmount);
    
    // Test null stake input
    CStakeInput nullStake;
    BOOST_CHECK(nullStake.IsNull());
    
    // Test stake with insufficient amount
    CStakeInput insufficientStake = CreateTestStakeInput(params.nMinStakeAmount - 1);
    BOOST_CHECK(!validator.CheckStakeAmount(insufficientStake, params));
    
    // Test stake with valid amount
    CStakeInput validAmountStake = CreateTestStakeInput(params.nMinStakeAmount);
    BOOST_CHECK(validator.CheckStakeAmount(validAmountStake, params));
}

BOOST_AUTO_TEST_CASE(stake_age_validation_test)
{
    CPoSValidator validator;
    const Consensus::Params& params = Params().GetConsensus();
    auto pindex = CreateTestBlockIndex();
    
    // Test stake that is too young
    CStakeInput youngStake = CreateTestStakeInput(1000 * COIN, params.nStakeMinAge - 1);
    BOOST_CHECK(!validator.CheckStakeAge(youngStake, pindex.get(), params));
    
    // Test stake with minimum age
    CStakeInput minAgeStake = CreateTestStakeInput(1000 * COIN, params.nStakeMinAge);
    BOOST_CHECK(validator.CheckStakeAge(minAgeStake, pindex.get(), params));
    
    // Test stake with valid age
    CStakeInput validAgeStake = CreateTestStakeInput(1000 * COIN, params.nStakeMinAge + 3600);
    BOOST_CHECK(validator.CheckStakeAge(validAgeStake, pindex.get(), params));
    
    // Test stake that is too old
    CStakeInput oldStake = CreateTestStakeInput(1000 * COIN, params.nStakeMaxAge + 1);
    BOOST_CHECK(!validator.CheckStakeAge(oldStake, pindex.get(), params));
}

BOOST_AUTO_TEST_CASE(stake_reward_calculation_test)
{
    CPoSValidator validator;
    const Consensus::Params& params = Params().GetConsensus();
    
    // Test reward calculation for different stake amounts and durations
    CAmount stakeAmount = 1000 * COIN;
    
    // Test 1 day staking
    int64_t oneDayDuration = 24 * 60 * 60;
    CAmount oneDayReward = validator.CalculateStakeReward(stakeAmount, oneDayDuration, params);
    BOOST_CHECK(oneDayReward > 0);
    
    // Test 1 year staking (should be approximately 5% of stake amount)
    int64_t oneYearDuration = 365 * 24 * 60 * 60;
    CAmount oneYearReward = validator.CalculateStakeReward(stakeAmount, oneYearDuration, params);
    BOOST_CHECK(oneYearReward > oneDayReward);
    
    // Reward should be roughly 5% for one year (allowing for some variance due to integer arithmetic)
    CAmount expectedYearlyReward = stakeAmount * 5 / 100; // 5%
    BOOST_CHECK(oneYearReward >= expectedYearlyReward * 90 / 100); // Within 10% tolerance
    BOOST_CHECK(oneYearReward <= expectedYearlyReward * 110 / 100);
    
    // Test minimum reward enforcement
    CAmount smallAmount = 1 * COIN;
    int64_t shortDuration = 60; // 1 minute
    CAmount minReward = validator.CalculateStakeReward(smallAmount, shortDuration, params);
    BOOST_CHECK(minReward >= COIN / 1000); // Minimum 0.001 coins
    
    // Test maximum reward enforcement (should not exceed 10% of stake)
    CAmount largeAmount = 10000 * COIN;
    CAmount maxReward = validator.CalculateStakeReward(largeAmount, oneYearDuration, params);
    BOOST_CHECK(maxReward <= largeAmount / 10); // Maximum 10% of stake
}

BOOST_AUTO_TEST_CASE(validator_selection_algorithm_test)
{
    CPoSValidator validator;
    
    // Create test validators with different stake amounts
    std::vector<CValidator> validators;
    
    // Validator 1: 1000 coins
    CKey key1;
    key1.MakeNewKey(true);
    CValidator validator1(key1.GetPubKey(), 1000 * COIN, GetTime(), true);
    validators.push_back(validator1);
    
    // Validator 2: 2000 coins (should have higher selection probability)
    CKey key2;
    key2.MakeNewKey(true);
    CValidator validator2(key2.GetPubKey(), 2000 * COIN, GetTime(), true);
    validators.push_back(validator2);
    
    // Validator 3: 3000 coins (should have highest selection probability)
    CKey key3;
    key3.MakeNewKey(true);
    CValidator validator3(key3.GetPubKey(), 3000 * COIN, GetTime(), true);
    validators.push_back(validator3);
    
    // Test validator selection multiple times to check distribution
    std::map<std::string, int> selectionCounts;
    const int numSelections = 1000;
    
    for (int i = 0; i < numSelections; ++i) {
        uint256 blockHash = GetRandHash();
        CValidator selectedValidator;
        
        BOOST_CHECK(validator.SelectNextValidator(validators, blockHash, selectedValidator));
        
        std::string validatorKey = HexStr(selectedValidator.pubkey);
        selectionCounts[validatorKey]++;
    }
    
    // Verify that all validators were selected at least once
    BOOST_CHECK_EQUAL(selectionCounts.size(), 3);
    
    // Verify that higher stake validators are selected more frequently
    std::string key1Hex = HexStr(validator1.pubkey);
    std::string key2Hex = HexStr(validator2.pubkey);
    std::string key3Hex = HexStr(validator3.pubkey);
    
    // Validator 3 (3000 coins) should be selected most frequently
    BOOST_CHECK(selectionCounts[key3Hex] > selectionCounts[key2Hex]);
    BOOST_CHECK(selectionCounts[key2Hex] > selectionCounts[key1Hex]);
}

BOOST_AUTO_TEST_CASE(stake_modifier_calculation_test)
{
    CPoSValidator validator;
    auto pindex = CreateTestBlockIndex();
    
    // Create test stake inputs
    CStakeInput stake1 = CreateTestStakeInput();
    CStakeInput stake2 = CreateTestStakeInput();
    
    // Calculate stake modifiers
    uint256 modifier1 = validator.CalculateStakeModifier(pindex.get(), stake1);
    uint256 modifier2 = validator.CalculateStakeModifier(pindex.get(), stake2);
    
    // Modifiers should be different for different stake inputs
    BOOST_CHECK(modifier1 != modifier2);
    BOOST_CHECK(!modifier1.IsNull());
    BOOST_CHECK(!modifier2.IsNull());
    
    // Same stake input should produce same modifier
    uint256 modifier1_repeat = validator.CalculateStakeModifier(pindex.get(), stake1);
    BOOST_CHECK(modifier1 == modifier1_repeat);
}

BOOST_AUTO_TEST_CASE(pos_block_validation_test)
{
    CPoSValidator validator;
    const Consensus::Params& params = Params().GetConsensus();
    auto pindexPrev = CreateTestBlockIndex(GetTime() - 600); // Previous block 10 minutes ago
    
    // Create a test block
    CBlock block;
    block.nVersion = 1;
    block.hashPrevBlock = pindexPrev->GetBlockHash();
    block.nTime = GetTime();
    block.nBits = 0x1d00ffff;
    block.nNonce = 0;
    
    // Create coinbase transaction
    CMutableTransaction coinbase;
    coinbase.nVersion = 1;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vin[0].scriptSig = CScript() << 101 << OP_0; // Height 101
    coinbase.vout.resize(1);
    coinbase.vout[0].nValue = 0;
    coinbase.vout[0].scriptPubKey = CScript();
    
    // Create coinstake transaction
    CMutableTransaction coinstake;
    coinstake.nVersion = 1;
    coinstake.nTime = block.nTime;
    coinstake.vin.resize(1);
    coinstake.vin[0].prevout = COutPoint(GetRandHash(), 0);
    coinstake.vin[0].nSequence = 0xffffffff;
    coinstake.vout.resize(2);
    coinstake.vout[0].nValue = 1000 * COIN; // Return stake
    coinstake.vout[0].scriptPubKey = CScript();
    coinstake.vout[1].nValue = 50 * COIN; // Reward
    coinstake.vout[1].scriptPubKey = CScript();
    
    // Add transactions to block
    block.vtx.clear();
    block.vtx.push_back(MakeTransactionRef(std::move(coinbase)));
    block.vtx.push_back(MakeTransactionRef(std::move(coinstake)));
    
    // Calculate merkle root
    bool mutated;
    block.hashMerkleRoot = BlockMerkleRoot(block, &mutated);
    
    // Create a mock coins view
    CCoinsView coinsDummy;
    CCoinsViewCache view(&coinsDummy);
    
    // Test block validation
    BlockValidationState state;
    BOOST_CHECK(validator.ValidatePoSBlock(block, pindexPrev.get(), state, params, view));
    
    // Test block with missing coinstake (should fail)
    CBlock invalidBlock = block;
    invalidBlock.vtx.resize(1); // Remove coinstake transaction
    
    BlockValidationState invalidState;
    BOOST_CHECK(!validator.ValidatePoSBlock(invalidBlock, pindexPrev.get(), invalidState, params, view));
    BOOST_CHECK_EQUAL(invalidState.GetRejectReason(), "bad-pos-missing-coinstake");
}

BOOST_AUTO_TEST_CASE(pos_block_creation_test)
{
    const Consensus::Params& params = Params().GetConsensus();
    auto pindexPrev = CreateTestBlockIndex(GetTime() - 600);
    
    // Create test stake input
    CStakeInput stakeInput = CreateTestStakeInput();
    
    // Create staking key
    CKey stakingKey;
    stakingKey.MakeNewKey(true);
    
    // Create PoS block
    CBlock block;
    BOOST_CHECK(CreatePoSBlock(block, pindexPrev.get(), stakeInput, stakingKey, params));
    
    // Verify block structure
    BOOST_CHECK_EQUAL(block.hashPrevBlock, pindexPrev->GetBlockHash());
    BOOST_CHECK(block.nTime > pindexPrev->GetMedianTimePast());
    BOOST_CHECK_EQUAL(block.nNonce, 0); // PoS doesn't use nonce
    BOOST_CHECK_EQUAL(block.vtx.size(), 2); // Coinbase + coinstake
    
    // Verify coinbase transaction
    const CTransaction& coinbase = *block.vtx[0];
    BOOST_CHECK(coinbase.IsCoinBase());
    BOOST_CHECK_EQUAL(coinbase.vout[0].nValue, 0); // No block reward in coinbase for PoS
    
    // Verify coinstake transaction
    const CTransaction& coinstake = *block.vtx[1];
    BOOST_CHECK(!coinstake.IsCoinBase());
    BOOST_CHECK_EQUAL(coinstake.vin.size(), 1);
    BOOST_CHECK_EQUAL(coinstake.vin[0].prevout, stakeInput.prevout);
    BOOST_CHECK_EQUAL(coinstake.vout.size(), 2);
    BOOST_CHECK_EQUAL(coinstake.vout[0].nValue, stakeInput.nValue); // Return stake
    BOOST_CHECK(coinstake.vout[1].nValue > 0); // Stake reward
    
    // Test block signing
    BOOST_CHECK(SignPoSBlock(block, stakingKey));
}

BOOST_AUTO_TEST_CASE(stake_validation_comprehensive_test)
{
    CPoSValidator validator;
    const Consensus::Params& params = Params().GetConsensus();
    auto pindexPrev = CreateTestBlockIndex();
    
    // Create a test block with coinstake
    CBlock block;
    block.nTime = GetTime();
    
    // Create mock coins view with a valid coin
    CCoinsView coinsDummy;
    CCoinsViewCache view(&coinsDummy);
    
    // Add a coin to the view
    COutPoint outpoint(GetRandHash(), 0);
    CTxOut txout(1000 * COIN, CScript());
    Coin coin(txout, 1, false);
    view.AddCoin(outpoint, std::move(coin), false);
    
    // Create stake input using the coin
    CStakeInput stakeInput(outpoint, 1000 * COIN, GetTime() - params.nStakeMinAge - 1, GetRandHash());
    
    // Test valid stake validation
    PoSValidationResult result = validator.ValidateStake(pindexPrev.get(), block, stakeInput, view);
    BOOST_CHECK_EQUAL(result, PoSValidationResult::VALID);
    
    // Test invalid stake amount
    CStakeInput invalidAmountStake(outpoint, params.nMinStakeAmount - 1, GetTime() - params.nStakeMinAge - 1, GetRandHash());
    result = validator.ValidateStake(pindexPrev.get(), block, invalidAmountStake, view);
    BOOST_CHECK_EQUAL(result, PoSValidationResult::INVALID_STAKE_AMOUNT);
    
    // Test invalid stake age (too young)
    CStakeInput youngStake(outpoint, 1000 * COIN, GetTime() - params.nStakeMinAge + 1, GetRandHash());
    result = validator.ValidateStake(pindexPrev.get(), block, youngStake, view);
    BOOST_CHECK_EQUAL(result, PoSValidationResult::INVALID_STAKE_AGE);
    
    // Test null stake input
    CStakeInput nullStake;
    result = validator.ValidateStake(pindexPrev.get(), block, nullStake, view);
    BOOST_CHECK_EQUAL(result, PoSValidationResult::INVALID_STAKE_AMOUNT);
}

BOOST_AUTO_TEST_CASE(pos_target_validation_test)
{
    // Test PoS target validation function
    const Consensus::Params& params = Params().GetConsensus();
    
    // Test valid PoS target
    uint32_t validTarget = 0x1d00ffff; // Standard difficulty
    BOOST_CHECK(CheckProofOfStakeTarget(validTarget, params));
    
    // Test invalid target (too easy)
    uint32_t tooEasyTarget = 0x1effffff;
    BOOST_CHECK(!CheckProofOfStakeTarget(tooEasyTarget, params));
    
    // Test invalid target (too hard)
    uint32_t tooHardTarget = 0x1c000000;
    BOOST_CHECK(!CheckProofOfStakeTarget(tooHardTarget, params));
}

BOOST_AUTO_TEST_SUITE_END()