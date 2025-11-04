// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POS_H
#define BITCOIN_POS_H

#include <consensus/amount.h>
#include <consensus/params.h>
#include <primitives/transaction.h>
#include <uint256.h>

#include <vector>

/**
 * Proof of Stake input structure
 */
class CStakeInput {
public:
    COutPoint prevout;          // Previous output being staked
    CAmount nValue;             // Amount being staked
    int64_t nTime;              // Stake time
    uint256 hashBlock;          // Block hash where stake originates
    
    CStakeInput() : nValue(0), nTime(0) {}
    
    CStakeInput(const COutPoint& prevoutIn, CAmount nValueIn, int64_t nTimeIn, const uint256& hashBlockIn)
        : prevout(prevoutIn), nValue(nValueIn), nTime(nTimeIn), hashBlock(hashBlockIn) {}
    
    SERIALIZE_METHODS(CStakeInput, obj) {
        READWRITE(obj.prevout, obj.nValue, obj.nTime, obj.hashBlock);
    }
};

/**
 * Proof of Stake output structure
 */
class CStakeOutput {
public:
    CScript scriptPubKey;       // Destination script
    CAmount nValue;             // Stake reward amount
    
    CStakeOutput() : nValue(0) {}
    
    CStakeOutput(const CScript& scriptPubKeyIn, CAmount nValueIn)
        : scriptPubKey(scriptPubKeyIn), nValue(nValueIn) {}
    
    SERIALIZE_METHODS(CStakeOutput, obj) {
        READWRITE(obj.scriptPubKey, obj.nValue);
    }
};

/**
 * Validator information structure
 */
class CValidator {
public:
    CPubKey pubkey;             // Validator public key
    CAmount nStakeAmount;       // Total staked amount
    int64_t nRegistrationTime;  // When validator registered
    bool fActive;               // Validator status
    
    CValidator() : nStakeAmount(0), nRegistrationTime(0), fActive(false) {}
    
    SERIALIZE_METHODS(CValidator, obj) {
        READWRITE(obj.pubkey, obj.nStakeAmount, obj.nRegistrationTime, obj.fActive);
    }
};

/**
 * Calculate staking reward based on stake amount and duration
 */
CAmount CalculateStakeReward(const CAmount& stakeAmount, int64_t stakeDuration, const Consensus::Params& consensusParams);

/**
 * Distribute staking rewards to validators
 */
bool DistributeStakingRewards(const std::vector<CStakeInput>& validators, CAmount totalReward, std::vector<CAmount>& rewards);

/**
 * Check if total supply cap is enforced
 */
bool IsSupplyCapEnforced(int nHeight, const Consensus::Params& consensusParams);

/**
 * Get remaining supply available for staking rewards
 */
CAmount GetRemainingStakingSupply(int nHeight, const Consensus::Params& consensusParams);

/**
 * Validate staking transaction
 */
bool ValidateStakingTransaction(const CTransaction& tx, const Consensus::Params& consensusParams);

#endif // BITCOIN_POS_H