// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POS_H
#define BITCOIN_POS_H

#include <consensus/params.h>
#include <consensus/amount.h>
#include <key.h>
#include <primitives/transaction.h>
#include <primitives/block.h>
#include <pubkey.h>
#include <uint256.h>
#include <serialize.h>

#include <stdint.h>
#include <vector>

class CBlockIndex;
class CCoinsViewCache;
class BlockValidationState;

/** Proof of Stake validation results */
enum class PoSValidationResult {
    VALID,
    INVALID_STAKE_AMOUNT,
    INVALID_STAKE_AGE,
    INVALID_SIGNATURE,
    DOUBLE_SPEND_ATTEMPT,
    INSUFFICIENT_STAKE,
    VALIDATOR_NOT_REGISTERED,
    INVALID_BLOCK_TIME,
    INVALID_STAKE_MODIFIER
};

/** Stake input structure for PoS transactions */
class CStakeInput
{
public:
    COutPoint prevout;          // Previous output being staked
    CAmount nValue;             // Amount being staked
    int64_t nTime;              // Stake time
    uint256 hashBlock;          // Block hash where stake originates
    
    CStakeInput()
    {
        SetNull();
    }
    
    CStakeInput(const COutPoint& prevoutIn, CAmount nValueIn, int64_t nTimeIn, const uint256& hashBlockIn)
        : prevout(prevoutIn), nValue(nValueIn), nTime(nTimeIn), hashBlock(hashBlockIn) {}
    
    SERIALIZE_METHODS(CStakeInput, obj) {
        READWRITE(obj.prevout, obj.nValue, obj.nTime, obj.hashBlock);
    }
    
    void SetNull()
    {
        prevout.SetNull();
        nValue = 0;
        nTime = 0;
        hashBlock.SetNull();
    }
    
    bool IsNull() const
    {
        return prevout.IsNull() && nValue == 0;
    }
    
    std::string ToString() const;
};

/** Stake output structure for PoS rewards */
class CStakeOutput
{
public:
    CScript scriptPubKey;       // Destination script
    CAmount nValue;             // Stake reward amount
    
    CStakeOutput()
    {
        SetNull();
    }
    
    CStakeOutput(const CScript& scriptPubKeyIn, CAmount nValueIn)
        : scriptPubKey(scriptPubKeyIn), nValue(nValueIn) {}
    
    SERIALIZE_METHODS(CStakeOutput, obj) {
        READWRITE(obj.scriptPubKey, obj.nValue);
    }
    
    void SetNull()
    {
        scriptPubKey.clear();
        nValue = 0;
    }
    
    bool IsNull() const
    {
        return scriptPubKey.empty() && nValue == 0;
    }
    
    std::string ToString() const;
};

/** Validator information structure */
class CValidator
{
public:
    CPubKey pubkey;             // Validator public key
    CAmount nStakeAmount;       // Total staked amount
    int64_t nRegistrationTime;  // When validator registered
    bool fActive;               // Validator status
    
    CValidator()
    {
        SetNull();
    }
    
    CValidator(const CPubKey& pubkeyIn, CAmount nStakeAmountIn, int64_t nRegistrationTimeIn, bool fActiveIn)
        : pubkey(pubkeyIn), nStakeAmount(nStakeAmountIn), nRegistrationTime(nRegistrationTimeIn), fActive(fActiveIn) {}
    
    SERIALIZE_METHODS(CValidator, obj) {
        READWRITE(obj.pubkey, obj.nStakeAmount, obj.nRegistrationTime, obj.fActive);
    }
    
    void SetNull()
    {
        pubkey = CPubKey();
        nStakeAmount = 0;
        nRegistrationTime = 0;
        fActive = false;
    }
    
    bool IsNull() const
    {
        return !pubkey.IsValid() && nStakeAmount == 0;
    }
    
    std::string ToString() const;
};

/** Main PoS validator class */
class CPoSValidator
{
public:
    CPoSValidator() = default;
    
    /** Validate a stake input for PoS consensus */
    PoSValidationResult ValidateStake(const CBlockIndex* pindexPrev, const CBlock& block, const CStakeInput& stakeInput, const CCoinsViewCache& view) const;
    
    /** Calculate stake reward based on amount and duration */
    CAmount CalculateStakeReward(const CAmount& stakeAmount, int64_t stakeDuration, const Consensus::Params& params) const;
    
    /** Select next validator based on stake weight and randomization */
    bool SelectNextValidator(const std::vector<CValidator>& validators, const uint256& blockHash, CValidator& selectedValidator) const;
    
    /** Validate PoS block according to consensus rules */
    bool ValidatePoSBlock(const CBlock& block, const CBlockIndex* pindexPrev, BlockValidationState& state, const Consensus::Params& params, const CCoinsViewCache& view) const;
    
    /** Check if stake meets minimum age requirements */
    bool CheckStakeAge(const CStakeInput& stakeInput, const CBlockIndex* pindexPrev, const Consensus::Params& params) const;
    
    /** Check if stake amount meets minimum requirements */
    bool CheckStakeAmount(const CStakeInput& stakeInput, const Consensus::Params& params) const;
    
    /** Calculate stake modifier for randomization */
    uint256 CalculateStakeModifier(const CBlockIndex* pindexPrev, const CStakeInput& stakeInput) const;
    
    /** Verify stake signature */
    bool VerifyStakeSignature(const CBlock& block, const CStakeInput& stakeInput, const CCoinsViewCache& view) const;
    
private:
    /** Internal helper to calculate stake weight */
    uint64_t CalculateStakeWeight(const CAmount& stakeAmount, int64_t stakeAge, const Consensus::Params& params) const;
    
    /** Internal helper to check stake kernel */
    bool CheckStakeKernel(const CStakeInput& stakeInput, const uint256& stakeModifier, const CBlockIndex* pindexPrev, const Consensus::Params& params) const;
};

/** Check whether a block satisfies the proof-of-stake requirement */
bool CheckProofOfStake(const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& params, const CCoinsViewCache& view);

/** Get stake modifier for a given block index */
uint256 GetStakeModifier(const CBlockIndex* pindex);

/** Create a new PoS block using the given stake input */
bool CreatePoSBlock(CBlock& block, const CBlockIndex* pindexPrev, const CStakeInput& stakeInput, const CKey& key, const Consensus::Params& params);

/** Sign a PoS block with the staking key */
bool SignPoSBlock(CBlock& block, const CKey& key);

#endif // BITCOIN_POS_H