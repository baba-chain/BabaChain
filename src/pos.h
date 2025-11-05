// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BABACHAIN_POS_H
#define BABACHAIN_POS_H

#include <consensus/params.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/script.h>
#include <serialize.h>
#include <uint256.h>
#include <util/time.h>

#include <map>
#include <set>
#include <string>
#include <vector>

class CBlockIndex;
class CCoinsViewCache;

// Forward declarations
struct CStakeInput;
struct CValidator;

/**
 * Stake transaction payload for TRANSACTION_STAKE
 */
struct CStakeTransactionPayload {
    CAmount nStakeAmount;
    CPubKey stakePubKey;
    int64_t nLockTime;
    
    CStakeTransactionPayload() : nStakeAmount(0), nLockTime(0) {}
    
    SERIALIZE_METHODS(CStakeTransactionPayload, obj) {
        READWRITE(obj.nStakeAmount, obj.stakePubKey, obj.nLockTime);
    }
    
    std::string ToString() const;
};

/**
 * Unstake transaction payload for TRANSACTION_UNSTAKE
 */
struct CUnstakeTransactionPayload {
    COutPoint stakeOutpoint;
    CPubKey stakePubKey;
    
    CUnstakeTransactionPayload() {}
    
    SERIALIZE_METHODS(CUnstakeTransactionPayload, obj) {
        READWRITE(obj.stakeOutpoint, obj.stakePubKey);
    }
    
    std::string ToString() const;
};

/**
 * Validator registration payload for TRANSACTION_VALIDATOR_REGISTER
 */
struct CValidatorRegistrationPayload {
    CPubKey validatorPubKey;
    CAmount nStakeAmount;
    CScript rewardScript;
    std::string strDescription;
    
    CValidatorRegistrationPayload() : nStakeAmount(0) {}
    
    SERIALIZE_METHODS(CValidatorRegistrationPayload, obj) {
        READWRITE(obj.validatorPubKey, obj.nStakeAmount, obj.rewardScript, obj.strDescription);
    }
    
    std::string ToString() const;
};

/**
 * Stake lock structure
 */
struct CStakeLock {
    COutPoint outpoint;
    int64_t nLockTime;
    CAmount nAmount;
    CPubKey ownerPubKey;
    
    CStakeLock() : nLockTime(0), nAmount(0) {}
    CStakeLock(const COutPoint& outpoint_, int64_t nLockTime_, CAmount nAmount_, const CPubKey& ownerPubKey_) 
        : outpoint(outpoint_), nLockTime(nLockTime_), nAmount(nAmount_), ownerPubKey(ownerPubKey_) {}
    
    bool IsExpired(int64_t nCurrentTime) const {
        return nCurrentTime >= nLockTime;
    }
    
    SERIALIZE_METHODS(CStakeLock, obj) {
        READWRITE(obj.outpoint, obj.nLockTime, obj.nAmount, obj.ownerPubKey);
    }
    
    std::string ToString() const;
};

/**
 * Stake input structure
 */
struct CStakeInput {
    COutPoint outpoint;
    CAmount nValue;
    int64_t nTime;
    CPubKey pubkey;
    
    CStakeInput() : nValue(0), nTime(0) {}
    
    SERIALIZE_METHODS(CStakeInput, obj) {
        READWRITE(obj.outpoint, obj.nValue, obj.nTime, obj.pubkey);
    }
};

/**
 * Validator structure
 */
struct CValidator {
    CPubKey pubkey;
    CAmount nStakeAmount;
    int64_t nRegistrationTime;
    bool fActive;
    
    CValidator() : nStakeAmount(0), nRegistrationTime(0), fActive(false) {}
    
    SERIALIZE_METHODS(CValidator, obj) {
        READWRITE(obj.pubkey, obj.nStakeAmount, obj.nRegistrationTime, obj.fActive);
    }
};

/**
 * Slashing conditions enumeration
 */
enum SlashingCondition {
    SLASH_DOUBLE_SIGNING = 1,       // Validator signed two conflicting blocks
    SLASH_LONG_RANGE_ATTACK = 2,    // Validator participated in long-range attack
    SLASH_UNAVAILABILITY = 3,       // Validator was offline for extended period
    SLASH_INVALID_BLOCK = 4,        // Validator produced invalid block
    SLASH_EQUIVOCATION = 5          // Validator sent conflicting messages
};

/**
 * Slashing evidence structure
 */
struct SlashingEvidence {
    CPubKey validatorPubKey;        // Validator being slashed
    SlashingCondition condition;    // Type of slashing condition
    int64_t nTime;                  // Time of the offense
    uint256 blockHash1;             // First conflicting block (if applicable)
    uint256 blockHash2;             // Second conflicting block (if applicable)
    std::vector<uint8_t> evidence;  // Additional evidence data
    
    SlashingEvidence() : condition(SLASH_DOUBLE_SIGNING), nTime(0) {}
    
    SERIALIZE_METHODS(SlashingEvidence, obj) {
        READWRITE(obj.validatorPubKey, obj.condition, obj.nTime, obj.blockHash1, obj.blockHash2, obj.evidence);
    }
    
    std::string ToString() const;
};

// Global validator registry
extern std::map<CPubKey, CValidator> mapValidators;
extern std::map<CPubKey, bool> mapValidatorStatus;
extern std::map<COutPoint, CStakeLock> mapStakeLocks;
extern std::set<CPubKey> setBlacklistedValidators;

// Core PoS functions
bool CheckProofOfStake(const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& params, CCoinsViewCache& view);
CAmount CalculateStakeReward(const CAmount& stakeAmount, int64_t stakeDuration, const Consensus::Params& consensusParams);
bool DistributeStakingRewards(const std::vector<CStakeInput>& validators, CAmount totalReward, std::vector<CAmount>& rewards);

// Validation functions
bool ValidateStakingTransaction(const CTransaction& tx, const Consensus::Params& consensusParams);
bool ValidateStakeTransaction(const CTransaction& tx, const CStakeTransactionPayload& payload, const Consensus::Params& consensusParams);
bool ValidateUnstakeTransaction(const CTransaction& tx, const CUnstakeTransactionPayload& payload, const Consensus::Params& consensusParams);
bool ValidateValidatorRegistration(const CTransaction& tx, const CValidatorRegistrationPayload& payload, const Consensus::Params& consensusParams);

// Processing functions
bool ProcessValidatorRegistration(const CValidatorRegistrationPayload& payload);

// Stake lock management
bool CreateStakeLock(const COutPoint& outpoint, CAmount nAmount, int64_t nLockTime, const CPubKey& ownerPubKey);
bool RemoveStakeLock(const COutPoint& outpoint);
bool IsStakeLocked(const COutPoint& outpoint);
bool GetStakeLock(const COutPoint& outpoint, CStakeLock& stakeLock);

// Validator management
bool RegisterValidator(const CPubKey& validatorPubKey, CAmount nStakeAmount, const CScript& rewardScript, const std::string& strDescription);
bool UpdateValidatorStake(const CPubKey& validatorPubKey, CAmount nNewStakeAmount);
bool SetValidatorStatus(const CPubKey& validatorPubKey, bool fActive);
bool GetValidator(const CPubKey& validatorPubKey, CValidator& validator);
bool IsValidatorRegistered(const CPubKey& validatorPubKey);
bool IsValidatorActive(const CPubKey& validatorPubKey);
bool RemoveValidator(const CPubKey& validatorPubKey);
size_t GetValidatorCount();
size_t GetActiveValidatorCount();

// Supply management
CAmount GetTotalSupply(int nHeight, const Consensus::Params& consensusParams);
CAmount GetCirculatingSupply(int nHeight, const Consensus::Params& consensusParams);
CAmount GetTotalActiveStake();
CAmount GetRemainingStakingSupply(int nHeight, const Consensus::Params& consensusParams);

// Slashing system
CAmount CalculateSlashingPenalty(const CPubKey& validatorPubKey, SlashingCondition condition);
bool SlashValidator(const CPubKey& validatorPubKey, SlashingCondition condition, const SlashingEvidence& evidence);
bool ReportDoubleSign(const CPubKey& validatorPubKey, int nHeight, const uint256& blockHash1, const uint256& blockHash2);
bool ReportUnavailability(const CPubKey& validatorPubKey, int nMissedBlocks, int nTotalBlocks);
bool IsValidatorBlacklisted(const CPubKey& validatorPubKey);
bool RemoveFromBlacklist(const CPubKey& validatorPubKey);

#endif // BABACHAIN_POS_H