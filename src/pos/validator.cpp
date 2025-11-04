// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pos/validator.h>

#include <hash.h>
#include <logging.h>
#include <random.h>
#include <util/time.h>

#include <algorithm>
#include <numeric>

bool CValidatorRegistry::RegisterValidator(const CValidator& validator)
{
    LOCK(cs_validators);
    
    uint256 validatorHash = GetValidatorHash(validator.pubkey);
    
    // Check if validator already exists
    if (mapValidators.find(validatorHash) != mapValidators.end()) {
        LogPrint(BCLog::POS, "CValidatorRegistry::%s: Validator %s already registered\n", 
                 __func__, validatorHash.ToString());
        return false;
    }
    
    // Check if stake input is already used
    if (mapStakeToValidator.find(validator.stakeInput.prevout) != mapStakeToValidator.end()) {
        LogPrint(BCLog::POS, "CValidatorRegistry::%s: Stake input %s already used\n", 
                 __func__, validator.stakeInput.prevout.ToString());
        return false;
    }
    
    // Validate stake input
    if (!validator.stakeInput.IsValid()) {
        LogPrint(BCLog::POS, "CValidatorRegistry::%s: Invalid stake input for validator %s\n", 
                 __func__, validatorHash.ToString());
        return false;
    }
    
    // Create validator copy and add to registry
    auto validatorPtr = std::make_shared<CValidator>(validator);
    mapValidators[validatorHash] = validatorPtr;
    mapStakeToValidator[validator.stakeInput.prevout] = validatorHash;
    
    if (validator.IsActive()) {
        setActiveValidators.insert(validatorHash);
    }
    
    LogPrint(BCLog::POS, "CValidatorRegistry::%s: Registered validator %s with stake %s\n", 
             __func__, validatorHash.ToString(), FormatMoney(validator.nStakeAmount));
    
    return true;
}

bool CValidatorRegistry::DeregisterValidator(const uint256& validatorHash)
{
    LOCK(cs_validators);
    
    auto it = mapValidators.find(validatorHash);
    if (it == mapValidators.end()) {
        LogPrint(BCLog::POS, "CValidatorRegistry::%s: Validator %s not found\n", 
                 __func__, validatorHash.ToString());
        return false;
    }
    
    auto validator = it->second;
    
    // Remove from stake mapping
    mapStakeToValidator.erase(validator->stakeInput.prevout);
    
    // Remove from active set
    setActiveValidators.erase(validatorHash);
    
    // Update status to deregistered
    validator->status = ValidatorStatus::DEREGISTERED;
    
    // Remove from main map
    mapValidators.erase(it);
    
    LogPrint(BCLog::POS, "CValidatorRegistry::%s: Deregistered validator %s\n", 
             __func__, validatorHash.ToString());
    
    return true;
}

bool CValidatorRegistry::UpdateValidatorStake(const uint256& validatorHash, const CStakeInput& newStake)
{
    LOCK(cs_validators);
    
    auto it = mapValidators.find(validatorHash);
    if (it == mapValidators.end()) {
        LogPrint(BCLog::POS, "CValidatorRegistry::%s: Validator %s not found\n", 
                 __func__, validatorHash.ToString());
        return false;
    }
    
    auto validator = it->second;
    
    // Validate new stake input
    if (!newStake.IsValid()) {
        LogPrint(BCLog::POS, "CValidatorRegistry::%s: Invalid new stake input for validator %s\n", 
                 __func__, validatorHash.ToString());
        return false;
    }
    
    // Check if new stake input is already used by another validator
    auto stakeIt = mapStakeToValidator.find(newStake.prevout);
    if (stakeIt != mapStakeToValidator.end() && stakeIt->second != validatorHash) {
        LogPrint(BCLog::POS, "CValidatorRegistry::%s: New stake input %s already used by another validator\n", 
                 __func__, newStake.prevout.ToString());
        return false;
    }
    
    // Remove old stake mapping
    mapStakeToValidator.erase(validator->stakeInput.prevout);
    
    // Update validator stake
    validator->stakeInput = newStake;
    validator->nStakeAmount = newStake.nValue;
    
    // Add new stake mapping
    mapStakeToValidator[newStake.prevout] = validatorHash;
    
    LogPrint(BCLog::POS, "CValidatorRegistry::%s: Updated validator %s stake to %s\n", 
             __func__, validatorHash.ToString(), FormatMoney(newStake.nValue));
    
    return true;
}

bool CValidatorRegistry::SlashValidator(const uint256& validatorHash, const std::string& reason)
{
    LOCK(cs_validators);
    
    auto it = mapValidators.find(validatorHash);
    if (it == mapValidators.end()) {
        LogPrint(BCLog::POS, "CValidatorRegistry::%s: Validator %s not found\n", 
                 __func__, validatorHash.ToString());
        return false;
    }
    
    auto validator = it->second;
    
    // Increment slashing count
    validator->nSlashingCount++;
    
    // Update status to slashed
    validator->status = ValidatorStatus::SLASHED;
    
    // Remove from active set
    setActiveValidators.erase(validatorHash);
    
    LogPrint(BCLog::POS, "CValidatorRegistry::%s: Slashed validator %s (count: %d, reason: %s)\n", 
             __func__, validatorHash.ToString(), validator->nSlashingCount, reason);
    
    return true;
}

CValidatorPtr CValidatorRegistry::GetValidator(const uint256& validatorHash) const
{
    LOCK(cs_validators);
    
    auto it = mapValidators.find(validatorHash);
    if (it != mapValidators.end()) {
        return it->second;
    }
    
    return nullptr;
}

CValidatorPtr CValidatorRegistry::GetValidatorByStake(const COutPoint& stakeInput) const
{
    LOCK(cs_validators);
    
    auto it = mapStakeToValidator.find(stakeInput);
    if (it != mapStakeToValidator.end()) {
        return GetValidator(it->second);
    }
    
    return nullptr;
}

std::vector<CValidatorPtr> CValidatorRegistry::GetActiveValidators() const
{
    LOCK(cs_validators);
    
    std::vector<CValidatorPtr> activeValidators;
    activeValidators.reserve(setActiveValidators.size());
    
    for (const auto& validatorHash : setActiveValidators) {
        auto it = mapValidators.find(validatorHash);
        if (it != mapValidators.end() && it->second->IsActive()) {
            activeValidators.push_back(it->second);
        }
    }
    
    return activeValidators;
}

std::vector<CValidatorPtr> CValidatorRegistry::GetEligibleValidators(int64_t currentTime) const
{
    LOCK(cs_validators);
    
    std::vector<CValidatorPtr> eligibleValidators;
    
    for (const auto& validatorHash : setActiveValidators) {
        auto it = mapValidators.find(validatorHash);
        if (it != mapValidators.end() && it->second->IsEligible(currentTime)) {
            eligibleValidators.push_back(it->second);
        }
    }
    
    return eligibleValidators;
}

CAmount CValidatorRegistry::GetTotalActiveStake() const
{
    LOCK(cs_validators);
    
    CAmount totalStake = 0;
    
    for (const auto& validatorHash : setActiveValidators) {
        auto it = mapValidators.find(validatorHash);
        if (it != mapValidators.end() && it->second->IsActive()) {
            totalStake += it->second->nStakeAmount;
        }
    }
    
    return totalStake;
}

bool CValidatorRegistry::HasValidator(const uint256& validatorHash) const
{
    LOCK(cs_validators);
    return mapValidators.find(validatorHash) != mapValidators.end();
}

bool CValidatorRegistry::IsStakeUsed(const COutPoint& stakeInput) const
{
    LOCK(cs_validators);
    return mapStakeToValidator.find(stakeInput) != mapStakeToValidator.end();
}

size_t CValidatorRegistry::GetValidatorCount() const
{
    LOCK(cs_validators);
    return mapValidators.size();
}

size_t CValidatorRegistry::GetActiveValidatorCount() const
{
    LOCK(cs_validators);
    return setActiveValidators.size();
}

void CValidatorRegistry::Clear()
{
    LOCK(cs_validators);
    mapValidators.clear();
    mapStakeToValidator.clear();
    setActiveValidators.clear();
}

CValidatorPtr CValidatorSelector::SelectValidator(const uint256& blockHash, int64_t currentTime) const
{
    // Get all eligible validators
    auto eligibleValidators = registry.GetEligibleValidators(currentTime);
    
    if (eligibleValidators.empty()) {
        LogPrint(BCLog::POS, "CValidatorSelector::%s: No eligible validators found\n", __func__);
        return nullptr;
    }
    
    // Calculate total stake of eligible validators
    CAmount totalStake = std::accumulate(eligibleValidators.begin(), eligibleValidators.end(), CAmount(0),
        [](CAmount sum, const CValidatorPtr& validator) {
            return sum + validator->nStakeAmount;
        });
    
    if (totalStake <= 0) {
        LogPrint(BCLog::POS, "CValidatorSelector::%s: Total stake is zero\n", __func__);
        return nullptr;
    }
    
    // Find validator with highest selection hash weighted by stake
    CValidatorPtr selectedValidator = nullptr;
    arith_uint256 highestScore = 0;
    
    for (const auto& validator : eligibleValidators) {
        // Calculate selection hash for this validator
        arith_uint256 selectionHash = GetSelectionHash(blockHash, validator->pubkey);
        
        // Weight the hash by stake amount (higher stake = higher chance)
        // Use 256-bit arithmetic to avoid overflow
        arith_uint256 stakeWeight = arith_uint256(validator->nStakeAmount.GetLow64());
        arith_uint256 weightedScore = (selectionHash / (totalStake.GetLow64() / stakeWeight.GetLow64()));
        
        if (weightedScore > highestScore) {
            highestScore = weightedScore;
            selectedValidator = validator;
        }
    }
    
    if (selectedValidator) {
        LogPrint(BCLog::POS, "CValidatorSelector::%s: Selected validator %s with stake %s (score: %s)\n", 
                 __func__, 
                 Hash(selectedValidator->pubkey.begin(), selectedValidator->pubkey.end()).ToString(),
                 FormatMoney(selectedValidator->nStakeAmount),
                 highestScore.ToString());
    }
    
    return selectedValidator;
}

double CValidatorSelector::CalculateSelectionProbability(const CValidator& validator, CAmount totalStake) const
{
    if (totalStake <= 0 || validator.nStakeAmount <= 0) {
        return 0.0;
    }
    
    return static_cast<double>(validator.nStakeAmount) / static_cast<double>(totalStake);
}

bool CValidatorSelector::VerifyValidatorSelection(const CValidator& validator, const uint256& blockHash, int64_t currentTime) const
{
    // Check if validator is eligible
    if (!validator.IsEligible(currentTime)) {
        LogPrint(BCLog::POS, "CValidatorSelector::%s: Validator is not eligible\n", __func__);
        return false;
    }
    
    // Get all eligible validators at the time
    auto eligibleValidators = registry.GetEligibleValidators(currentTime);
    
    if (eligibleValidators.empty()) {
        LogPrint(BCLog::POS, "CValidatorSelector::%s: No eligible validators found\n", __func__);
        return false;
    }
    
    // Check if the validator was in the eligible set
    uint256 validatorHash = Hash(validator.pubkey.begin(), validator.pubkey.end());
    bool found = false;
    for (const auto& eligibleValidator : eligibleValidators) {
        uint256 eligibleHash = Hash(eligibleValidator->pubkey.begin(), eligibleValidator->pubkey.end());
        if (eligibleHash == validatorHash) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        LogPrint(BCLog::POS, "CValidatorSelector::%s: Validator was not in eligible set\n", __func__);
        return false;
    }
    
    // Verify that this validator would have been selected using the same algorithm
    auto selectedValidator = SelectValidator(blockHash, currentTime);
    if (!selectedValidator) {
        LogPrint(BCLog::POS, "CValidatorSelector::%s: No validator selected by algorithm\n", __func__);
        return false;
    }
    
    uint256 selectedHash = Hash(selectedValidator->pubkey.begin(), selectedValidator->pubkey.end());
    bool isCorrect = (selectedHash == validatorHash);
    
    if (!isCorrect) {
        LogPrint(BCLog::POS, "CValidatorSelector::%s: Validator selection mismatch. Expected: %s, Got: %s\n", 
                 __func__, selectedHash.ToString(), validatorHash.ToString());
    }
    
    return isCorrect;
}

arith_uint256 CValidatorSelector::GetSelectionHash(const uint256& blockHash, const CPubKey& validatorPubkey) const
{
    // Combine block hash and validator pubkey to create deterministic randomness
    CHashWriter hasher(SER_GETHASH, 0);
    hasher << blockHash;
    hasher << validatorPubkey;
    
    uint256 combinedHash = hasher.GetHash();
    return UintToArith256(combinedHash);
}