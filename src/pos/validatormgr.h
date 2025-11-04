// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POS_VALIDATORMGR_H
#define BITCOIN_POS_VALIDATORMGR_H

#include <pos/validator.h>

#include <dbwrapper.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <uint256.h>

#include <memory>

class CBlockIndex;
class CCoinsViewCache;
class TxValidationState;

/** Database key prefixes for validator storage */
static constexpr char DB_VALIDATOR = 'V';
static constexpr char DB_VALIDATOR_BY_STAKE = 'S';
static constexpr char DB_VALIDATOR_COUNT = 'C';

/**
 * Validator registration transaction
 * Contains the information needed to register a new validator
 */
class CValidatorRegTx
{
public:
    static constexpr uint16_t CURRENT_VERSION = 1;
    
    uint16_t nVersion{CURRENT_VERSION};
    CPubKey validatorPubKey;        //!< Validator's public key
    CBLSPublicKey blsPubKey;        //!< BLS public key for consensus
    COutPoint stakeOutPoint;        //!< Outpoint of the stake UTXO
    CAmount nStakeAmount;           //!< Amount being staked
    CScript payoutScript;           //!< Script for reward payouts
    std::vector<unsigned char> vchSig; //!< Signature proving ownership
    
    CValidatorRegTx() : nStakeAmount(0) {}

    SERIALIZE_METHODS(CValidatorRegTx, obj) {
        READWRITE(obj.nVersion, obj.validatorPubKey, obj.blsPubKey);
        READWRITE(obj.stakeOutPoint, obj.nStakeAmount, obj.payoutScript, obj.vchSig);
    }

    uint256 GetHash() const {
        return SerializeHash(*this);
    }

    std::string ToString() const;
};

/**
 * Validator deregistration transaction
 * Used to remove a validator from the active set
 */
class CValidatorDeregTx
{
public:
    static constexpr uint16_t CURRENT_VERSION = 1;
    
    uint16_t nVersion{CURRENT_VERSION};
    CPubKey validatorPubKey;        //!< Validator's public key
    std::vector<unsigned char> vchSig; //!< Signature proving ownership
    
    CValidatorDeregTx() = default;

    SERIALIZE_METHODS(CValidatorDeregTx, obj) {
        READWRITE(obj.nVersion, obj.validatorPubKey, obj.vchSig);
    }

    uint256 GetHash() const {
        return SerializeHash(*this);
    }

    std::string ToString() const;
};

/**
 * Manages validator registration, deregistration, and persistence
 */
class CValidatorManager
{
private:
    mutable Mutex cs_db;
    std::unique_ptr<CDBWrapper> m_db GUARDED_BY(cs_db);
    CValidatorRegistry registry;
    
    //! Cache for recently processed transactions to avoid double processing
    mutable Mutex cs_tx_cache;
    std::set<uint256> processedTxs GUARDED_BY(cs_tx_cache);

public:
    CValidatorManager();
    ~CValidatorManager();
    
    //! Initialize the database
    bool Init(const fs::path& datadir);
    
    //! Load validators from database
    bool LoadValidators();
    
    //! Save validator to database
    bool SaveValidator(const CValidator& validator) EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    
    //! Remove validator from database
    bool RemoveValidator(const uint256& validatorHash) EXCLUSIVE_LOCKS_REQUIRED(!cs_db);
    
    //! Process validator registration transaction
    bool ProcessValidatorRegTx(const CTransaction& tx, const CValidatorRegTx& regTx, 
                              const CBlockIndex* pindex, const CCoinsViewCache& view,
                              TxValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !cs_tx_cache);
    
    //! Process validator deregistration transaction
    bool ProcessValidatorDeregTx(const CTransaction& tx, const CValidatorDeregTx& deregTx,
                                const CBlockIndex* pindex, TxValidationState& state) 
                                EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !cs_tx_cache);
    
    //! Undo validator registration (for block reorg)
    bool UndoValidatorRegTx(const CTransaction& tx, const CValidatorRegTx& regTx,
                           const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !cs_tx_cache);
    
    //! Undo validator deregistration (for block reorg)
    bool UndoValidatorDeregTx(const CTransaction& tx, const CValidatorDeregTx& deregTx,
                             const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !cs_tx_cache);
    
    //! Validate validator registration transaction
    bool ValidateValidatorRegTx(const CValidatorRegTx& regTx, const CCoinsViewCache& view,
                               TxValidationState& state) const;
    
    //! Validate validator deregistration transaction
    bool ValidateValidatorDeregTx(const CValidatorDeregTx& deregTx, TxValidationState& state) const;
    
    //! Get the validator registry (read-only access)
    const CValidatorRegistry& GetRegistry() const { return registry; }
    
    //! Get validator registry (for validator selection)
    CValidatorRegistry& GetRegistry() { return registry; }
    
    //! Check if transaction has been processed
    bool IsTransactionProcessed(const uint256& txHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_tx_cache);
    
    //! Mark transaction as processed
    void MarkTransactionProcessed(const uint256& txHash) EXCLUSIVE_LOCKS_REQUIRED(!cs_tx_cache);
    
    //! Clear processed transaction cache
    void ClearProcessedTransactions() EXCLUSIVE_LOCKS_REQUIRED(!cs_tx_cache);

private:
    //! Verify signature on validator registration
    bool VerifyValidatorRegSignature(const CValidatorRegTx& regTx) const;
    
    //! Verify signature on validator deregistration
    bool VerifyValidatorDeregSignature(const CValidatorDeregTx& deregTx) const;
    
    //! Get database key for validator
    std::string GetValidatorKey(const uint256& validatorHash) const;
    
    //! Get database key for stake mapping
    std::string GetStakeKey(const COutPoint& stakeOutPoint) const;
};

/**
 * Extract validator registration data from transaction
 * @param tx Transaction to examine
 * @param regTx Output parameter for registration data
 * @return True if transaction contains valid validator registration
 */
bool GetValidatorRegTx(const CTransaction& tx, CValidatorRegTx& regTx);

/**
 * Extract validator deregistration data from transaction
 * @param tx Transaction to examine
 * @param deregTx Output parameter for deregistration data
 * @return True if transaction contains valid validator deregistration
 */
bool GetValidatorDeregTx(const CTransaction& tx, CValidatorDeregTx& deregTx);

/**
 * Check if transaction is a validator registration transaction
 */
bool IsValidatorRegTx(const CTransaction& tx);

/**
 * Check if transaction is a validator deregistration transaction
 */
bool IsValidatorDeregTx(const CTransaction& tx);

#endif // BITCOIN_POS_VALIDATORMGR_H