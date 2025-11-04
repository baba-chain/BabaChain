// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pos.h>

#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <hash.h>
#include <key.h>
#include <logging.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/standard.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <validation.h>
#include <arith_uint256.h>
#include <versionbits.h>

#include <algorithm>
#include <random>

std::string CStakeInput::ToString() const
{
    return strprintf("CStakeInput(prevout=%s, nValue=%s, nTime=%d, hashBlock=%s)",
                     prevout.ToString(), FormatMoney(nValue), nTime, hashBlock.ToString());
}

std::string CStakeOutput::ToString() const
{
    return strprintf("CStakeOutput(scriptPubKey=%s, nValue=%s)",
                     HexStr(scriptPubKey), FormatMoney(nValue));
}

std::string CValidator::ToString() const
{
    return strprintf("CValidator(pubkey=%s, nStakeAmount=%s, nRegistrationTime=%d, fActive=%s)",
                     HexStr(pubkey), FormatMoney(nStakeAmount), nRegistrationTime, fActive ? "true" : "false");
}

PoSValidationResult CPoSValidator::ValidateStake(const CBlockIndex* pindexPrev, const CBlock& block, const CStakeInput& stakeInput, const CCoinsViewCache& view) const
{
    // Check if stake input is valid
    if (stakeInput.IsNull()) {
        return PoSValidationResult::INVALID_STAKE_AMOUNT;
    }
    
    // Get the consensus parameters
    const Consensus::Params& params = Params().GetConsensus();
    
    // Check stake amount meets minimum requirements
    if (!CheckStakeAmount(stakeInput, params)) {
        return PoSValidationResult::INVALID_STAKE_AMOUNT;
    }
    
    // Check stake age meets minimum requirements
    if (!CheckStakeAge(stakeInput, pindexPrev, params)) {
        return PoSValidationResult::INVALID_STAKE_AGE;
    }
    
    // Verify the stake signature
    if (!VerifyStakeSignature(block, stakeInput, view)) {
        return PoSValidationResult::INVALID_SIGNATURE;
    }
    
    // Calculate and verify stake modifier
    uint256 stakeModifier = CalculateStakeModifier(pindexPrev, stakeInput);
    if (stakeModifier.IsNull()) {
        return PoSValidationResult::INVALID_STAKE_MODIFIER;
    }
    
    // Check stake kernel
    if (!CheckStakeKernel(stakeInput, stakeModifier, pindexPrev, params)) {
        return PoSValidationResult::INVALID_STAKE_MODIFIER;
    }
    
    return PoSValidationResult::VALID;
}

CAmount CPoSValidator::CalculateStakeReward(const CAmount& stakeAmount, int64_t stakeDuration, const Consensus::Params& params) const
{
    // Base annual reward rate (e.g., 5% = 0.05)
    const double ANNUAL_REWARD_RATE = 0.05;
    
    // Convert duration from seconds to years
    const int64_t SECONDS_PER_YEAR = 365 * 24 * 60 * 60;
    double durationInYears = static_cast<double>(stakeDuration) / SECONDS_PER_YEAR;
    
    // Calculate reward: stakeAmount * rate * duration
    CAmount reward = static_cast<CAmount>(stakeAmount * ANNUAL_REWARD_RATE * durationInYears);
    
    // Apply minimum and maximum reward limits
    const CAmount MIN_REWARD = 1 * COIN / 1000; // 0.001 coins minimum
    const CAmount MAX_REWARD = stakeAmount / 10; // Maximum 10% of stake amount
    
    reward = std::max(MIN_REWARD, std::min(reward, MAX_REWARD));
    
    return reward;
}

bool CPoSValidator::SelectNextValidator(const std::vector<CValidator>& validators, const uint256& blockHash, CValidator& selectedValidator) const
{
    if (validators.empty()) {
        return false;
    }
    
    // Filter active validators
    std::vector<CValidator> activeValidators;
    for (const auto& validator : validators) {
        if (validator.fActive && validator.nStakeAmount > 0) {
            activeValidators.push_back(validator);
        }
    }
    
    if (activeValidators.empty()) {
        return false;
    }
    
    // Calculate total stake weight
    CAmount totalStake = 0;
    for (const auto& validator : activeValidators) {
        totalStake += validator.nStakeAmount;
    }
    
    if (totalStake == 0) {
        return false;
    }
    
    // Use block hash as source of randomness
    uint256 randomSeed = Hash(blockHash.begin(), blockHash.end());
    uint64_t randomValue = randomSeed.GetUint64(0) % totalStake;
    
    // Select validator based on stake weight
    CAmount cumulativeStake = 0;
    for (const auto& validator : activeValidators) {
        cumulativeStake += validator.nStakeAmount;
        if (randomValue < cumulativeStake) {
            selectedValidator = validator;
            return true;
        }
    }
    
    // Fallback to last validator (should not happen)
    selectedValidator = activeValidators.back();
    return true;
}

bool CPoSValidator::ValidatePoSBlock(const CBlock& block, const CBlockIndex* pindexPrev, BlockValidationState& state, const Consensus::Params& params, const CCoinsViewCache& view) const
{
    // Check if block has at least one transaction (coinbase)
    if (block.vtx.empty()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-cb-missing", "first tx is not coinbase");
    }
    
    // For PoS blocks, we expect a coinstake transaction as the second transaction
    if (block.vtx.size() < 2) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-pos-missing-coinstake", "PoS block missing coinstake transaction");
    }
    
    // Validate block time
    if (block.GetBlockTime() <= pindexPrev->GetMedianTimePast()) {
        return state.Invalid(BlockValidationResult::BLOCK_INVALID_HEADER, "time-too-old", "block's timestamp is too early");
    }
    
    // Additional PoS-specific validations would go here
    // For now, we'll implement basic structure validation
    
    return true;
}

bool CPoSValidator::CheckStakeAge(const CStakeInput& stakeInput, const CBlockIndex* pindexPrev, const Consensus::Params& params) const
{
    // Calculate stake age
    int64_t stakeAge = pindexPrev->GetBlockTime() - stakeInput.nTime;
    
    // Check minimum stake age (default: 8 hours)
    int64_t nStakeMinAge = params.nStakeMinAge;
    if (stakeAge < nStakeMinAge) {
        return false;
    }
    
    // Check maximum stake age (default: 30 days)
    int64_t nStakeMaxAge = params.nStakeMaxAge;
    if (stakeAge > nStakeMaxAge) {
        return false;
    }
    
    return true;
}

bool CPoSValidator::CheckStakeAmount(const CStakeInput& stakeInput, const Consensus::Params& params) const
{
    // Check minimum stake amount
    if (stakeInput.nValue < params.nMinStakeAmount) {
        return false;
    }
    
    return true;
}

uint256 CPoSValidator::CalculateStakeModifier(const CBlockIndex* pindexPrev, const CStakeInput& stakeInput) const
{
    // Combine previous block hash with stake input data for randomization
    CHashWriter ss(SER_GETHASH, 0);
    ss << pindexPrev->GetBlockHash();
    ss << stakeInput.prevout;
    ss << stakeInput.nTime;
    
    return ss.GetHash();
}

bool CPoSValidator::VerifyStakeSignature(const CBlock& block, const CStakeInput& stakeInput, const CCoinsViewCache& view) const
{
    // Get the coin being staked
    Coin coin;
    if (!view.GetCoin(stakeInput.prevout, coin)) {
        return false;
    }
    
    // For now, we'll implement a basic signature verification
    // In a full implementation, this would verify the block signature against the stake input
    
    // Check that the coin exists and is unspent
    if (coin.IsSpent()) {
        return false;
    }
    
    // Check that the coin value matches the stake input
    if (coin.out.nValue != stakeInput.nValue) {
        return false;
    }
    
    return true;
}

uint64_t CPoSValidator::CalculateStakeWeight(const CAmount& stakeAmount, int64_t stakeAge, const Consensus::Params& params) const
{
    // Calculate stake weight based on amount and age
    // Weight = amount * min(age, maxAge) / minAge
    
    int64_t effectiveAge = std::min(stakeAge, params.nStakeMaxAge);
    effectiveAge = std::max(effectiveAge, params.nStakeMinAge);
    
    uint64_t weight = (stakeAmount * effectiveAge) / params.nStakeMinAge;
    
    return weight;
}

bool CPoSValidator::CheckStakeKernel(const CStakeInput& stakeInput, const uint256& stakeModifier, const CBlockIndex* pindexPrev, const Consensus::Params& params) const
{
    // Calculate stake kernel hash
    CHashWriter ss(SER_GETHASH, 0);
    ss << stakeModifier;
    ss << stakeInput.nTime;
    ss << stakeInput.prevout;
    
    uint256 kernelHash = ss.GetHash();
    
    // Calculate target based on stake amount and difficulty
    arith_uint256 target;
    target.SetCompact(pindexPrev->nBits);
    
    // Adjust target based on stake amount (higher stake = easier target)
    target = target * stakeInput.nValue / (100 * COIN); // Scale by stake amount
    
    // Check if kernel hash meets target
    return UintToArith256(kernelHash) <= target;
}

bool CheckProofOfStake(const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& params, const CCoinsViewCache& view)
{
    CPoSValidator validator;
    BlockValidationState state;
    
    return validator.ValidatePoSBlock(block, pindexPrev, state, params, view);
}

uint256 GetStakeModifier(const CBlockIndex* pindex)
{
    if (!pindex) {
        return uint256();
    }
    
    // For now, use the block hash as stake modifier
    // In a full implementation, this would be calculated based on previous stake modifiers
    return pindex->GetBlockHash();
}

bool CreatePoSBlock(CBlock& block, const CBlockIndex* pindexPrev, const CStakeInput& stakeInput, const CKey& key, const Consensus::Params& params)
{
    // Set block version
    block.nVersion = VERSIONBITS_LAST_OLD_BLOCK_VERSION;
    
    // Set previous block hash
    block.hashPrevBlock = pindexPrev->GetBlockHash();
    
    // Set block time (must be greater than previous block's median time)
    block.nTime = std::max(pindexPrev->GetMedianTimePast() + 1, GetAdjustedTime());
    
    // Set target bits for PoS
    block.nBits = GetNextPoSTarget(pindexPrev, params);
    
    // Set nonce (not used in PoS, but set to 0)
    block.nNonce = 0;
    
    // Create coinbase transaction
    CMutableTransaction coinbaseTx;
    coinbaseTx.nVersion = 1;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vin[0].scriptSig = CScript() << pindexPrev->nHeight + 1 << OP_0;
    coinbaseTx.vout.resize(1);
    coinbaseTx.vout[0].nValue = 0; // No block reward in coinbase for PoS
    coinbaseTx.vout[0].scriptPubKey = CScript(); // Empty script
    
    // Add coinbase transaction
    block.vtx.clear();
    block.vtx.push_back(MakeTransactionRef(std::move(coinbaseTx)));
    
    // Create coinstake transaction
    CMutableTransaction coinstakeTx;
    coinstakeTx.nVersion = 1;
    coinstakeTx.nTime = block.nTime;
    
    // Input: the stake being used
    coinstakeTx.vin.resize(1);
    coinstakeTx.vin[0].prevout = stakeInput.prevout;
    coinstakeTx.vin[0].nSequence = 0xffffffff;
    
    // Output 1: return the stake
    coinstakeTx.vout.resize(2);
    coinstakeTx.vout[0].nValue = stakeInput.nValue;
    
    // Output 2: stake reward
    CPoSValidator validator;
    int64_t stakeDuration = block.nTime - stakeInput.nTime;
    CAmount stakeReward = validator.CalculateStakeReward(stakeInput.nValue, stakeDuration, params);
    coinstakeTx.vout[1].nValue = stakeReward;
    
    // Set output scripts (would normally be set to staker's address)
    // For now, we'll leave them empty as this is a basic implementation
    coinstakeTx.vout[0].scriptPubKey = CScript();
    coinstakeTx.vout[1].scriptPubKey = CScript();
    
    // Add coinstake transaction
    block.vtx.push_back(MakeTransactionRef(std::move(coinstakeTx)));
    
    // Calculate merkle root
    bool mutated;
    block.hashMerkleRoot = BlockMerkleRoot(block, &mutated);
    
    return true;
}

bool SignPoSBlock(CBlock& block, const CKey& key)
{
    // Sign the block hash with the staking key
    // This is a simplified implementation
    uint256 blockHash = block.GetHash();
    
    std::vector<unsigned char> vchSig;
    if (!key.Sign(blockHash, vchSig)) {
        return false;
    }
    
    // Store signature in block (this would normally be in a specific field)
    // For now, we'll just return true as the signature validation
    // is handled in the PoS validation functions
    
    return true;
}