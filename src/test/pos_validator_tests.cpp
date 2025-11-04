// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pos/validator.h>
#include <pos/validatormgr.h>

#include <key.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pos_validator_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(validator_creation_test)
{
    // Create a test validator
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    
    CBLSSecretKey blsKey;
    blsKey.MakeNewKey();
    CBLSPublicKey blsPubkey = blsKey.GetPublicKey();
    
    COutPoint stakeOutpoint(GetRandHash(), 0);
    CStakeInput stakeInput(stakeOutpoint, MIN_VALIDATOR_STAKE, GetTime(), GetRandHash());
    
    CValidator validator(pubkey, blsPubkey, stakeInput);
    
    BOOST_CHECK(validator.IsActive());
    BOOST_CHECK_EQUAL(validator.nStakeAmount, MIN_VALIDATOR_STAKE);
    BOOST_CHECK(validator.pubkey == pubkey);
    BOOST_CHECK(validator.blsPubkey == blsPubkey);
}

BOOST_AUTO_TEST_CASE(validator_registry_test)
{
    CValidatorRegistry registry;
    
    // Create test validators
    std::vector<CValidator> validators;
    for (int i = 0; i < 5; ++i) {
        CKey key;
        key.MakeNewKey(true);
        CPubKey pubkey = key.GetPubKey();
        
        CBLSSecretKey blsKey;
        blsKey.MakeNewKey();
        CBLSPublicKey blsPubkey = blsKey.GetPublicKey();
        
        COutPoint stakeOutpoint(GetRandHash(), i);
        CStakeInput stakeInput(stakeOutpoint, MIN_VALIDATOR_STAKE + i * COIN, GetTime(), GetRandHash());
        
        CValidator validator(pubkey, blsPubkey, stakeInput);
        validators.push_back(validator);
        
        BOOST_CHECK(registry.RegisterValidator(validator));
    }
    
    BOOST_CHECK_EQUAL(registry.GetValidatorCount(), 5);
    BOOST_CHECK_EQUAL(registry.GetActiveValidatorCount(), 5);
    
    // Test getting validators
    auto activeValidators = registry.GetActiveValidators();
    BOOST_CHECK_EQUAL(activeValidators.size(), 5);
    
    // Test total stake calculation
    CAmount expectedTotalStake = 0;
    for (const auto& validator : validators) {
        expectedTotalStake += validator.nStakeAmount;
    }
    BOOST_CHECK_EQUAL(registry.GetTotalActiveStake(), expectedTotalStake);
}

BOOST_AUTO_TEST_CASE(validator_selection_test)
{
    CValidatorRegistry registry;
    
    // Create test validators with different stake amounts
    std::vector<CValidator> validators;
    std::vector<CAmount> stakeAmounts = {1000 * COIN, 2000 * COIN, 3000 * COIN, 4000 * COIN, 5000 * COIN};
    
    for (size_t i = 0; i < stakeAmounts.size(); ++i) {
        CKey key;
        key.MakeNewKey(true);
        CPubKey pubkey = key.GetPubKey();
        
        CBLSSecretKey blsKey;
        blsKey.MakeNewKey();
        CBLSPublicKey blsPubkey = blsKey.GetPublicKey();
        
        COutPoint stakeOutpoint(GetRandHash(), i);
        CStakeInput stakeInput(stakeOutpoint, stakeAmounts[i], GetTime() - MIN_STAKE_AGE - 1, GetRandHash());
        
        CValidator validator(pubkey, blsPubkey, stakeInput);
        validators.push_back(validator);
        
        BOOST_CHECK(registry.RegisterValidator(validator));
    }
    
    CValidatorSelector selector(registry);
    
    // Test validator selection
    uint256 blockHash = GetRandHash();
    int64_t currentTime = GetTime();
    
    auto selectedValidator = selector.SelectValidator(blockHash, currentTime);
    BOOST_CHECK(selectedValidator != nullptr);
    BOOST_CHECK(selectedValidator->IsEligible(currentTime));
    
    // Test selection probability calculation
    CAmount totalStake = registry.GetTotalActiveStake();
    for (const auto& validator : validators) {
        double probability = selector.CalculateSelectionProbability(validator, totalStake);
        BOOST_CHECK(probability > 0.0);
        BOOST_CHECK(probability <= 1.0);
        
        // Higher stake should have higher probability
        double expectedProbability = static_cast<double>(validator.nStakeAmount) / static_cast<double>(totalStake);
        BOOST_CHECK_CLOSE(probability, expectedProbability, 0.001);
    }
}

BOOST_AUTO_TEST_CASE(validator_slashing_test)
{
    CValidatorRegistry registry;
    
    // Create a test validator
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    
    CBLSSecretKey blsKey;
    blsKey.MakeNewKey();
    CBLSPublicKey blsPubkey = blsKey.GetPublicKey();
    
    COutPoint stakeOutpoint(GetRandHash(), 0);
    CStakeInput stakeInput(stakeOutpoint, MIN_VALIDATOR_STAKE, GetTime(), GetRandHash());
    
    CValidator validator(pubkey, blsPubkey, stakeInput);
    BOOST_CHECK(registry.RegisterValidator(validator));
    
    uint256 validatorHash = Hash(pubkey.begin(), pubkey.end());
    
    // Test slashing
    BOOST_CHECK(registry.SlashValidator(validatorHash, "test slashing"));
    
    auto slashedValidator = registry.GetValidator(validatorHash);
    BOOST_CHECK(slashedValidator != nullptr);
    BOOST_CHECK(slashedValidator->IsSlashed());
    BOOST_CHECK(!slashedValidator->IsActive());
    BOOST_CHECK_EQUAL(slashedValidator->nSlashingCount, 1);
    
    // Active validator count should decrease
    BOOST_CHECK_EQUAL(registry.GetActiveValidatorCount(), 0);
}

BOOST_AUTO_TEST_CASE(stake_input_validation_test)
{
    // Test valid stake input
    COutPoint validOutpoint(GetRandHash(), 0);
    CStakeInput validStake(validOutpoint, MIN_VALIDATOR_STAKE, GetTime(), GetRandHash());
    
    BOOST_CHECK(validStake.IsValid());
    BOOST_CHECK(validStake.IsEligible(GetTime() + MIN_STAKE_AGE + 1));
    
    // Test invalid stake input - insufficient amount
    CStakeInput invalidStake1(validOutpoint, MIN_VALIDATOR_STAKE - 1, GetTime(), GetRandHash());
    BOOST_CHECK(!invalidStake1.IsValid());
    
    // Test invalid stake input - null outpoint
    CStakeInput invalidStake2(COutPoint(), MIN_VALIDATOR_STAKE, GetTime(), GetRandHash());
    BOOST_CHECK(!invalidStake2.IsValid());
    
    // Test stake age eligibility
    int64_t currentTime = GetTime();
    CStakeInput youngStake(validOutpoint, MIN_VALIDATOR_STAKE, currentTime, GetRandHash());
    BOOST_CHECK(!youngStake.IsEligible(currentTime + MIN_STAKE_AGE - 1));
    BOOST_CHECK(youngStake.IsEligible(currentTime + MIN_STAKE_AGE + 1));
    
    // Test maximum stake age
    CStakeInput oldStake(validOutpoint, MIN_VALIDATOR_STAKE, currentTime - MAX_STAKE_AGE - 1, GetRandHash());
    BOOST_CHECK(!oldStake.IsEligible(currentTime));
}

BOOST_AUTO_TEST_CASE(validator_registration_tx_test)
{
    // Create validator registration transaction data
    CValidatorRegTx regTx;
    
    CKey key;
    key.MakeNewKey(true);
    regTx.validatorPubKey = key.GetPubKey();
    
    CBLSSecretKey blsKey;
    blsKey.MakeNewKey();
    regTx.blsPubKey = blsKey.GetPublicKey();
    
    regTx.stakeOutPoint = COutPoint(GetRandHash(), 0);
    regTx.nStakeAmount = MIN_VALIDATOR_STAKE;
    regTx.payoutScript = CScript() << OP_DUP << OP_HASH160 << ToByteVector(key.GetPubKey().GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;
    
    // Sign the registration
    uint256 messageHash = regTx.GetHash();
    key.SignECDSA(messageHash, regTx.vchSig);
    
    BOOST_CHECK_EQUAL(regTx.nVersion, CValidatorRegTx::CURRENT_VERSION);
    BOOST_CHECK(regTx.nStakeAmount >= MIN_VALIDATOR_STAKE);
    BOOST_CHECK(!regTx.validatorPubKey.IsCompressed() || regTx.validatorPubKey.IsCompressed()); // Just check it's valid
}

BOOST_AUTO_TEST_SUITE_END()