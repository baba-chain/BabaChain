// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/staking.h>

#include <wallet/wallet.h>
#include <pos.h>
#include <validation.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <key_io.h>
#include <logging.h>
#include <streams.h>
#include <util/time.h>

namespace wallet {

/**
 * Get all staking transactions from the wallet
 */
std::vector<CStakingInfo> GetStakingTransactions(const CWallet& wallet)
{
    std::vector<CStakingInfo> stakingTxs;
    
    LOCK(wallet.cs_wallet);
    
    for (const auto& pair : wallet.mapWallet) {
        const CWalletTx& wtx = pair.second;
        const CTransaction& tx = *wtx.tx;
        
        // Check if this is a staking-related transaction
        if (tx.nType == TRANSACTION_STAKE || 
            tx.nType == TRANSACTION_UNSTAKE || 
            tx.nType == TRANSACTION_VALIDATOR_REGISTER) {
            
            CStakingInfo info;
            info.txid = tx.GetHash();
            info.nType = tx.nType;
            info.nTime = wtx.GetTxTime();
            info.nDepth = wallet.GetTxDepthInMainChain(wtx);
            
            // Parse transaction payload based on type
            if (tx.nType == TRANSACTION_STAKE && !tx.vExtraPayload.empty()) {
                try {
                    CDataStream ds(tx.vExtraPayload, SER_NETWORK, PROTOCOL_VERSION);
                    CStakeTransactionPayload payload;
                    ds >> payload;
                    
                    info.nAmount = payload.nStakeAmount;
                    info.nLockTime = payload.nLockTime;
                    info.stakePubKey = payload.stakePubKey;
                } catch (const std::exception& e) {
                    LogPrintf("GetStakingTransactions: Failed to parse stake payload: %s\n", e.what());
                    continue;
                }
            }
            else if (tx.nType == TRANSACTION_UNSTAKE && !tx.vExtraPayload.empty()) {
                try {
                    CDataStream ds(tx.vExtraPayload, SER_NETWORK, PROTOCOL_VERSION);
                    CUnstakeTransactionPayload payload;
                    ds >> payload;
                    
                    info.stakeOutpoint = payload.stakeOutpoint;
                    info.stakePubKey = payload.stakePubKey;
                    
                    // Get amount from the referenced stake
                    CStakeLock stakeLock;
                    if (GetStakeLock(payload.stakeOutpoint, stakeLock)) {
                        info.nAmount = stakeLock.nAmount;
                    }
                } catch (const std::exception& e) {
                    LogPrintf("GetStakingTransactions: Failed to parse unstake payload: %s\n", e.what());
                    continue;
                }
            }
            else if (tx.nType == TRANSACTION_VALIDATOR_REGISTER && !tx.vExtraPayload.empty()) {
                try {
                    CDataStream ds(tx.vExtraPayload, SER_NETWORK, PROTOCOL_VERSION);
                    CValidatorRegistrationPayload payload;
                    ds >> payload;
                    
                    info.nAmount = payload.nStakeAmount;
                    info.validatorPubKey = payload.validatorPubKey;
                    info.strDescription = payload.strDescription;
                } catch (const std::exception& e) {
                    LogPrintf("GetStakingTransactions: Failed to parse validator registration payload: %s\n", e.what());
                    continue;
                }
            }
            
            stakingTxs.push_back(info);
        }
    }
    
    return stakingTxs;
}

/**
 * Get all validators owned by this wallet
 */
std::vector<CValidatorInfo> GetWalletValidators(const CWallet& wallet)
{
    std::vector<CValidatorInfo> validators;
    
    LOCK(wallet.cs_wallet);
    
    // Get all validator registration transactions from this wallet
    for (const auto& pair : wallet.mapWallet) {
        const CWalletTx& wtx = pair.second;
        const CTransaction& tx = *wtx.tx;
        
        if (tx.nType == TRANSACTION_VALIDATOR_REGISTER && !tx.vExtraPayload.empty()) {
            try {
                CDataStream ds(tx.vExtraPayload, SER_NETWORK, PROTOCOL_VERSION);
                CValidatorRegistrationPayload payload;
                ds >> payload;
                
                // Check if we own the validator key
                if (wallet.HaveKey(payload.validatorPubKey.GetID())) {
                    CValidatorInfo info;
                    info.pubkey = payload.validatorPubKey;
                    info.nStakeAmount = payload.nStakeAmount;
                    info.strDescription = payload.strDescription;
                    info.nRegistrationTime = wtx.GetTxTime();
                    info.txid = tx.GetHash();
                    
                    // Get current validator status from the network
                    CValidator validator;
                    if (GetValidator(payload.validatorPubKey, validator)) {
                        info.fActive = validator.fActive && IsValidatorActive(payload.validatorPubKey);
                        info.nStakeAmount = validator.nStakeAmount; // Use current stake amount
                    } else {
                        info.fActive = false;
                    }
                    
                    info.fSlashed = IsValidatorSlashed(payload.validatorPubKey);
                    info.fBlacklisted = IsValidatorBlacklisted(payload.validatorPubKey);
                    
                    validators.push_back(info);
                }
            } catch (const std::exception& e) {
                LogPrintf("GetWalletValidators: Failed to parse validator registration payload: %s\n", e.what());
                continue;
            }
        }
    }
    
    return validators;
}

/**
 * Get total staked balance for this wallet
 */
CAmount GetStakedBalance(const CWallet& wallet)
{
    CAmount totalStaked = 0;
    
    LOCK(wallet.cs_wallet);
    
    // Get all wallet UTXOs and check if they're stake-locked
    std::vector<COutput> vCoins;
    wallet.AvailableCoins(vCoins);
    
    for (const COutput& out : vCoins) {
        COutPoint outpoint(out.tx->GetHash(), out.i);
        if (IsStakeLocked(outpoint)) {
            totalStaked += out.tx->tx->vout[out.i].nValue;
        }
    }
    
    return totalStaked;
}

/**
 * Get balance available for staking (not currently staked)
 */
CAmount GetStakingBalance(const CWallet& wallet)
{
    CAmount totalAvailable = 0;
    
    LOCK(wallet.cs_wallet);
    
    // Get all wallet UTXOs that are not stake-locked
    std::vector<COutput> vCoins;
    wallet.AvailableCoins(vCoins);
    
    for (const COutput& out : vCoins) {
        COutPoint outpoint(out.tx->GetHash(), out.i);
        if (!IsStakeLocked(outpoint)) {
            totalAvailable += out.tx->tx->vout[out.i].nValue;
        }
    }
    
    return totalAvailable;
}

/**
 * Create a stake transaction
 */
bool CreateStakeTransaction(CWallet& wallet, CAmount nAmount, int64_t nLockTime, 
                           const CTxDestination& dest, CTransactionRef& txOut, std::string& strError)
{
    LOCK(wallet.cs_wallet);
    
    if (wallet.IsLocked()) {
        strError = "Wallet is locked";
        return false;
    }
    
    const Consensus::Params& consensusParams = Params().GetConsensus();
    
    // Validate parameters
    if (nAmount < consensusParams.nMinStakeAmount) {
        strError = strprintf("Stake amount %s is below minimum %s", 
                           FormatMoney(nAmount), FormatMoney(consensusParams.nMinStakeAmount));
        return false;
    }
    
    if (nLockTime < consensusParams.nStakeMinAge) {
        strError = strprintf("Lock time %d is below minimum %d seconds", nLockTime, consensusParams.nStakeMinAge);
        return false;
    }
    
    if (nLockTime > consensusParams.nStakeMaxAge) {
        strError = strprintf("Lock time %d exceeds maximum %d seconds", nLockTime, consensusParams.nStakeMaxAge);
        return false;
    }
    
    // Get public key for staking
    CPubKey stakePubKey;
    if (!wallet.GetPubKey(GetScriptForDestination(dest), stakePubKey)) {
        strError = "Unable to get public key for staking";
        return false;
    }
    
    // Create transaction
    CMutableTransaction mtx;
    mtx.nVersion = 3;
    mtx.nType = TRANSACTION_STAKE;
    
    // Create stake payload
    CStakeTransactionPayload payload;
    payload.nStakeAmount = nAmount;
    payload.stakePubKey = stakePubKey;
    payload.nLockTime = nLockTime;
    
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    mtx.vExtraPayload.assign(ds.begin(), ds.end());
    
    // Add output for staked amount
    mtx.vout.emplace_back(nAmount, GetScriptForDestination(dest));
    
    // Fund the transaction
    CCoinControl coin_control;
    CAmount nFeeRequired;
    int nChangePosRet = -1;
    
    if (!wallet.FundTransaction(mtx, nFeeRequired, nChangePosRet, strError, false, coin_control)) {
        return false;
    }
    
    // Sign the transaction
    if (!wallet.SignTransaction(mtx)) {
        strError = "Transaction signing failed";
        return false;
    }
    
    txOut = MakeTransactionRef(std::move(mtx));
    return true;
}

/**
 * Create an unstake transaction
 */
bool CreateUnstakeTransaction(CWallet& wallet, const COutPoint& stakeOutpoint, 
                             const CTxDestination& dest, CTransactionRef& txOut, std::string& strError)
{
    LOCK(wallet.cs_wallet);
    
    if (wallet.IsLocked()) {
        strError = "Wallet is locked";
        return false;
    }
    
    // Check if the stake exists and is owned by this wallet
    CStakeLock stakeLock;
    if (!GetStakeLock(stakeOutpoint, stakeLock)) {
        strError = "Stake lock not found";
        return false;
    }
    
    // Check if stake lock has expired
    int64_t nCurrentTime = GetTime();
    if (!stakeLock.IsExpired(nCurrentTime)) {
        strError = strprintf("Stake lock not yet expired (expires at %d, current time %d)", 
                           stakeLock.nLockTime, nCurrentTime);
        return false;
    }
    
    // Verify we own the stake
    if (!wallet.HaveKey(stakeLock.ownerPubKey.GetID())) {
        strError = "Wallet does not own this stake";
        return false;
    }
    
    // Create transaction
    CMutableTransaction mtx;
    mtx.nVersion = 3;
    mtx.nType = TRANSACTION_UNSTAKE;
    
    // Create unstake payload
    CUnstakeTransactionPayload payload;
    payload.stakeOutpoint = stakeOutpoint;
    payload.stakePubKey = stakeLock.ownerPubKey;
    
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    mtx.vExtraPayload.assign(ds.begin(), ds.end());
    
    // Add input for the staked output
    mtx.vin.emplace_back(stakeOutpoint);
    
    // Add output to return the staked amount (minus fee)
    CAmount nStakeAmount = stakeLock.nAmount;
    CAmount nFee = 1000; // Basic fee estimation
    if (nStakeAmount > nFee) {
        mtx.vout.emplace_back(nStakeAmount - nFee, GetScriptForDestination(dest));
    }
    
    // Sign the transaction
    if (!wallet.SignTransaction(mtx)) {
        strError = "Transaction signing failed";
        return false;
    }
    
    txOut = MakeTransactionRef(std::move(mtx));
    return true;
}

/**
 * Create a validator registration transaction
 */
bool CreateValidatorRegistration(CWallet& wallet, CAmount nStakeAmount, const CPubKey& validatorPubKey,
                                const std::string& strDescription, const CTxDestination& rewardDest,
                                CTransactionRef& txOut, std::string& strError)
{
    LOCK(wallet.cs_wallet);
    
    if (wallet.IsLocked()) {
        strError = "Wallet is locked";
        return false;
    }
    
    const Consensus::Params& consensusParams = Params().GetConsensus();
    
    // Validate parameters
    CAmount nMinValidatorStake = consensusParams.nMinStakeAmount * 10; // Validators need 10x minimum stake
    if (nStakeAmount < nMinValidatorStake) {
        strError = strprintf("Validator stake amount %s is below minimum %s", 
                           FormatMoney(nStakeAmount), FormatMoney(nMinValidatorStake));
        return false;
    }
    
    if (!validatorPubKey.IsValid()) {
        strError = "Invalid validator public key";
        return false;
    }
    
    if (strDescription.length() > 256) {
        strError = "Description too long (maximum 256 characters)";
        return false;
    }
    
    // Check if validator is already registered
    if (IsValidatorRegistered(validatorPubKey)) {
        strError = "Validator already registered";
        return false;
    }
    
    // Verify we own the validator key
    if (!wallet.HaveKey(validatorPubKey.GetID())) {
        strError = "Wallet does not own the validator key";
        return false;
    }
    
    // Create transaction
    CMutableTransaction mtx;
    mtx.nVersion = 3;
    mtx.nType = TRANSACTION_VALIDATOR_REGISTER;
    
    // Create validator registration payload
    CValidatorRegistrationPayload payload;
    payload.validatorPubKey = validatorPubKey;
    payload.nStakeAmount = nStakeAmount;
    payload.rewardScript = GetScriptForDestination(rewardDest);
    payload.strDescription = strDescription;
    
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    mtx.vExtraPayload.assign(ds.begin(), ds.end());
    
    // Add output for staked amount (locked for validator)
    mtx.vout.emplace_back(nStakeAmount, GetScriptForDestination(rewardDest));
    
    // Fund the transaction
    CCoinControl coin_control;
    CAmount nFeeRequired;
    int nChangePosRet = -1;
    
    if (!wallet.FundTransaction(mtx, nFeeRequired, nChangePosRet, strError, false, coin_control)) {
        return false;
    }
    
    // Sign the transaction
    if (!wallet.SignTransaction(mtx)) {
        strError = "Transaction signing failed";
        return false;
    }
    
    txOut = MakeTransactionRef(std::move(mtx));
    return true;
}

/**
 * Calculate expected staking rewards for a given amount and duration
 */
CAmount CalculateExpectedStakingReward(CAmount nStakeAmount, int64_t nStakeDuration)
{
    const Consensus::Params& consensusParams = Params().GetConsensus();
    return CalculateStakeReward(nStakeAmount, nStakeDuration, consensusParams);
}

/**
 * Check if wallet has any active stakes
 */
bool HasActiveStakes(const CWallet& wallet)
{
    return GetStakedBalance(wallet) > 0;
}

/**
 * Check if wallet has any registered validators
 */
bool HasRegisteredValidators(const CWallet& wallet)
{
    std::vector<CValidatorInfo> validators = GetWalletValidators(wallet);
    return !validators.empty();
}

} // namespace wallet