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
#include <util/time.h>
#include <logging.h>
#include <key_io.h>
#include <script/standard.h>

namespace wallet {

/**
 * Check if automatic staking is enabled for this wallet
 */
bool CWallet::IsStakingEnabled() const
{
    LOCK(cs_wallet);
    return m_staking_enabled && !IsLocked();
}

/**
 * Enable or disable automatic staking
 */
void CWallet::SetStakingEnabled(bool enabled)
{
    LOCK(cs_wallet);
    m_staking_enabled = enabled;
    
    if (enabled && !IsLocked()) {
        LogPrintf("CWallet::SetStakingEnabled: Automatic staking enabled for wallet %s\n", GetName());
    } else {
        LogPrintf("CWallet::SetStakingEnabled: Automatic staking disabled for wallet %s\n", GetName());
    }
}

/**
 * Get the total balance available for staking (mature, unlocked coins)
 */
CAmount CWallet::GetStakingBalance() const
{
    LOCK(cs_wallet);
    
    CAmount nStakingBalance = 0;
    const Consensus::Params& consensusParams = Params().GetConsensus();
    
    // Get all available coins
    std::vector<COutput> vCoins;
    AvailableCoins(vCoins, nullptr, 1, MAX_MONEY, MAX_MONEY, 0);
    
    for (const COutput& out : vCoins) {
        // Skip if coin is locked for other purposes
        if (IsLockedCoin(out.outpoint)) {
            continue;
        }
        
        // Skip if coin is already staked
        if (IsStakeLocked(out.outpoint)) {
            continue;
        }
        
        // Skip if amount is below minimum stake
        if (out.tx->tx->vout[out.i].nValue < consensusParams.nMinStakeAmount) {
            continue;
        }
        
        // Check if coin is mature enough for staking
        int nDepth = GetTxDepthInMainChain(*out.tx);
        if (nDepth >= consensusParams.nStakeMinAge / 600) { // Assuming 10 minute blocks
            nStakingBalance += out.tx->tx->vout[out.i].nValue;
        }
    }
    
    return nStakingBalance;
}

/**
 * Get the total balance currently staked
 */
CAmount CWallet::GetStakedBalance() const
{
    LOCK(cs_wallet);
    
    CAmount nStakedBalance = 0;
    
    // Get all wallet UTXOs
    std::vector<COutput> vCoins;
    AvailableCoins(vCoins, nullptr, 0, MAX_MONEY, MAX_MONEY, 0);
    
    for (const COutput& out : vCoins) {
        // Check if this output is stake-locked
        if (IsStakeLocked(out.outpoint)) {
            nStakedBalance += out.tx->tx->vout[out.i].nValue;
        }
    }
    
    return nStakedBalance;
}

/**
 * Get list of all staking transactions for this wallet
 */
std::vector<CStakingInfo> CWallet::GetStakingTransactions() const
{
    LOCK(cs_wallet);
    
    std::vector<CStakingInfo> stakingTxs;
    
    for (const auto& pair : mapWallet) {
        const CWalletTx& wtx = pair.second;
        
        // Check if this is a staking-related transaction
        if (wtx.tx->nType == TRANSACTION_STAKE || 
            wtx.tx->nType == TRANSACTION_UNSTAKE ||
            wtx.tx->nType == TRANSACTION_VALIDATOR_REGISTER) {
            
            CStakingInfo info;
            info.txid = wtx.GetHash();
            info.nType = wtx.tx->nType;
            info.nTime = wtx.GetTxTime();
            info.nAmount = 0;
            info.nDepth = GetTxDepthInMainChain(wtx);
            
            // Calculate total amount involved
            for (const CTxOut& txout : wtx.tx->vout) {
                if (IsMine(txout)) {
                    info.nAmount += txout.nValue;
                }
            }
            
            // Parse payload for additional information
            if (!wtx.tx->vExtraPayload.empty()) {
                try {
                    CDataStream ds(wtx.tx->vExtraPayload, SER_NETWORK, PROTOCOL_VERSION);
                    
                    if (wtx.tx->nType == TRANSACTION_STAKE) {
                        CStakeTransactionPayload payload;
                        ds >> payload;
                        info.nLockTime = payload.nLockTime;
                        info.stakePubKey = payload.stakePubKey;
                    } else if (wtx.tx->nType == TRANSACTION_UNSTAKE) {
                        CUnstakeTransactionPayload payload;
                        ds >> payload;
                        info.stakeOutpoint = payload.stakeOutpoint;
                        info.stakePubKey = payload.stakePubKey;
                    } else if (wtx.tx->nType == TRANSACTION_VALIDATOR_REGISTER) {
                        CValidatorRegistrationPayload payload;
                        ds >> payload;
                        info.validatorPubKey = payload.validatorPubKey;
                        info.strDescription = payload.strDescription;
                    }
                } catch (const std::exception& e) {
                    LogPrintf("CWallet::GetStakingTransactions: Failed to parse payload for tx %s: %s\n", 
                             wtx.GetHash().ToString(), e.what());
                }
            }
            
            stakingTxs.push_back(info);
        }
    }
    
    return stakingTxs;
}

/**
 * Create a staking transaction
 */
bool CWallet::CreateStakingTransaction(CAmount nAmount, int64_t nLockTime, const CTxDestination& dest, CTransactionRef& txNew, std::string& strError)
{
    LOCK(cs_wallet);
    
    if (IsLocked()) {
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
    if (!GetPubKey(GetScriptForDestination(dest), stakePubKey)) {
        strError = "Unable to get public key for staking";
        return false;
    }
    
    // Create transaction
    CMutableTransaction mtx;
    mtx.nVersion = 3;
    mtx.nType = TRANSACTION_STAKE;
    
    // Create stake payload
    CStakeTransactionPayload payload(nAmount, stakePubKey, nLockTime);
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    mtx.vExtraPayload.assign(ds.begin(), ds.end());
    
    // Add output for staked amount
    mtx.vout.emplace_back(nAmount, GetScriptForDestination(dest));
    
    // Fund the transaction
    CCoinControl coin_control;
    CAmount nFeeRequired;
    int nChangePosRet = -1;
    
    if (!FundTransaction(mtx, nFeeRequired, nChangePosRet, strError, false, coin_control)) {
        return false;
    }
    
    // Sign the transaction
    if (!SignTransaction(mtx)) {
        strError = "Transaction signing failed";
        return false;
    }
    
    txNew = MakeTransactionRef(std::move(mtx));
    return true;
}

/**
 * Create an unstaking transaction
 */
bool CWallet::CreateUnstakingTransaction(const COutPoint& stakeOutpoint, CTransactionRef& txNew, std::string& strError)
{
    LOCK(cs_wallet);
    
    if (IsLocked()) {
        strError = "Wallet is locked";
        return false;
    }
    
    // Check if the stake lock exists and is owned by this wallet
    CStakeLock stakeLock;
    if (!GetStakeLock(stakeOutpoint, stakeLock)) {
        strError = strprintf("No stake lock found for outpoint %s", stakeOutpoint.ToString());
        return false;
    }
    
    // Check if stake lock has expired
    int64_t nCurrentTime = GetTime();
    if (!stakeLock.IsExpired(nCurrentTime)) {
        strError = strprintf("Stake lock not yet expired (expires at %d, current time %d)", 
                           stakeLock.nLockTime, nCurrentTime);
        return false;
    }
    
    // Verify we own this stake
    const CWalletTx* pwtx = GetWalletTx(stakeOutpoint.hash);
    if (!pwtx) {
        strError = "Stake transaction not found in wallet";
        return false;
    }
    
    if (stakeOutpoint.n >= pwtx->tx->vout.size()) {
        strError = "Invalid stake output index";
        return false;
    }
    
    if (!IsMine(pwtx->tx->vout[stakeOutpoint.n])) {
        strError = "Stake output not owned by this wallet";
        return false;
    }
    
    // Create unstaking transaction
    CMutableTransaction mtx;
    mtx.nVersion = 3;
    mtx.nType = TRANSACTION_UNSTAKE;
    
    // Create unstake payload
    CUnstakeTransactionPayload payload(stakeOutpoint, stakeLock.ownerPubKey);
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    mtx.vExtraPayload.assign(ds.begin(), ds.end());
    
    // Add input spending the staked output
    mtx.vin.emplace_back(stakeOutpoint);
    
    // Add output returning the staked amount (minus fee)
    CScript scriptPubKey = pwtx->tx->vout[stakeOutpoint.n].scriptPubKey;
    CAmount nStakeAmount = pwtx->tx->vout[stakeOutpoint.n].nValue;
    
    // Estimate fee (simplified)
    CAmount nFee = 1000; // 1000 satoshis
    if (nStakeAmount <= nFee) {
        strError = "Stake amount too small to cover fee";
        return false;
    }
    
    mtx.vout.emplace_back(nStakeAmount - nFee, scriptPubKey);
    
    // Sign the transaction
    if (!SignTransaction(mtx)) {
        strError = "Transaction signing failed";
        return false;
    }
    
    txNew = MakeTransactionRef(std::move(mtx));
    return true;
}

/**
 * Automatically stake available coins
 */
bool CWallet::AutoStake()
{
    if (!IsStakingEnabled()) {
        return false;
    }
    
    LOCK(cs_wallet);
    
    const Consensus::Params& consensusParams = Params().GetConsensus();
    CAmount nMinStakeAmount = consensusParams.nMinStakeAmount;
    
    // Get available coins for staking
    std::vector<COutput> vCoins;
    AvailableCoins(vCoins, nullptr, 1, MAX_MONEY, MAX_MONEY, 0);
    
    bool fStakedAny = false;
    
    for (const COutput& out : vCoins) {
        // Skip if coin is locked or already staked
        if (IsLockedCoin(out.outpoint) || IsStakeLocked(out.outpoint)) {
            continue;
        }
        
        CAmount nAmount = out.tx->tx->vout[out.i].nValue;
        
        // Skip if amount is below minimum stake
        if (nAmount < nMinStakeAmount) {
            continue;
        }
        
        // Check if coin is mature enough for staking
        int nDepth = GetTxDepthInMainChain(*out.tx);
        if (nDepth < consensusParams.nStakeMinAge / 600) { // Assuming 10 minute blocks
            continue;
        }
        
        // Create staking transaction for this coin
        CTxDestination dest;
        if (!ExtractDestination(out.tx->tx->vout[out.i].scriptPubKey, dest)) {
            continue;
        }
        
        // Use default lock time (1 day)
        int64_t nLockTime = 24 * 60 * 60;
        
        CTransactionRef txNew;
        std::string strError;
        
        if (CreateStakingTransaction(nAmount, nLockTime, dest, txNew, strError)) {
            // Submit transaction to mempool
            // Note: In a real implementation, we would need to broadcast this transaction
            LogPrintf("CWallet::AutoStake: Created staking transaction %s for amount %s\n", 
                     txNew->GetHash().ToString(), FormatMoney(nAmount));
            fStakedAny = true;
        } else {
            LogPrintf("CWallet::AutoStake: Failed to create staking transaction: %s\n", strError);
        }
    }
    
    return fStakedAny;
}

/**
 * Register as a validator
 */
bool CWallet::RegisterValidator(CAmount nStakeAmount, const std::string& strDescription, CTransactionRef& txNew, std::string& strError)
{
    LOCK(cs_wallet);
    
    if (IsLocked()) {
        strError = "Wallet is locked";
        return false;
    }
    
    const Consensus::Params& consensusParams = Params().GetConsensus();
    
    // Validators need 10x minimum stake
    CAmount nMinValidatorStake = consensusParams.nMinStakeAmount * 10;
    if (nStakeAmount < nMinValidatorStake) {
        strError = strprintf("Validator stake amount %s is below minimum %s", 
                           FormatMoney(nStakeAmount), FormatMoney(nMinValidatorStake));
        return false;
    }
    
    // Generate new key pair for validator
    CTxDestination dest;
    if (!GetNewDestination(OutputType::LEGACY, "", dest)) {
        strError = "Failed to generate new address for validator";
        return false;
    }
    
    CPubKey validatorPubKey;
    if (!GetPubKey(GetScriptForDestination(dest), validatorPubKey)) {
        strError = "Unable to get public key for validator";
        return false;
    }
    
    // Create validator registration transaction
    CMutableTransaction mtx;
    mtx.nVersion = 3;
    mtx.nType = TRANSACTION_VALIDATOR_REGISTER;
    
    // Create validator registration payload
    CScript rewardScript = GetScriptForDestination(dest);
    CValidatorRegistrationPayload payload(validatorPubKey, nStakeAmount, rewardScript, strDescription);
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    mtx.vExtraPayload.assign(ds.begin(), ds.end());
    
    // Add output for validator stake
    mtx.vout.emplace_back(nStakeAmount, rewardScript);
    
    // Fund the transaction
    CCoinControl coin_control;
    CAmount nFeeRequired;
    int nChangePosRet = -1;
    
    if (!FundTransaction(mtx, nFeeRequired, nChangePosRet, strError, false, coin_control)) {
        return false;
    }
    
    // Sign the transaction
    if (!SignTransaction(mtx)) {
        strError = "Transaction signing failed";
        return false;
    }
    
    txNew = MakeTransactionRef(std::move(mtx));
    return true;
}

/**
 * Get validator information for this wallet
 */
std::vector<CValidatorInfo> CWallet::GetValidatorInfo() const
{
    LOCK(cs_wallet);
    
    std::vector<CValidatorInfo> validators;
    
    // Look through wallet transactions for validator registrations
    for (const auto& pair : mapWallet) {
        const CWalletTx& wtx = pair.second;
        
        if (wtx.tx->nType == TRANSACTION_VALIDATOR_REGISTER && !wtx.tx->vExtraPayload.empty()) {
            try {
                CDataStream ds(wtx.tx->vExtraPayload, SER_NETWORK, PROTOCOL_VERSION);
                CValidatorRegistrationPayload payload;
                ds >> payload;
                
                // Check if we own this validator
                bool fOwnValidator = false;
                for (const CTxOut& txout : wtx.tx->vout) {
                    if (IsMine(txout)) {
                        fOwnValidator = true;
                        break;
                    }
                }
                
                if (fOwnValidator) {
                    CValidatorInfo info;
                    info.pubkey = payload.validatorPubKey;
                    info.nStakeAmount = payload.nStakeAmount;
                    info.strDescription = payload.strDescription;
                    info.nRegistrationTime = wtx.GetTxTime();
                    info.txid = wtx.GetHash();
                    info.fActive = IsValidatorActive(payload.validatorPubKey);
                    info.fSlashed = IsValidatorSlashed(payload.validatorPubKey);
                    info.fBlacklisted = IsValidatorBlacklisted(payload.validatorPubKey);
                    
                    validators.push_back(info);
                }
            } catch (const std::exception& e) {
                LogPrintf("CWallet::GetValidatorInfo: Failed to parse validator registration payload for tx %s: %s\n", 
                         wtx.GetHash().ToString(), e.what());
            }
        }
    }
    
    return validators;
}

} // namespace wallet