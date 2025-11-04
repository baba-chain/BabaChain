// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pos.h>
#include <validation.h>
#include <chainparams.h>
#include <consensus/consensus.h>
#include <util/moneystr.h>
#include <logging.h>

/**
 * Calculate staking reward based on stake amount and duration
 */
CAmount CalculateStakeReward(const CAmount& stakeAmount, int64_t stakeDuration, const Consensus::Params& consensusParams)
{
    // Base reward calculation: proportional to stake amount
    CAmount baseReward = consensusParams.nStakeRewardPerBlock;
    
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
        totalSupply += (nHeight - 1) * consensusParams.nStakeRewardPerBlock;
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
        distributedRewards = (nHeight - 1) * consensusParams.nStakeRewardPerBlock;
    }
    
    // Return remaining from staking supply pool
    if (distributedRewards >= consensusParams.nStakingSupply) {
        return 0;
    }
    
    return consensusParams.nStakingSupply - distributedRewards;
}

/**
 * Validate staking transaction
 */
bool ValidateStakingTransaction(const CTransaction& tx, const Consensus::Params& consensusParams)
{
    // Basic validation checks
    if (tx.vin.empty() || tx.vout.empty()) {
        return false;
    }
    
    // Check for staking-specific transaction markers
    // This is a placeholder - actual implementation would check for
    // specific transaction types and validation rules
    
    return true;
}