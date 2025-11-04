// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POS_VALIDATOR_H
#define BITCOIN_POS_VALIDATOR_H

#include <amount.h>
#include <arith_uint256.h>
#include <bls/bls.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <serialize.h>
#include <sync.h>
#include <uint256.h>

#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <vector>

class CBlockIndex;
class CCoinsViewCache;

/** Minimum stake amount required to become a validator */
static constexpr CAmount MIN_VALIDATOR_STAKE = 1000 * COIN;

/** Minimum age for stake to be eligible for validation (in seconds) */
static constexpr int64_t MIN_STAKE_AGE = 8 * 60 * 60; // 8 hours

/** Maximum age for stake to be eligible for validation (in seconds) */
static constexpr int64_t MAX_STAKE_AGE = 30 * 24 * 60 * 60; // 30 days

/** Validator status enumeration */
enum class ValidatorStatus : uint8_t {
    INACTIVE = 0,
    ACTIVE = 1,
    SLASHED = 2,
    DEREGISTERED = 3
};

/**
 * Represents a stake input used for validator selection
 */
class CStakeInput
{
public:
    COutPoint prevout;          //!< Previous output being staked
    CAmount nValue;             //!< Amount being staked
    int64_t nTime;              //!< Stake time
    uint256 hashBlock;          //!< Block hash where stake originates
    
    CStakeInput() : nValue(0), nTime(0) {}
    
    CStakeInput(const COutPoint& _prevout, CAmount _nValue, int64_t _nTime, const uint256& _hashBlock)
        : prevout(_prevout), nValue(_nValue), nTime(_nTime), hashBlock(_hashBlock) {}

    SERIALIZE_METHODS(CStakeInput, obj) {
        READWRITE(obj.prevout, obj.nValue, obj.nTime, obj.hashBlock);
    }

    bool IsValid() const {
        return !prevout.IsNull() && nValue >= MIN_VALIDATOR_STAKE && nTime > 0;
    }

    int64_t GetAge(int64_t currentTime) const {
        return currentTime - nTime;
    }

    bool IsEligible(int64_t currentTime) const {
        int64_t age = GetAge(currentTime);
        return age >= MIN_STAKE_AGE && age <= MAX_STAKE_AGE;
    }
};

/**
 * Represents a validator in the PoS system
 */
class CValidator
{
public:
    CPubKey pubkey;                 //!< Validator public key
    CBLSPublicKey blsPubkey;        //!< BLS public key for signing
    CAmount nStakeAmount;           //!< Total staked amount
    int64_t nRegistrationTime;      //!< When validator registered
    ValidatorStatus status;         //!< Current validator status
    CStakeInput stakeInput;         //!< Primary stake input
    int nSlashingCount;             //!< Number of times slashed
    int64_t nLastActiveTime;        //!< Last time validator was active
    
    CValidator() : nStakeAmount(0), nRegistrationTime(0), status(ValidatorStatus::INACTIVE), 
                   nSlashingCount(0), nLastActiveTime(0) {}
    
    CValidator(const CPubKey& _pubkey, const CBLSPublicKey& _blsPubkey, const CStakeInput& _stakeInput)
        : pubkey(_pubkey), blsPubkey(_blsPubkey), nStakeAmount(_stakeInput.nValue), 
          nRegistrationTime(GetTime()), status(ValidatorStatus::ACTIVE), 
          stakeInput(_stakeInput), nSlashingCount(0), nLastActiveTime(GetTime()) {}

    SERIALIZE_METHODS(CValidator, obj) {
        READWRITE(obj.pubkey, obj.blsPubkey, obj.nStakeAmount, obj.nRegistrationTime);
        READWRITE(obj.status, obj.stakeInput, obj.nSlashingCount, obj.nLastActiveTime);
    }

    bool IsActive() const {
        return status == ValidatorStatus::ACTIVE;
    }

    bool IsSlashed() const {
        return status == ValidatorStatus::SLASHED;
    }

    bool IsEligible(int64_t currentTime) const {
        return IsActive() && stakeInput.IsEligible(currentTime);
    }

    uint256 GetHash() const {
        return SerializeHash(*this);
    }
};

using CValidatorPtr = std::shared_ptr<CValidator>;

/**
 * Manages the validator registry and selection for PoS consensus
 */
class CValidatorRegistry
{
private:
    mutable Mutex cs_validators;
    
    //! Map of validator pubkey hash to validator
    std::map<uint256, CValidatorPtr> mapValidators GUARDED_BY(cs_validators);
    
    //! Map of stake input to validator pubkey hash
    std::map<COutPoint, uint256> mapStakeToValidator GUARDED_BY(cs_validators);
    
    //! Set of active validators for quick lookup
    std::set<uint256> setActiveValidators GUARDED_BY(cs_validators);

public:
    CValidatorRegistry() = default;
    
    /**
     * Register a new validator
     */
    bool RegisterValidator(const CValidator& validator) EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Deregister a validator
     */
    bool DeregisterValidator(const uint256& validatorHash) EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Update validator stake amount
     */
    bool UpdateValidatorStake(const uint256& validatorHash, const CStakeInput& newStake) EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Slash a validator for malicious behavior
     */
    bool SlashValidator(const uint256& validatorHash, const std::string& reason) EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Get validator by pubkey hash
     */
    CValidatorPtr GetValidator(const uint256& validatorHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Get validator by stake input
     */
    CValidatorPtr GetValidatorByStake(const COutPoint& stakeInput) const EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Get all active validators
     */
    std::vector<CValidatorPtr> GetActiveValidators() const EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Get eligible validators for current time
     */
    std::vector<CValidatorPtr> GetEligibleValidators(int64_t currentTime) const EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Get total stake amount of all active validators
     */
    CAmount GetTotalActiveStake() const EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Check if validator exists
     */
    bool HasValidator(const uint256& validatorHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Check if stake input is already used
     */
    bool IsStakeUsed(const COutPoint& stakeInput) const EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Get validator count
     */
    size_t GetValidatorCount() const EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Get active validator count
     */
    size_t GetActiveValidatorCount() const EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);
    
    /**
     * Clear all validators (for testing)
     */
    void Clear() EXCLUSIVE_LOCKS_REQUIRED(!cs_validators);

private:
    uint256 GetValidatorHash(const CPubKey& pubkey) const {
        return Hash(pubkey.begin(), pubkey.end());
    }
};

/**
 * Handles validator selection for block production using stake-weighted randomization
 */
class CValidatorSelector
{
private:
    const CValidatorRegistry& registry;
    
public:
    explicit CValidatorSelector(const CValidatorRegistry& _registry) : registry(_registry) {}
    
    /**
     * Select next validator for block production based on stake weight and randomization
     * @param blockHash Hash of the previous block (used for randomization)
     * @param currentTime Current timestamp
     * @return Selected validator, or nullptr if no eligible validators
     */
    CValidatorPtr SelectValidator(const uint256& blockHash, int64_t currentTime) const;
    
    /**
     * Calculate validator selection probability based on stake weight
     * @param validator The validator to calculate probability for
     * @param totalStake Total stake of all eligible validators
     * @return Probability value between 0.0 and 1.0
     */
    double CalculateSelectionProbability(const CValidator& validator, CAmount totalStake) const;
    
    /**
     * Verify that a validator was correctly selected for a given block
     * @param validator The validator that produced the block
     * @param blockHash Hash of the previous block
     * @param currentTime Timestamp when block was produced
     * @return True if validator selection is valid
     */
    bool VerifyValidatorSelection(const CValidator& validator, const uint256& blockHash, int64_t currentTime) const;

private:
    /**
     * Generate deterministic random value from block hash and validator pubkey
     */
    arith_uint256 GetSelectionHash(const uint256& blockHash, const CPubKey& validatorPubkey) const;
};

#endif // BITCOIN_POS_VALIDATOR_H