// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pos.h>
#include <validation.h>
#include <chainparams.h>
#include <consensus/consensus.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <logging.h>
#include <streams.h>
#include <util/time.h>
#include <primitives/block.h>
#include <chain.h>
#include <coins.h>

// Forward declaration to avoid circular dependency
extern const CChainParams& Params();

std::string CStakeTransactionPayload::ToString() const
{
    return strprintf("CStakeTransactionPayload(nStakeAmount=%s, stakePubKey=%s, nLockTime=%d)",
                    FormatMoney(nStakeAmount), HexStr(stakePubKey), nLockTime);
}

std::string CUnstakeTransactionPayload::ToString() const
{
    return strprintf("CUnstakeTransactionPayload(stakeOutpoint=%s, stakePubKey=%s)",
                    stakeOutpoint.ToString(), HexStr(stakePubKey));
}

std::string CValidatorRegistrationPayload::ToString() const
{
    return strprintf("CValidatorRegistrationPayload(validatorPubKey=%s, nStakeAmount=%s, strDescription=%s)",
                    HexStr(validatorPubKey), FormatMoney(nStakeAmount), strDescription);
}

std::string CStakeLock::ToString() const
{
    return strprintf("CStakeLock(outpoint=%s, nLockTime=%d, nAmount=%s, ownerPubKey=%s)",
                    outpoint.ToString(), nLockTime, FormatMoney(nAmount), HexStr(ownerPubKey));
}

/**
 * Calculate staking reward based on stake amount and duration
 */
CAmount CalculateStakeReward(const CAmount& stakeAmount, int64_t stakeDuration, const Consensus::Params& consensusParams)
{
    // Base reward calculation: proportional to stake amount
    CAmount baseReward = consensusParams.nInitialBlockReward;
    
    // Minimum stake requirement check
    if (stakeAmount < consensusParams.nMinStakeAmount) {
        return 0;
    }
    
    // Calculate reward based on stake weight
    // Larger stakes get proportionally larger rewards
    double stakeWeight = static_cast<double>(stakeAmount) / static_cast<double>(consensusParams.nMinStakeAmount);
    
    // Apply stake duration bonus (longer stakes get slight bonus)
    double durationMultiplier = 1.0;
    if (stakeDuration >= consensusParams.nStakeMinAge) {
        // Bonus for stakes held longer than minimum age
        int64_t bonusDays = (stakeDuration - consensusParams.nStakeMinAge) / (24 * 60 * 60);
        durationMultiplier = 1.0 + (bonusDays * 0.001); // 0.1% bonus per day, capped
        if (durationMultiplier > 1.1) durationMultiplier = 1.1; // Max 10% bonus
    }
    
    CAmount reward = static_cast<CAmount>(baseReward * stakeWeight * durationMultiplier);
    
    // Cap individual rewards to prevent excessive concentration
    CAmount maxReward = baseReward * 10; // Max 10x base reward per stake
    if (reward > maxReward) {
        reward = maxReward;
    }
    
    return reward;
}

/**
 * Distribute staking rewards to validators
 */
bool DistributeStakingRewards(const std::vector<CStakeInput>& validators, CAmount totalReward, std::vector<CAmount>& rewards)
{
    if (validators.empty()) {
        return false;
    }
    
    rewards.clear();
    rewards.resize(validators.size());
    
    // Calculate total stake weight
    CAmount totalStake = 0;
    for (const auto& validator : validators) {
        totalStake += validator.nValue;
    }
    
    if (totalStake == 0) {
        return false;
    }
    
    // Distribute rewards proportionally to stake
    CAmount distributedReward = 0;
    for (size_t i = 0; i < validators.size(); ++i) {
        CAmount validatorReward = (totalReward * validators[i].nValue) / totalStake;
        rewards[i] = validatorReward;
        distributedReward += validatorReward;
    }
    
    // Handle rounding by giving remainder to largest staker
    if (distributedReward < totalReward) {
        CAmount remainder = totalReward - distributedReward;
        
        // Find validator with largest stake
        size_t largestStakeIndex = 0;
        for (size_t i = 1; i < validators.size(); ++i) {
            if (validators[i].nValue > validators[largestStakeIndex].nValue) {
                largestStakeIndex = i;
            }
        }
        
        rewards[largestStakeIndex] += remainder;
    }
    
    return true;
}

/**
 * Check if total supply cap is enforced
 */
bool IsSupplyCapEnforced(int nHeight, const Consensus::Params& consensusParams)
{
    // Calculate total supply at this height
    CAmount totalSupply = consensusParams.nPremineAmount; // Start with premine
    
    // Add all staking rewards distributed so far
    if (nHeight > 0) {
        totalSupply += (nHeight - 1) * consensusParams.nInitialBlockReward;
    }
    
    // Check if we're approaching or at the cap
    if (totalSupply >= consensusParams.nMaxSupply) {
        LogPrintf("Supply cap reached at height %d: %s / %s\n", 
                 nHeight, FormatMoney(totalSupply), FormatMoney(consensusParams.nMaxSupply));
        return true;
    }
    
    return false;
}

/**
 * Get remaining supply available for staking rewards
 */
CAmount GetRemainingStakingSupply(int nHeight, const Consensus::Params& consensusParams)
{
    // Calculate total rewards already distributed
    CAmount distributedRewards = 0;
    if (nHeight > 0) {
        distributedRewards = (nHeight - 1) * consensusParams.nInitialBlockReward;
    }
    
    // Return remaining from staking supply pool
    if (distributedRewards >= consensusParams.nStakingSupply) {
        return 0;
    }
    
    return consensusParams.nStakingSupply - distributedRewards;
}

// Global stake lock registry (in production, this would be in a database)
static std::map<COutPoint, CStakeLock> mapStakeLocks;

/**
 * Validate staking transaction
 */
bool ValidateStakingTransaction(const CTransaction& tx, const Consensus::Params& consensusParams)
{
    // Basic validation checks
    if (tx.vin.empty() || tx.vout.empty()) {
        LogPrintf("ValidateStakingTransaction: Transaction has no inputs or outputs\n");
        return false;
    }
    
    // Check transaction type and validate accordingly
    if (tx.nType == TRANSACTION_STAKE) {
        if (tx.vExtraPayload.empty()) {
            LogPrintf("ValidateStakingTransaction: Stake transaction missing payload\n");
            return false;
        }
        
        CStakeTransactionPayload payload;
        try {
            CDataStream ds(tx.vExtraPayload, SER_NETWORK, PROTOCOL_VERSION);
            ds >> payload;
        } catch (const std::exception& e) {
            LogPrintf("ValidateStakingTransaction: Failed to deserialize stake payload: %s\n", e.what());
            return false;
        }
        
        return ValidateStakeTransaction(tx, payload, consensusParams);
    }
    else if (tx.nType == TRANSACTION_UNSTAKE) {
        if (tx.vExtraPayload.empty()) {
            LogPrintf("ValidateStakingTransaction: Unstake transaction missing payload\n");
            return false;
        }
        
        CUnstakeTransactionPayload payload;
        try {
            CDataStream ds(tx.vExtraPayload, SER_NETWORK, PROTOCOL_VERSION);
            ds >> payload;
        } catch (const std::exception& e) {
            LogPrintf("ValidateStakingTransaction: Failed to deserialize unstake payload: %s\n", e.what());
            return false;
        }
        
        return ValidateUnstakeTransaction(tx, payload, consensusParams);
    }
    else if (tx.nType == TRANSACTION_VALIDATOR_REGISTER) {
        if (tx.vExtraPayload.empty()) {
            LogPrintf("ValidateStakingTransaction: Validator registration missing payload\n");
            return false;
        }
        
        CValidatorRegistrationPayload payload;
        try {
            CDataStream ds(tx.vExtraPayload, SER_NETWORK, PROTOCOL_VERSION);
            ds >> payload;
        } catch (const std::exception& e) {
            LogPrintf("ValidateStakingTransaction: Failed to deserialize validator registration payload: %s\n", e.what());
            return false;
        }
        
        return ValidateValidatorRegistration(tx, payload, consensusParams);
    }
    
    return true;
}

/**
 * Validate stake transaction payload
 */
bool ValidateStakeTransaction(const CTransaction& tx, const CStakeTransactionPayload& payload, const Consensus::Params& consensusParams)
{
    // Check minimum stake amount
    if (payload.nStakeAmount < consensusParams.nMinStakeAmount) {
        LogPrintf("ValidateStakeTransaction: Stake amount %s below minimum %s\n", 
                 FormatMoney(payload.nStakeAmount), FormatMoney(consensusParams.nMinStakeAmount));
        return false;
    }
    
    // Check lock time is within acceptable range
    if (payload.nLockTime < consensusParams.nStakeMinAge) {
        LogPrintf("ValidateStakeTransaction: Lock time %d below minimum %d\n", 
                 payload.nLockTime, consensusParams.nStakeMinAge);
        return false;
    }
    
    if (payload.nLockTime > consensusParams.nStakeMaxAge) {
        LogPrintf("ValidateStakeTransaction: Lock time %d above maximum %d\n", 
                 payload.nLockTime, consensusParams.nStakeMaxAge);
        return false;
    }
    
    // Validate public key
    if (!payload.stakePubKey.IsValid()) {
        LogPrintf("ValidateStakeTransaction: Invalid stake public key\n");
        return false;
    }
    
    // Check that transaction output matches stake amount
    CAmount totalOutput = 0;
    for (const auto& vout : tx.vout) {
        totalOutput += vout.nValue;
    }
    
    if (totalOutput != payload.nStakeAmount) {
        LogPrintf("ValidateStakeTransaction: Output amount %s doesn't match stake amount %s\n", 
                 FormatMoney(totalOutput), FormatMoney(payload.nStakeAmount));
        return false;
    }
    
    return true;
}

/**
 * Validate unstake transaction payload
 */
bool ValidateUnstakeTransaction(const CTransaction& tx, const CUnstakeTransactionPayload& payload, const Consensus::Params& consensusParams)
{
    // Check if the referenced stake exists and is locked
    CStakeLock stakeLock;
    if (!GetStakeLock(payload.stakeOutpoint, stakeLock)) {
        LogPrintf("ValidateUnstakeTransaction: No stake lock found for outpoint %s\n", 
                 payload.stakeOutpoint.ToString());
        return false;
    }
    
    // Check if stake lock has expired
    int64_t nCurrentTime = GetTime();
    if (!stakeLock.IsExpired(nCurrentTime)) {
        LogPrintf("ValidateUnstakeTransaction: Stake lock not yet expired (expires at %d, current time %d)\n", 
                 stakeLock.nLockTime, nCurrentTime);
        return false;
    }
    
    // Validate that the public key matches the stake owner
    if (stakeLock.ownerPubKey != payload.stakePubKey) {
        LogPrintf("ValidateUnstakeTransaction: Public key mismatch\n");
        return false;
    }
    
    return true;
}

/**
 * Validate validator registration transaction payload
 */
bool ValidateValidatorRegistration(const CTransaction& tx, const CValidatorRegistrationPayload& payload, const Consensus::Params& consensusParams)
{
    // Check minimum stake amount for validators
    if (payload.nStakeAmount < consensusParams.nMinStakeAmount * 10) { // Validators need 10x minimum stake
        LogPrintf("ValidateValidatorRegistration: Validator stake amount %s below minimum %s\n", 
                 FormatMoney(payload.nStakeAmount), FormatMoney(consensusParams.nMinStakeAmount * 10));
        return false;
    }
    
    // Validate public key
    if (!payload.validatorPubKey.IsValid()) {
        LogPrintf("ValidateValidatorRegistration: Invalid validator public key\n");
        return false;
    }
    
    // Check if validator is already registered
    if (IsValidatorRegistered(payload.validatorPubKey)) {
        LogPrintf("ValidateValidatorRegistration: Validator %s already registered\n", 
                 HexStr(payload.validatorPubKey));
        return false;
    }
    
    // Validate reward script
    if (payload.rewardScript.empty()) {
        LogPrintf("ValidateValidatorRegistration: Empty reward script\n");
        return false;
    }
    
    // Check description length (optional field)
    if (payload.strDescription.length() > 256) {
        LogPrintf("ValidateValidatorRegistration: Description too long (%d > 256)\n", 
                 payload.strDescription.length());
        return false;
    }
    
    // Note: In a real implementation, we would need to look up the input values
    // from the UTXO set to verify sufficient funds. For now, we assume this is done elsewhere.
    
    CAmount totalOutput = 0;
    for (const auto& vout : tx.vout) {
        totalOutput += vout.nValue;
    }
    
    // The stake amount should be locked in the transaction outputs
    if (totalOutput < payload.nStakeAmount) {
        LogPrintf("ValidateValidatorRegistration: Insufficient output value %s for stake %s\n", 
                 FormatMoney(totalOutput), FormatMoney(payload.nStakeAmount));
        return false;
    }
    
    return true;
}

/**
 * Create a stake lock for a given transaction output
 */
bool CreateStakeLock(const COutPoint& outpoint, int64_t nLockDuration, CAmount nAmount, const CPubKey& ownerPubKey)
{
    // Check if already locked
    if (mapStakeLocks.find(outpoint) != mapStakeLocks.end()) {
        LogPrintf("CreateStakeLock: Output %s already locked\n", outpoint.ToString());
        return false;
    }
    
    int64_t nLockTime = GetTime() + nLockDuration;
    CStakeLock stakeLock(outpoint, nLockTime, nAmount, ownerPubKey);
    
    mapStakeLocks[outpoint] = stakeLock;
    
    LogPrintf("CreateStakeLock: Created stake lock for %s, amount %s, expires at %d\n", 
             outpoint.ToString(), FormatMoney(nAmount), nLockTime);
    
    return true;
}

/**
 * Remove a stake lock (when unstaking)
 */
bool RemoveStakeLock(const COutPoint& outpoint)
{
    auto it = mapStakeLocks.find(outpoint);
    if (it == mapStakeLocks.end()) {
        LogPrintf("RemoveStakeLock: No stake lock found for %s\n", outpoint.ToString());
        return false;
    }
    
    mapStakeLocks.erase(it);
    
    LogPrintf("RemoveStakeLock: Removed stake lock for %s\n", outpoint.ToString());
    
    return true;
}

/**
 * Check if an output is currently stake-locked
 */
bool IsStakeLocked(const COutPoint& outpoint, int64_t nCurrentTime)
{
    if (nCurrentTime == 0) {
        nCurrentTime = GetTime();
    }
    
    auto it = mapStakeLocks.find(outpoint);
    if (it == mapStakeLocks.end()) {
        return false;
    }
    
    return !it->second.IsExpired(nCurrentTime);
}

/**
 * Get stake lock information for an output
 */
bool GetStakeLock(const COutPoint& outpoint, CStakeLock& stakeLock)
{
    auto it = mapStakeLocks.find(outpoint);
    if (it == mapStakeLocks.end()) {
        return false;
    }
    
    stakeLock = it->second;
    return true;
}

// Global validator registry (in production, this would be in a database)
static std::map<CPubKey, CValidator> mapValidators;
static std::map<CPubKey, bool> mapValidatorStatus; // true = active, false = inactive

/**
 * Register a new validator
 */
bool RegisterValidator(const CPubKey& validatorPubKey, CAmount nStakeAmount, const CScript& rewardScript, const std::string& strDescription)
{
    // Check if validator is already registered
    if (mapValidators.find(validatorPubKey) != mapValidators.end()) {
        LogPrintf("RegisterValidator: Validator %s already registered\n", HexStr(validatorPubKey));
        return false;
    }
    
    // Create validator entry
    CValidator validator;
    validator.pubkey = validatorPubKey;
    validator.nStakeAmount = nStakeAmount;
    validator.nRegistrationTime = GetTime();
    validator.fActive = true;
    
    // Add to registry
    mapValidators[validatorPubKey] = validator;
    mapValidatorStatus[validatorPubKey] = true;
    
    LogPrintf("RegisterValidator: Registered validator %s with stake %s\n", 
             HexStr(validatorPubKey), FormatMoney(nStakeAmount));
    
    return true;
}

/**
 * Update validator stake amount
 */
bool UpdateValidatorStake(const CPubKey& validatorPubKey, CAmount nNewStakeAmount)
{
    auto it = mapValidators.find(validatorPubKey);
    if (it == mapValidators.end()) {
        LogPrintf("UpdateValidatorStake: Validator %s not found\n", HexStr(validatorPubKey));
        return false;
    }
    
    CAmount oldStake = it->second.nStakeAmount;
    it->second.nStakeAmount = nNewStakeAmount;
    
    LogPrintf("UpdateValidatorStake: Updated validator %s stake from %s to %s\n", 
             HexStr(validatorPubKey), FormatMoney(oldStake), FormatMoney(nNewStakeAmount));
    
    return true;
}

/**
 * Set validator active/inactive status
 */
bool SetValidatorStatus(const CPubKey& validatorPubKey, bool fActive)
{
    auto it = mapValidators.find(validatorPubKey);
    if (it == mapValidators.end()) {
        LogPrintf("SetValidatorStatus: Validator %s not found\n", HexStr(validatorPubKey));
        return false;
    }
    
    it->second.fActive = fActive;
    mapValidatorStatus[validatorPubKey] = fActive;
    
    LogPrintf("SetValidatorStatus: Set validator %s status to %s\n", 
             HexStr(validatorPubKey), fActive ? "active" : "inactive");
    
    return true;
}

/**
 * Get validator information
 */
bool GetValidator(const CPubKey& validatorPubKey, CValidator& validator)
{
    auto it = mapValidators.find(validatorPubKey);
    if (it == mapValidators.end()) {
        return false;
    }
    
    validator = it->second;
    return true;
}

/**
 * Check if validator is registered
 */
bool IsValidatorRegistered(const CPubKey& validatorPubKey)
{
    return mapValidators.find(validatorPubKey) != mapValidators.end();
}

/**
 * Check if validator is active
 */
bool IsValidatorActive(const CPubKey& validatorPubKey)
{
    auto it = mapValidatorStatus.find(validatorPubKey);
    if (it == mapValidatorStatus.end()) {
        return false;
    }
    
    return it->second;
}

/**
 * Get all active validators
 */
std::vector<CValidator> GetActiveValidators()
{
    std::vector<CValidator> activeValidators;
    
    for (const auto& pair : mapValidators) {
        if (pair.second.fActive && IsValidatorActive(pair.first)) {
            activeValidators.push_back(pair.second);
        }
    }
    
    return activeValidators;
}

/**
 * Get total stake of all active validators
 */
CAmount GetTotalActiveStake()
{
    CAmount totalStake = 0;
    
    for (const auto& pair : mapValidators) {
        if (pair.second.fActive && IsValidatorActive(pair.first)) {
            totalStake += pair.second.nStakeAmount;
        }
    }
    
    return totalStake;
}

/**
 * Remove validator from registry (for slashing or voluntary exit)
 */
bool RemoveValidator(const CPubKey& validatorPubKey)
{
    auto it = mapValidators.find(validatorPubKey);
    if (it == mapValidators.end()) {
        LogPrintf("RemoveValidator: Validator %s not found\n", HexStr(validatorPubKey));
        return false;
    }
    
    mapValidators.erase(it);
    mapValidatorStatus.erase(validatorPubKey);
    
    LogPrintf("RemoveValidator: Removed validator %s from registry\n", HexStr(validatorPubKey));
    
    return true;
}

/**
 * Get validator count
 */
size_t GetValidatorCount()
{
    return mapValidators.size();
}

/**
 * Get active validator count
 */
size_t GetActiveValidatorCount()
{
    size_t count = 0;
    
    for (const auto& pair : mapValidators) {
        if (pair.second.fActive && IsValidatorActive(pair.first)) {
            count++;
        }
    }
    
    return count;
}

/**
 * Process validator registration transaction (called during block processing)
 */
bool ProcessValidatorRegistration(const CTransaction& tx, const CValidatorRegistrationPayload& payload)
{
    // Validation should have been done already, but double-check
    if (IsValidatorRegistered(payload.validatorPubKey)) {
        LogPrintf("ProcessValidatorRegistration: Validator %s already registered\n", 
                 HexStr(payload.validatorPubKey));
        return false;
    }
    
    // Register the validator
    bool success = RegisterValidator(payload.validatorPubKey, payload.nStakeAmount, 
                                   payload.rewardScript, payload.strDescription);
    
    if (success) {
        LogPrintf("ProcessValidatorRegistration: Successfully registered validator %s with stake %s\n", 
                 HexStr(payload.validatorPubKey), FormatMoney(payload.nStakeAmount));
    }
    
    return success;
}

/**
 * Process stake transaction (called during block processing)
 */
bool ProcessStakeTransaction(const CTransaction& tx, const CStakeTransactionPayload& payload)
{
    // Create stake lock for the first output (assuming it contains the staked amount)
    if (tx.vout.empty()) {
        LogPrintf("ProcessStakeTransaction: No outputs in stake transaction\n");
        return false;
    }
    
    // Create outpoint for the first output
    COutPoint stakeOutpoint(tx.GetHash(), 0);
    
    // Create stake lock
    bool success = CreateStakeLock(stakeOutpoint, payload.nLockTime, payload.nStakeAmount, payload.stakePubKey);
    
    if (success) {
        LogPrintf("ProcessStakeTransaction: Created stake lock for %s, amount %s, duration %d seconds\n", 
                 stakeOutpoint.ToString(), FormatMoney(payload.nStakeAmount), payload.nLockTime);
    }
    
    return success;
}

/**
 * Process unstake transaction (called during block processing)
 */
bool ProcessUnstakeTransaction(const CTransaction& tx, const CUnstakeTransactionPayload& payload)
{
    // Remove the stake lock
    bool success = RemoveStakeLock(payload.stakeOutpoint);
    
    if (success) {
        LogPrintf("ProcessUnstakeTransaction: Removed stake lock for %s\n", 
                 payload.stakeOutpoint.ToString());
    }
    
    return success;
}

// Global slashing and blacklist management
static std::map<CPubKey, int64_t> mapSlashedValidators; // validator -> slash time
static std::map<CPubKey, CAmount> mapSlashPenalties;    // validator -> penalty amount
static std::set<CPubKey> setBlacklistedValidators;     // permanently blacklisted validators

/**
 * Calculate slashing penalty based on condition and validator stake
 */
CAmount CalculateSlashingPenalty(const CPubKey& validatorPubKey, SlashingCondition condition, const Consensus::Params& consensusParams)
{
    CValidator validator;
    if (!GetValidator(validatorPubKey, validator)) {
        LogPrintf("CalculateSlashingPenalty: Validator %s not found\n", HexStr(validatorPubKey));
        return 0;
    }
    
    CAmount penalty = 0;
    
    switch (condition) {
        case SLASH_DOUBLE_SIGNING:
            // Severe penalty: 50% of stake
            penalty = validator.nStakeAmount / 2;
            break;
            
        case SLASH_LONG_RANGE_ATTACK:
            // Maximum penalty: 100% of stake (permanent slashing)
            penalty = validator.nStakeAmount;
            break;
            
        case SLASH_UNAVAILABILITY:
            // Moderate penalty: 5% of stake
            penalty = validator.nStakeAmount / 20;
            break;
            
        case SLASH_INVALID_BLOCK:
            // Significant penalty: 25% of stake
            penalty = validator.nStakeAmount / 4;
            break;
            
        case SLASH_EQUIVOCATION:
            // Severe penalty: 40% of stake
            penalty = (validator.nStakeAmount * 2) / 5;
            break;
            
        default:
            LogPrintf("CalculateSlashingPenalty: Unknown slashing condition %d\n", condition);
            penalty = 0;
            break;
    }
    
    // Ensure penalty doesn't exceed validator's stake
    if (penalty > validator.nStakeAmount) {
        penalty = validator.nStakeAmount;
    }
    
    LogPrintf("CalculateSlashingPenalty: Validator %s, condition %d, penalty %s\n", 
             HexStr(validatorPubKey), condition, FormatMoney(penalty));
    
    return penalty;
}

/**
 * Apply slashing penalty to a validator
 */
bool SlashValidator(const CPubKey& validatorPubKey, SlashingCondition condition, const SlashingEvidence& evidence, const Consensus::Params& consensusParams)
{
    // Check if validator exists
    if (!IsValidatorRegistered(validatorPubKey)) {
        LogPrintf("SlashValidator: Validator %s not registered\n", HexStr(validatorPubKey));
        return false;
    }
    
    // Check if validator is already slashed
    if (mapSlashedValidators.find(validatorPubKey) != mapSlashedValidators.end()) {
        LogPrintf("SlashValidator: Validator %s already slashed\n", HexStr(validatorPubKey));
        return false;
    }
    
    // Calculate penalty
    CAmount penalty = CalculateSlashingPenalty(validatorPubKey, condition, consensusParams);
    if (penalty == 0) {
        LogPrintf("SlashValidator: No penalty calculated for validator %s\n", HexStr(validatorPubKey));
        return false;
    }
    
    // Apply penalty
    CValidator validator;
    if (!GetValidator(validatorPubKey, validator)) {
        return false;
    }
    
    // Reduce validator's stake
    CAmount newStake = validator.nStakeAmount - penalty;
    if (newStake < 0) newStake = 0;
    
    UpdateValidatorStake(validatorPubKey, newStake);
    
    // Record slashing
    mapSlashedValidators[validatorPubKey] = GetTime();
    mapSlashPenalties[validatorPubKey] = penalty;
    
    // Deactivate validator
    SetValidatorStatus(validatorPubKey, false);
    
    // For severe offenses, add to blacklist
    if (condition == SLASH_LONG_RANGE_ATTACK || condition == SLASH_DOUBLE_SIGNING) {
        setBlacklistedValidators.insert(validatorPubKey);
        LogPrintf("SlashValidator: Blacklisted validator %s for severe offense\n", HexStr(validatorPubKey));
    }
    
    LogPrintf("SlashValidator: Slashed validator %s, penalty %s, new stake %s\n", 
             HexStr(validatorPubKey), FormatMoney(penalty), FormatMoney(newStake));
    
    return true;
}

/**
 * Check if validator is blacklisted
 */
bool IsValidatorBlacklisted(const CPubKey& validatorPubKey)
{
    return setBlacklistedValidators.find(validatorPubKey) != setBlacklistedValidators.end();
}

/**
 * Check if validator has been slashed
 */
bool IsValidatorSlashed(const CPubKey& validatorPubKey)
{
    return mapSlashedValidators.find(validatorPubKey) != mapSlashedValidators.end();
}

/**
 * Get slashing information for a validator
 */
bool GetSlashingInfo(const CPubKey& validatorPubKey, int64_t& slashTime, CAmount& penalty)
{
    auto timeIt = mapSlashedValidators.find(validatorPubKey);
    auto penaltyIt = mapSlashPenalties.find(validatorPubKey);
    
    if (timeIt == mapSlashedValidators.end() || penaltyIt == mapSlashPenalties.end()) {
        return false;
    }
    
    slashTime = timeIt->second;
    penalty = penaltyIt->second;
    return true;
}

/**
 * Detect double signing (validator signed two different blocks at same height)
 */
bool DetectDoubleSigning(const CPubKey& validatorPubKey, const uint256& blockHash1, const uint256& blockHash2, int nHeight)
{
    if (blockHash1 == blockHash2) {
        return false; // Same block, not double signing
    }
    
    LogPrintf("DetectDoubleSigning: Validator %s signed two blocks at height %d: %s and %s\n", 
             HexStr(validatorPubKey), nHeight, blockHash1.ToString(), blockHash2.ToString());
    
    // Create slashing evidence
    SlashingEvidence evidence;
    evidence.validatorPubKey = validatorPubKey;
    evidence.condition = SLASH_DOUBLE_SIGNING;
    evidence.nTime = GetTime();
    evidence.blockHash1 = blockHash1;
    evidence.blockHash2 = blockHash2;
    
    // Apply slashing
    return SlashValidator(validatorPubKey, SLASH_DOUBLE_SIGNING, evidence, Params().GetConsensus());
}

/**
 * Detect validator unavailability (missed too many blocks)
 */
bool DetectUnavailability(const CPubKey& validatorPubKey, int nMissedBlocks, int nTotalBlocks)
{
    // Threshold: if validator missed more than 20% of blocks in recent period
    double missRate = static_cast<double>(nMissedBlocks) / static_cast<double>(nTotalBlocks);
    
    if (missRate <= 0.2) {
        return false; // Within acceptable range
    }
    
    LogPrintf("DetectUnavailability: Validator %s missed %d/%d blocks (%.2f%%)\n", 
             HexStr(validatorPubKey), nMissedBlocks, nTotalBlocks, missRate * 100);
    
    // Create slashing evidence
    SlashingEvidence evidence;
    evidence.validatorPubKey = validatorPubKey;
    evidence.condition = SLASH_UNAVAILABILITY;
    evidence.nTime = GetTime();
    
    // Apply slashing
    return SlashValidator(validatorPubKey, SLASH_UNAVAILABILITY, evidence, Params().GetConsensus());
}

/**
 * Remove validator from blacklist (for governance decisions)
 */
bool RemoveFromBlacklist(const CPubKey& validatorPubKey)
{
    auto it = setBlacklistedValidators.find(validatorPubKey);
    if (it == setBlacklistedValidators.end()) {
        LogPrintf("RemoveFromBlacklist: Validator %s not blacklisted\n", HexStr(validatorPubKey));
        return false;
    }
    
    setBlacklistedValidators.erase(it);
    
    LogPrintf("RemoveFromBlacklist: Removed validator %s from blacklist\n", HexStr(validatorPubKey));
    
    return true;
}

/**
 * Get all blacklisted validators
 */
std::vector<CPubKey> GetBlacklistedValidators()
{
    std::vector<CPubKey> blacklisted;
    for (const auto& pubkey : setBlacklistedValidators) {
        blacklisted.push_back(pubkey);
    }
    return blacklisted;
}

/**
 * Get all slashed validators
 */
std::vector<CPubKey> GetSlashedValidators()
{
    std::vector<CPubKey> slashed;
    for (const auto& pair : mapSlashedValidators) {
        slashed.push_back(pair.first);
    }
    return slashed;
}

/**
 * Check proof of stake for a block
 */
bool CheckProofOfStake(const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams, CCoinsViewCache& view)
{
    // For now, implement a basic PoS check
    // In a real implementation, this would verify:
    // 1. The block is signed by a valid validator
    // 2. The validator has sufficient stake
    // 3. The validator is selected based on stake weight
    // 4. The block timestamp is valid for PoS
    
    if (block.vtx.empty()) {
        LogPrintf("CheckProofOfStake: Block has no transactions\n");
        return false;
    }
    
    // Check if this is a PoS block (has staking transaction)
    bool hasStakingTx = false;
    for (const auto& tx : block.vtx) {
        if (tx->nType == TRANSACTION_STAKE || tx->nType == TRANSACTION_VALIDATOR_REGISTER) {
            hasStakingTx = true;
            break;
        }
    }
    
    // For now, accept blocks without staking transactions (during transition)
    if (!hasStakingTx) {
        LogPrint(BCLog::POS, "CheckProofOfStake: Block has no staking transactions, allowing during transition\n");
        return true;
    }
    
    // Basic timestamp check
    if (block.nTime <= pindexPrev->nTime) {
        LogPrintf("CheckProofOfStake: Block timestamp %d not greater than previous %d\n", 
                 block.nTime, pindexPrev->nTime);
        return false;
    }
    
    // Check block time is not too far in the future
    int64_t nCurrentTime = GetTime();
    if (block.nTime > nCurrentTime + consensusParams.nStakeTargetSpacing * 2) {
        LogPrintf("CheckProofOfStake: Block timestamp %d too far in future (current: %d)\n", 
                 block.nTime, nCurrentTime);
        return false;
    }
    
    LogPrint(BCLog::POS, "CheckProofOfStake: Block passed basic PoS validation\n");
    return true;
}