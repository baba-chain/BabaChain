// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/util.h>
#include <pos.h>
#include <validation.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <key_io.h>
#include <wallet/wallet.h>
#include <wallet/rpc/util.h>
#include <node/context.h>

#include <univalue.h>

using node::NodeContext;

static RPCHelpMan stakecoin()
{
    return RPCHelpMan{"stakecoin",
        "\nStake coins for a specified duration to earn rewards.\n",
        {
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The amount to stake"},
            {"locktime", RPCArg::Type::NUM, RPCArg::Optional::NO, "Lock duration in seconds"},
            {"address", RPCArg::Type::STR, RPCArg::Default{""}, "The address to receive staking rewards (optional)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
                {RPCResult::Type::STR_AMOUNT, "amount", "The staked amount"},
                {RPCResult::Type::NUM, "locktime", "The lock duration in seconds"},
                {RPCResult::Type::STR, "address", "The reward address"},
            }
        },
        RPCExamples{
            HelpExampleCli("stakecoin", "100 86400")
            + HelpExampleCli("stakecoin", "100 86400 \"BabaChainAddress\"")
            + HelpExampleRpc("stakecoin", "100, 86400")
            + HelpExampleRpc("stakecoin", "100, 86400, \"BabaChainAddress\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    // Ensure wallet is unlocked
    LOCK(pwallet->cs_wallet);
    
    if (pwallet->IsLocked()) {
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    }

    CAmount nAmount = AmountFromValue(request.params[0]);
    int64_t nLockTime = request.params[1].get_int64();
    
    // Validate parameters
    const Consensus::Params& consensusParams = Params().GetConsensus();
    
    if (nAmount < consensusParams.nMinStakeAmount) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, 
            strprintf("Stake amount %s is below minimum %s", 
                     FormatMoney(nAmount), FormatMoney(consensusParams.nMinStakeAmount)));
    }
    
    if (nLockTime < consensusParams.nStakeMinAge) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, 
            strprintf("Lock time %d is below minimum %d seconds", nLockTime, consensusParams.nStakeMinAge));
    }
    
    if (nLockTime > consensusParams.nStakeMaxAge) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, 
            strprintf("Lock time %d exceeds maximum %d seconds", nLockTime, consensusParams.nStakeMaxAge));
    }

    // Get or generate reward address
    CTxDestination dest;
    if (!request.params[2].isNull() && !request.params[2].get_str().empty()) {
        dest = DecodeDestination(request.params[2].get_str());
        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BabaChain address");
        }
    } else {
        // Generate new address for rewards
        if (!pwallet->GetNewDestination(OutputType::LEGACY, "", dest)) {
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
        }
    }

    // Get public key for staking
    CPubKey stakePubKey;
    if (!pwallet->GetPubKey(GetScriptForDestination(dest), stakePubKey)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Unable to get public key for staking");
    }

    // Create staking transaction
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
    std::string strError;
    
    if (!pwallet->FundTransaction(mtx, nFeeRequired, nChangePosRet, strError, false, coin_control)) {
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    // Sign the transaction
    const CTransaction txConst(mtx);
    if (!pwallet->SignTransaction(mtx)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction signing failed");
    }

    // Submit to mempool
    CTransactionRef tx = MakeTransactionRef(std::move(mtx));
    
    NodeContext& node = EnsureAnyNodeContext(request.context);
    const TransactionError err = BroadcastTransaction(node, tx, strError, DEFAULT_MAX_RAW_TX_FEE_RATE.GetFeePerK(), true, true);
    if (TransactionError::OK != err) {
        throw JSONRPCTransactionError(err, strError);
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("txid", tx->GetHash().GetHex());
    result.pushKV("amount", ValueFromAmount(nAmount));
    result.pushKV("locktime", nLockTime);
    result.pushKV("address", EncodeDestination(dest));
    
    return result;
#else
    throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Wallet functionality not available");
#endif
},
    };
}

static RPCHelpMan getstakinginfo()
{
    return RPCHelpMan{"getstakinginfo",
        "\nReturns information about the current staking status and network.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "staking", "Whether staking is currently active"},
                {RPCResult::Type::NUM, "stakingbalance", "Total balance available for staking"},
                {RPCResult::Type::NUM, "stakedbalance", "Total balance currently staked"},
                {RPCResult::Type::NUM, "totalvalidators", "Total number of registered validators"},
                {RPCResult::Type::NUM, "activevalidators", "Number of active validators"},
                {RPCResult::Type::STR_AMOUNT, "totalstake", "Total amount staked in the network"},
                {RPCResult::Type::STR_AMOUNT, "expectedreward", "Expected reward per block"},
                {RPCResult::Type::NUM, "stakingdifficulty", "Current staking difficulty"},
                {RPCResult::Type::NUM, "netstakeweight", "Network stake weight"},
                {RPCResult::Type::STR_AMOUNT, "remainingsupply", "Remaining supply for staking rewards"},
            }
        },
        RPCExamples{
            HelpExampleCli("getstakinginfo", "")
            + HelpExampleRpc("getstakinginfo", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    LOCK(cs_main);
    
    const Consensus::Params& consensusParams = Params().GetConsensus();
    const CChainState& active_chainstate = ChainstateActive();
    
    UniValue result(UniValue::VOBJ);
    
    // Basic staking status
    bool fStaking = true; // In a real implementation, check if staking is enabled
    result.pushKV("staking", fStaking);
    
    // Get validator information
    size_t nTotalValidators = GetValidatorCount();
    size_t nActiveValidators = GetActiveValidatorCount();
    CAmount nTotalStake = GetTotalActiveStake();
    
    result.pushKV("totalvalidators", (int)nTotalValidators);
    result.pushKV("activevalidators", (int)nActiveValidators);
    result.pushKV("totalstake", ValueFromAmount(nTotalStake));
    
    // Calculate expected reward
    CAmount nExpectedReward = consensusParams.nStakeRewardPerBlock;
    result.pushKV("expectedreward", ValueFromAmount(nExpectedReward));
    
    // Staking difficulty (simplified calculation)
    double dDifficulty = 1.0;
    if (nTotalStake > 0) {
        dDifficulty = static_cast<double>(nTotalStake) / static_cast<double>(consensusParams.nMinStakeAmount);
    }
    result.pushKV("stakingdifficulty", dDifficulty);
    
    // Network stake weight (same as total stake for now)
    result.pushKV("netstakeweight", ValueFromAmount(nTotalStake));
    
    // Remaining supply for staking rewards
    int nHeight = active_chainstate.m_chain.Height();
    CAmount nRemainingSupply = GetRemainingStakingSupply(nHeight, consensusParams);
    result.pushKV("remainingsupply", ValueFromAmount(nRemainingSupply));

#ifdef ENABLE_WALLET
    // Wallet-specific information if wallet is available
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (pwallet) {
        LOCK(pwallet->cs_wallet);
        
        // Calculate staking and staked balances
        CAmount nStakingBalance = 0;
        CAmount nStakedBalance = 0;
        
        // Get all wallet UTXOs
        std::vector<COutput> vCoins;
        pwallet->AvailableCoins(vCoins);
        
        for (const COutput& out : vCoins) {
            if (IsStakeLocked(COutPoint(out.tx->GetHash(), out.i))) {
                nStakedBalance += out.tx->tx->vout[out.i].nValue;
            } else {
                nStakingBalance += out.tx->tx->vout[out.i].nValue;
            }
        }
        
        result.pushKV("stakingbalance", ValueFromAmount(nStakingBalance));
        result.pushKV("stakedbalance", ValueFromAmount(nStakedBalance));
    } else {
        result.pushKV("stakingbalance", ValueFromAmount(0));
        result.pushKV("stakedbalance", ValueFromAmount(0));
    }
#else
    result.pushKV("stakingbalance", ValueFromAmount(0));
    result.pushKV("stakedbalance", ValueFromAmount(0));
#endif
    
    return result;
},
    };
}

static RPCHelpMan listvalidators()
{
    return RPCHelpMan{"listvalidators",
        "\nReturns a list of all registered validators and their information.\n",
        {
            {"activeonly", RPCArg::Type::BOOL, RPCArg::Default{false}, "Only return active validators"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR, "pubkey", "Validator public key"},
                    {RPCResult::Type::STR_AMOUNT, "stake", "Validator stake amount"},
                    {RPCResult::Type::NUM_TIME, "registrationtime", "Registration timestamp"},
                    {RPCResult::Type::BOOL, "active", "Whether validator is active"},
                    {RPCResult::Type::BOOL, "slashed", "Whether validator has been slashed"},
                    {RPCResult::Type::BOOL, "blacklisted", "Whether validator is blacklisted"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("listvalidators", "")
            + HelpExampleCli("listvalidators", "true")
            + HelpExampleRpc("listvalidators", "")
            + HelpExampleRpc("listvalidators", "true")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    bool fActiveOnly = false;
    if (!request.params[0].isNull()) {
        fActiveOnly = request.params[0].get_bool();
    }
    
    UniValue result(UniValue::VARR);
    
    // Get all validators
    std::vector<CValidator> validators;
    if (fActiveOnly) {
        validators = GetActiveValidators();
    } else {
        // For now, we'll get active validators and add inactive ones
        // In a real implementation, we'd have a function to get all validators
        validators = GetActiveValidators();
    }
    
    for (const CValidator& validator : validators) {
        UniValue validatorObj(UniValue::VOBJ);
        
        validatorObj.pushKV("pubkey", validator.pubkey.ToString());
        validatorObj.pushKV("stake", ValueFromAmount(validator.nStakeAmount));
        validatorObj.pushKV("registrationtime", validator.nRegistrationTime);
        validatorObj.pushKV("active", validator.fActive && IsValidatorActive(validator.pubkey));
        validatorObj.pushKV("slashed", IsValidatorSlashed(validator.pubkey));
        validatorObj.pushKV("blacklisted", IsValidatorBlacklisted(validator.pubkey));
        
        // Add slashing information if validator was slashed
        if (IsValidatorSlashed(validator.pubkey)) {
            int64_t slashTime;
            CAmount penalty;
            if (GetSlashingInfo(validator.pubkey, slashTime, penalty)) {
                validatorObj.pushKV("slashtime", slashTime);
                validatorObj.pushKV("penalty", ValueFromAmount(penalty));
            }
        }
        
        result.push_back(validatorObj);
    }
    
    return result;
},
    };
}

void RegisterStakingRPCCommands(CRPCTable &t)
{
    static const CRPCCommand commands[] = {
        {"staking", &stakecoin},
        {"staking", &getstakinginfo},
        {"staking", &listvalidators},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}