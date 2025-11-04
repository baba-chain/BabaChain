// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POS_H
#define BITCOIN_POS_H

#include <consensus/amount.h>
#include <consensus/params.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <pubkey.h>
#include <script/script.h>
#include <serialize.h>
#include <tinyformat.h>

#include <vector>
#include <map>
#include <string>

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
 * Staking transaction payload for TRANSACTION_STAKE
 */
class CStakeTransactionPayload {
public:
    CAmount nStakeAmount;       // Amount to stake
    CPubKey stakePubKey;        // Public key for staking
    int64_t nLockTime;          // Lock duration in seconds
    
    CStakeTransactionPayload() : nStakeAmount(0), nLockTime(0) {}
    
    CStakeTransactionPayload(CAmount nStakeAmountIn, const CPubKey& stakePubKeyIn, int64_t nLockTimeIn)
        : nStakeAmount(nStakeAmountIn), stakePubKey(stakePubKeyIn), nLockTime(nLockTimeIn) {}
    
    SERIALIZE_METHODS(CStakeTransactionPayload, obj) {
        READWRITE(obj.nStakeAmount, obj.stakePubKey, obj.nLockTime);
    }
    
    std::string ToString() const;
};

/**
 * Unstaking transaction payload for TRANSACTION_UNSTAKE
 */
class CUnstakeTransactionPayload {
public:
    COutPoint stakeOutpoint;    // Reference to original stake transaction
    CPubKey stakePubKey;        // Public key that created the stake
    
    CUnstakeTransactionPayload() {}
    
    CUnstakeTransactionPayload(const COutPoint& stakeOutpointIn, const CPubKey& stakePubKeyIn)
        : stakeOutpoint(stakeOutpointIn), stakePubKey(stakePubKeyIn) {}
    
    SERIALIZE_METHODS(CUnstakeTransactionPayload, obj) {
        READWRITE(obj.stakeOutpoint, obj.stakePubKey);
    }
    
    std::string ToString() const;
};

/**
 * Validator registration transaction payload for TRANSACTION_VALIDATOR_REGISTER
 */
class CValidatorRegistrationPayload {
public:
    CPubKey validatorPubKey;    // Validator's public key
    CAmount nStakeAmount;       // Initial stake amount
    CScript rewardScript;       // Script for receiving rewards
    std::string strDescription; // Optional validator description
    
    CValidatorRegistrationPayload() : nStakeAmount(0) {}
    
    CValidatorRegistrationPayload(const CPubKey& validatorPubKeyIn, CAmount nStakeAmountIn, 
                                 const CScript& rewardScriptIn, const std::string& strDescriptionIn = "")
        : validatorPubKey(validatorPubKeyIn), nStakeAmount(nStakeAmountIn), 
          rewardScript(rewardScriptIn), strDescription(strDescriptionIn) {}
    
    SERIALIZE_METHODS(CValidatorRegistrationPayload, obj) {
        READWRITE(obj.validatorPubKey, obj.nStakeAmount, obj.rewardScript, obj.strDescription);
    }
    
    std::string ToString() const;
};

/**
 * Stake locking mechanism
 */
class CStakeLock {
public:
    COutPoint outpoint;         // Locked output
    int64_t nLockTime;          // Lock expiration time
    CAmount nAmount;            // Locked amount
    CPubKey ownerPubKey;        // Owner's public key
    
    CStakeLock() : nLockTime(0), nAmount(0) {}
    
    CStakeLock(const COutPoint& outpointIn, int64_t nLockTimeIn, CAmount nAmountIn, const CPubKey& ownerPubKeyIn)
        : outpoint(outpointIn), nLockTime(nLockTimeIn), nAmount(nAmountIn), ownerPubKey(ownerPubKeyIn) {}
    
    SERIALIZE_METHODS(CStakeLock, obj) {
        READWRITE(obj.outpoint, obj.nLockTime, obj.nAmount, obj.ownerPubKey);
    }
    
    bool IsExpired(int64_t nCurrentTime) const {
        return nCurrentTime >= nLockTime;
    }
    
    std::string ToString() const;
};

/**
 * Validate staking transaction
 */
bool ValidateStakingTransaction(const CTransaction& tx, const Consensus::Params& consensusParams);

/**
 * Validate stake transaction payload
 */
bool ValidateStakeTransaction(const CTransaction& tx, const CStakeTransactionPayload& payload, const Consensus::Params& consensusParams);

/**
 * Validate unstake transaction payload
 */
bool ValidateUnstakeTransaction(const CTransaction& tx, const CUnstakeTransactionPayload& payload, const Consensus::Params& consensusParams);

/**
 * Validate validator registration transaction payload
 */
bool ValidateValidatorRegistration(const CTransaction& tx, const CValidatorRegistrationPayload& payload, const Consensus::Params& consensusParams);

/**
 * Create a stake lock for a given transaction output
 */
bool CreateStakeLock(const COutPoint& outpoint, int64_t nLockDuration, CAmount nAmount, const CPubKey& ownerPubKey);

/**
 * Remove a stake lock (when unstaking)
 */
bool RemoveStakeLock(const COutPoint& outpoint);

/**
 * Check if an output is currently stake-locked
 */
bool IsStakeLocked(const COutPoint& outpoint, int64_t nCurrentTime = 0);

/**
 * Get stake lock information for an output
 */
bool GetStakeLock(const COutPoint& outpoint, CStakeLock& stakeLock);

/**
 * Validator Registry Functions
 */

/**
 * Register a new validator
 */
bool RegisterValidator(const CPubKey& validatorPubKey, CAmount nStakeAmount, const CScript& rewardScript, const std::string& strDescription = "");

/**
 * Update validator stake amount
 */
bool UpdateValidatorStake(const CPubKey& validatorPubKey, CAmount nNewStakeAmount);

/**
 * Set validator active/inactive status
 */
bool SetValidatorStatus(const CPubKey& validatorPubKey, bool fActive);

/**
 * Get validator information
 */
bool GetValidator(const CPubKey& validatorPubKey, CValidator& validator);

/**
 * Check if validator is registered
 */
bool IsValidatorRegistered(const CPubKey& validatorPubKey);

/**
 * Check if validator is active
 */
bool IsValidatorActive(const CPubKey& validatorPubKey);

/**
 * Get all active validators
 */
std::vector<CValidator> GetActiveValidators();

/**
 * Get total stake of all active validators
 */
CAmount GetTotalActiveStake();

/**
 * Remove validator from registry (for slashing or voluntary exit)
 */
bool RemoveValidator(const CPubKey& validatorPubKey);

/**
 * Get validator count
 */
size_t GetValidatorCount();

/**
 * Get active validator count
 */
size_t GetActiveValidatorCount();

/**
 * Transaction Processing Functions
 */

/**
 * Process validator registration transaction (called during block processing)
 */
bool ProcessValidatorRegistration(const CTransaction& tx, const CValidatorRegistrationPayload& payload);

/**
 * Process stake transaction (called during block processing)
 */
bool ProcessStakeTransaction(const CTransaction& tx, const CStakeTransactionPayload& payload);

/**
 * Process unstake transaction (called during block processing)
 */
bool ProcessUnstakeTransaction(const CTransaction& tx, const CUnstakeTransactionPayload& payload);

#endif // BITCOIN_POS_H