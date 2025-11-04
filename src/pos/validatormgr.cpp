// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pos/validatormgr.h>

#include <chainparams.h>
#include <coins.h>
#include <consensus/validation.h>
#include <hash.h>
#include <key_io.h>
#include <logging.h>
#include <script/script.h>
#include <script/standard.h>
#include <util/system.h>
#include <validation.h>

#include <boost/filesystem.hpp>

std::string CValidatorRegTx::ToString() const
{
    return strprintf("CValidatorRegTx(nVersion=%d, validatorPubKey=%s, stakeOutPoint=%s, nStakeAmount=%s)",
                     nVersion, HexStr(validatorPubKey), stakeOutPoint.ToString(), FormatMoney(nStakeAmount));
}

std::string CValidatorDeregTx::ToString() const
{
    return strprintf("CValidatorDeregTx(nVersion=%d, validatorPubKey=%s)",
                     nVersion, HexStr(validatorPubKey));
}

CValidatorManager::CValidatorManager()
{
}

CValidatorManager::~CValidatorManager()
{
}

bool CValidatorManager::Init(const fs::path& datadir)
{
    LOCK(cs_db);
    
    fs::path dbPath = datadir / "validators";
    
    try {
        m_db = std::make_unique<CDBWrapper>(dbPath, 1 << 20, false, false, false);
    } catch (const std::exception& e) {
        LogPrintf("CValidatorManager::%s: Failed to initialize database: %s\n", __func__, e.what());
        return false;
    }
    
    LogPrintf("CValidatorManager::%s: Initialized validator database at %s\n", __func__, dbPath.string());
    return true;
}

bool CValidatorManager::LoadValidators()
{
    LOCK(cs_db);
    
    if (!m_db) {
        LogPrintf("CValidatorManager::%s: Database not initialized\n", __func__);
        return false;
    }
    
    // Clear existing validators
    registry.Clear();
    
    // Iterate through all validators in database
    std::unique_ptr<CDBIterator> pcursor(m_db->NewIterator());
    pcursor->Seek(std::make_pair(DB_VALIDATOR, uint256()));
    
    int loadedCount = 0;
    while (pcursor->Valid()) {
        std::pair<char, uint256> key;
        if (!pcursor->GetKey(key) || key.first != DB_VALIDATOR) {
            break;
        }
        
        CValidator validator;
        if (!pcursor->GetValue(validator)) {
            LogPrintf("CValidatorManager::%s: Failed to deserialize validator %s\n", 
                     __func__, key.second.ToString());
            pcursor->Next();
            continue;
        }
        
        // Add validator to registry
        if (registry.RegisterValidator(validator)) {
            loadedCount++;
        } else {
            LogPrintf("CValidatorManager::%s: Failed to register loaded validator %s\n", 
                     __func__, key.second.ToString());
        }
        
        pcursor->Next();
    }
    
    LogPrintf("CValidatorManager::%s: Loaded %d validators from database\n", __func__, loadedCount);
    return true;
}

bool CValidatorManager::SaveValidator(const CValidator& validator)
{
    LOCK(cs_db);
    
    if (!m_db) {
        LogPrintf("CValidatorManager::%s: Database not initialized\n", __func__);
        return false;
    }
    
    uint256 validatorHash = Hash(validator.pubkey.begin(), validator.pubkey.end());
    
    // Save validator
    if (!m_db->Write(std::make_pair(DB_VALIDATOR, validatorHash), validator)) {
        LogPrintf("CValidatorManager::%s: Failed to write validator %s to database\n", 
                 __func__, validatorHash.ToString());
        return false;
    }
    
    // Save stake mapping
    if (!m_db->Write(std::make_pair(DB_VALIDATOR_BY_STAKE, validator.stakeInput.prevout), validatorHash)) {
        LogPrintf("CValidatorManager::%s: Failed to write stake mapping for validator %s\n", 
                 __func__, validatorHash.ToString());
        return false;
    }
    
    LogPrint(BCLog::POS, "CValidatorManager::%s: Saved validator %s to database\n", 
             __func__, validatorHash.ToString());
    
    return true;
}

bool CValidatorManager::RemoveValidator(const uint256& validatorHash)
{
    LOCK(cs_db);
    
    if (!m_db) {
        LogPrintf("CValidatorManager::%s: Database not initialized\n", __func__);
        return false;
    }
    
    // Get validator to find stake outpoint
    CValidator validator;
    if (!m_db->Read(std::make_pair(DB_VALIDATOR, validatorHash), validator)) {
        LogPrintf("CValidatorManager::%s: Validator %s not found in database\n", 
                 __func__, validatorHash.ToString());
        return false;
    }
    
    // Remove validator
    if (!m_db->Erase(std::make_pair(DB_VALIDATOR, validatorHash))) {
        LogPrintf("CValidatorManager::%s: Failed to erase validator %s from database\n", 
                 __func__, validatorHash.ToString());
        return false;
    }
    
    // Remove stake mapping
    if (!m_db->Erase(std::make_pair(DB_VALIDATOR_BY_STAKE, validator.stakeInput.prevout))) {
        LogPrintf("CValidatorManager::%s: Failed to erase stake mapping for validator %s\n", 
                 __func__, validatorHash.ToString());
        return false;
    }
    
    LogPrint(BCLog::POS, "CValidatorManager::%s: Removed validator %s from database\n", 
             __func__, validatorHash.ToString());
    
    return true;
}

bool CValidatorManager::ProcessValidatorRegTx(const CTransaction& tx, const CValidatorRegTx& regTx,
                                             const CBlockIndex* pindex, const CCoinsViewCache& view,
                                             TxValidationState& state)
{
    // Check if already processed
    if (IsTransactionProcessed(tx.GetHash())) {
        LogPrint(BCLog::POS, "CValidatorManager::%s: Transaction %s already processed\n", 
                 __func__, tx.GetHash().ToString());
        return true;
    }
    
    // Validate the registration transaction
    if (!ValidateValidatorRegTx(regTx, view, state)) {
        return false;
    }
    
    // Create validator object
    CStakeInput stakeInput(regTx.stakeOutPoint, regTx.nStakeAmount, pindex->GetBlockTime(), pindex->GetBlockHash());
    CValidator validator(regTx.validatorPubKey, regTx.blsPubKey, stakeInput);
    
    // Register validator in memory
    uint256 validatorHash = Hash(validator.pubkey.begin(), validator.pubkey.end());
    if (!registry.RegisterValidator(validator)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "validator-reg-failed", 
                           "Failed to register validator in registry");
    }
    
    // Save to database
    if (!SaveValidator(validator)) {
        // Rollback memory registration
        registry.DeregisterValidator(validatorHash);
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "validator-db-save-failed", 
                           "Failed to save validator to database");
    }
    
    // Mark transaction as processed
    MarkTransactionProcessed(tx.GetHash());
    
    LogPrintf("CValidatorManager::%s: Registered validator %s with stake %s\n", 
             __func__, validatorHash.ToString(), FormatMoney(regTx.nStakeAmount));
    
    return true;
}

bool CValidatorManager::ProcessValidatorDeregTx(const CTransaction& tx, const CValidatorDeregTx& deregTx,
                                               const CBlockIndex* pindex, TxValidationState& state)
{
    // Check if already processed
    if (IsTransactionProcessed(tx.GetHash())) {
        LogPrint(BCLog::POS, "CValidatorManager::%s: Transaction %s already processed\n", 
                 __func__, tx.GetHash().ToString());
        return true;
    }
    
    // Validate the deregistration transaction
    if (!ValidateValidatorDeregTx(deregTx, state)) {
        return false;
    }
    
    uint256 validatorHash = Hash(deregTx.validatorPubKey.begin(), deregTx.validatorPubKey.end());
    
    // Check if validator exists
    if (!registry.HasValidator(validatorHash)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "validator-not-found", 
                           "Validator not found in registry");
    }
    
    // Deregister validator from memory
    if (!registry.DeregisterValidator(validatorHash)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "validator-dereg-failed", 
                           "Failed to deregister validator from registry");
    }
    
    // Remove from database
    if (!RemoveValidator(validatorHash)) {
        // This is not a consensus failure, just log the error
        LogPrintf("CValidatorManager::%s: Failed to remove validator %s from database\n", 
                 __func__, validatorHash.ToString());
    }
    
    // Mark transaction as processed
    MarkTransactionProcessed(tx.GetHash());
    
    LogPrintf("CValidatorManager::%s: Deregistered validator %s\n", 
             __func__, validatorHash.ToString());
    
    return true;
}

bool CValidatorManager::UndoValidatorRegTx(const CTransaction& tx, const CValidatorRegTx& regTx,
                                          const CBlockIndex* pindex)
{
    uint256 validatorHash = Hash(regTx.validatorPubKey.begin(), regTx.validatorPubKey.end());
    
    // Remove from memory registry
    registry.DeregisterValidator(validatorHash);
    
    // Remove from database
    RemoveValidator(validatorHash);
    
    LogPrint(BCLog::POS, "CValidatorManager::%s: Undid validator registration %s\n", 
             __func__, validatorHash.ToString());
    
    return true;
}

bool CValidatorManager::UndoValidatorDeregTx(const CTransaction& tx, const CValidatorDeregTx& deregTx,
                                            const CBlockIndex* pindex)
{
    // For undo, we would need to restore the validator state
    // This is complex and would require storing the previous state
    // For now, we'll reload from database on reorg
    LogPrint(BCLog::POS, "CValidatorManager::%s: Undid validator deregistration (reload required)\n", __func__);
    
    return LoadValidators();
}

bool CValidatorManager::ValidateValidatorRegTx(const CValidatorRegTx& regTx, const CCoinsViewCache& view,
                                              TxValidationState& state) const
{
    // Check version
    if (regTx.nVersion != CValidatorRegTx::CURRENT_VERSION) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "validator-reg-version", 
                           "Invalid validator registration version");
    }
    
    // Check minimum stake amount
    if (regTx.nStakeAmount < MIN_VALIDATOR_STAKE) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "validator-insufficient-stake", 
                           strprintf("Stake amount %s below minimum %s", 
                                   FormatMoney(regTx.nStakeAmount), FormatMoney(MIN_VALIDATOR_STAKE)));
    }
    
    // Check if validator already exists
    uint256 validatorHash = Hash(regTx.validatorPubKey.begin(), regTx.validatorPubKey.end());
    if (registry.HasValidator(validatorHash)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "validator-already-exists", 
                           "Validator already registered");
    }
    
    // Check if stake input is already used
    if (registry.IsStakeUsed(regTx.stakeOutPoint)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "stake-already-used", 
                           "Stake input already used by another validator");
    }
    
    // Verify stake UTXO exists and has correct amount
    Coin coin;
    if (!view.GetCoin(regTx.stakeOutPoint, coin) || coin.IsSpent()) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "stake-utxo-not-found", 
                           "Stake UTXO not found or already spent");
    }
    
    if (coin.out.nValue != regTx.nStakeAmount) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "stake-amount-mismatch", 
                           strprintf("Stake UTXO amount %s does not match claimed amount %s",
                                   FormatMoney(coin.out.nValue), FormatMoney(regTx.nStakeAmount)));
    }
    
    // Verify signature
    if (!VerifyValidatorRegSignature(regTx)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "validator-reg-signature-invalid", 
                           "Invalid validator registration signature");
    }
    
    return true;
}

bool CValidatorManager::ValidateValidatorDeregTx(const CValidatorDeregTx& deregTx, TxValidationState& state) const
{
    // Check version
    if (deregTx.nVersion != CValidatorDeregTx::CURRENT_VERSION) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "validator-dereg-version", 
                           "Invalid validator deregistration version");
    }
    
    // Check if validator exists
    uint256 validatorHash = Hash(deregTx.validatorPubKey.begin(), deregTx.validatorPubKey.end());
    if (!registry.HasValidator(validatorHash)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "validator-not-found", 
                           "Validator not found in registry");
    }
    
    // Verify signature
    if (!VerifyValidatorDeregSignature(deregTx)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "validator-dereg-signature-invalid", 
                           "Invalid validator deregistration signature");
    }
    
    return true;
}

bool CValidatorManager::IsTransactionProcessed(const uint256& txHash) const
{
    LOCK(cs_tx_cache);
    return processedTxs.find(txHash) != processedTxs.end();
}

void CValidatorManager::MarkTransactionProcessed(const uint256& txHash)
{
    LOCK(cs_tx_cache);
    processedTxs.insert(txHash);
}

void CValidatorManager::ClearProcessedTransactions()
{
    LOCK(cs_tx_cache);
    processedTxs.clear();
}

bool CValidatorManager::VerifyValidatorRegSignature(const CValidatorRegTx& regTx) const
{
    // Create message to sign (hash of registration data without signature)
    CValidatorRegTx regTxCopy = regTx;
    regTxCopy.vchSig.clear();
    uint256 messageHash = regTxCopy.GetHash();
    
    // Verify signature
    return regTx.validatorPubKey.VerifyECDSA(messageHash, regTx.vchSig);
}

bool CValidatorManager::VerifyValidatorDeregSignature(const CValidatorDeregTx& deregTx) const
{
    // Create message to sign (hash of deregistration data without signature)
    CValidatorDeregTx deregTxCopy = deregTx;
    deregTxCopy.vchSig.clear();
    uint256 messageHash = deregTxCopy.GetHash();
    
    // Verify signature
    return deregTx.validatorPubKey.VerifyECDSA(messageHash, deregTx.vchSig);
}

std::string CValidatorManager::GetValidatorKey(const uint256& validatorHash) const
{
    return strprintf("%c%s", DB_VALIDATOR, validatorHash.ToString());
}

std::string CValidatorManager::GetStakeKey(const COutPoint& stakeOutPoint) const
{
    return strprintf("%c%s", DB_VALIDATOR_BY_STAKE, stakeOutPoint.ToString());
}

bool GetValidatorRegTx(const CTransaction& tx, CValidatorRegTx& regTx)
{
    // Look for OP_RETURN output with validator registration data
    for (const auto& output : tx.vout) {
        if (output.scriptPubKey.size() > 1 && output.scriptPubKey[0] == OP_RETURN) {
            try {
                CDataStream ss(std::vector<unsigned char>(output.scriptPubKey.begin() + 2, output.scriptPubKey.end()), 
                              SER_NETWORK, PROTOCOL_VERSION);
                
                // Check for validator registration marker
                std::string marker;
                ss >> marker;
                if (marker == "VALREG") {
                    ss >> regTx;
                    return true;
                }
            } catch (const std::exception&) {
                // Invalid data, continue to next output
                continue;
            }
        }
    }
    
    return false;
}

bool GetValidatorDeregTx(const CTransaction& tx, CValidatorDeregTx& deregTx)
{
    // Look for OP_RETURN output with validator deregistration data
    for (const auto& output : tx.vout) {
        if (output.scriptPubKey.size() > 1 && output.scriptPubKey[0] == OP_RETURN) {
            try {
                CDataStream ss(std::vector<unsigned char>(output.scriptPubKey.begin() + 2, output.scriptPubKey.end()), 
                              SER_NETWORK, PROTOCOL_VERSION);
                
                // Check for validator deregistration marker
                std::string marker;
                ss >> marker;
                if (marker == "VALDEREG") {
                    ss >> deregTx;
                    return true;
                }
            } catch (const std::exception&) {
                // Invalid data, continue to next output
                continue;
            }
        }
    }
    
    return false;
}

bool IsValidatorRegTx(const CTransaction& tx)
{
    CValidatorRegTx regTx;
    return GetValidatorRegTx(tx, regTx);
}

bool IsValidatorDeregTx(const CTransaction& tx)
{
    CValidatorDeregTx deregTx;
    return GetValidatorDeregTx(tx, deregTx);
}