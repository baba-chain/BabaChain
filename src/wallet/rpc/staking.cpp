// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/rpc/util.h>
#include <wallet/wallet.h>
#include <wallet/staking.h>
#include <wallet/gamification.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <rpc/server_util.h>
#include <util/moneystr.h>
#include <key_io.h>
#include <node/context.h>
#include <node/transaction.h>
#include <pos.h>
#include <core_io.h>
#include <span.h>

#include <univalue.h>

using node::NodeContext;

namespace wallet {

static RPCHelpMan setstaking()
{
    return RPCHelpMan{"setstaking",
        "\nEnable or disable automatic staking for this wallet.\n",
        {
            {"enabled", RPCArg::Type::BOOL, RPCArg::Optional::NO, "Enable (true) or disable (false) automatic staking"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "staking", "Current staking status"},
            }
        },
        RPCExamples{
            HelpExampleCli("setstaking", "true")
            + HelpExampleCli("setstaking", "false")
            + HelpExampleRpc("setstaking", "true")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    bool enabled = request.params[0].get_bool();
    pwallet->SetStakingEnabled(enabled);
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("staking", pwallet->IsStakingEnabled());
    
    return result;
},
    };
}

static RPCHelpMan getstakingbalance()
{
    return RPCHelpMan{"getstakingbalance",
        "\nReturns the wallet's staking and staked balances.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_AMOUNT, "stakingbalance", "Balance available for staking"},
                {RPCResult::Type::STR_AMOUNT, "stakedbalance", "Balance currently staked"},
                {RPCResult::Type::BOOL, "staking", "Whether automatic staking is enabled"},
            }
        },
        RPCExamples{
            HelpExampleCli("getstakingbalance", "")
            + HelpExampleRpc("getstakingbalance", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("stakingbalance", ValueFromAmount(pwallet->GetStakingBalance()));
    result.pushKV("stakedbalance", ValueFromAmount(pwallet->GetStakedBalance()));
    result.pushKV("staking", pwallet->IsStakingEnabled());
    
    return result;
},
    };
}

static RPCHelpMan liststaking()
{
    return RPCHelpMan{"liststaking",
        "\nReturns a list of all staking transactions for this wallet.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "txid", "Transaction ID"},
                    {RPCResult::Type::STR, "type", "Transaction type (stake, unstake, validator)"},
                    {RPCResult::Type::NUM_TIME, "time", "Transaction time"},
                    {RPCResult::Type::STR_AMOUNT, "amount", "Amount involved"},
                    {RPCResult::Type::NUM, "confirmations", "Number of confirmations"},
                    {RPCResult::Type::NUM, "locktime", "Lock duration in seconds (for stake transactions)"},
                    {RPCResult::Type::STR, "stakepubkey", "Staking public key"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("liststaking", "")
            + HelpExampleRpc("liststaking", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    std::vector<CStakingInfo> stakingTxs = pwallet->GetStakingTransactions();
    
    UniValue result(UniValue::VARR);
    
    for (const CStakingInfo& info : stakingTxs) {
        UniValue obj(UniValue::VOBJ);
        
        obj.pushKV("txid", info.txid.GetHex());
        
        std::string strType;
        switch (info.nType) {
            case TRANSACTION_STAKE:
                strType = "stake";
                break;
            case TRANSACTION_UNSTAKE:
                strType = "unstake";
                break;
            case TRANSACTION_VALIDATOR_REGISTER:
                strType = "validator";
                break;
            default:
                strType = "unknown";
                break;
        }
        obj.pushKV("type", strType);
        
        obj.pushKV("time", info.nTime);
        obj.pushKV("amount", ValueFromAmount(info.nAmount));
        obj.pushKV("confirmations", info.nDepth);
        
        if (info.nType == TRANSACTION_STAKE) {
            obj.pushKV("locktime", info.nLockTime);
        }
        
        if (info.stakePubKey.IsValid()) {
            obj.pushKV("stakepubkey", HexStr(info.stakePubKey));
        }
        
        if (info.nType == TRANSACTION_VALIDATOR_REGISTER) {
            if (info.validatorPubKey.IsValid()) {
                obj.pushKV("validatorpubkey", HexStr(info.validatorPubKey));
            }
            if (!info.strDescription.empty()) {
                obj.pushKV("description", info.strDescription);
            }
        }
        
        if (info.nType == TRANSACTION_UNSTAKE) {
            obj.pushKV("stakeoutpoint", info.stakeOutpoint.ToString());
        }
        
        result.push_back(obj);
    }
    
    return result;
},
    };
}

static RPCHelpMan unstakecoin()
{
    return RPCHelpMan{"unstakecoin",
        "\nUnstake coins from a previous staking transaction.\n",
        {
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Transaction ID of the stake transaction"},
            {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "Output index of the stake transaction"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The unstaking transaction ID"},
                {RPCResult::Type::STR_AMOUNT, "amount", "The unstaked amount"},
            }
        },
        RPCExamples{
            HelpExampleCli("unstakecoin", "\"1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef\" 0")
            + HelpExampleRpc("unstakecoin", "\"1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef\", 0")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    uint256 txid = ParseHashV(request.params[0], "txid");
    int vout = request.params[1].getInt<int>();
    
    COutPoint stakeOutpoint(txid, vout);
    
    CTransactionRef txNew;
    std::string strError;
    
    if (!pwallet->CreateUnstakingTransaction(stakeOutpoint, txNew, strError)) {
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    
    // Submit to mempool
    NodeContext& node = EnsureAnyNodeContext(request.context);
    const TransactionError err = BroadcastTransaction(node, txNew, strError, CFeeRate(COIN / 10).GetFeePerK(), true, true);
    if (TransactionError::OK != err) {
        throw JSONRPCTransactionError(err, strError);
    }
    
    // Calculate unstaked amount
    CAmount nAmount = 0;
    for (const CTxOut& txout : txNew->vout) {
        nAmount += txout.nValue;
    }
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("txid", txNew->GetHash().GetHex());
    result.pushKV("amount", ValueFromAmount(nAmount));
    
    return result;
},
    };
}

static RPCHelpMan registervalidator()
{
    return RPCHelpMan{"registervalidator",
        "\nRegister as a validator on the BabaChain network.\n",
        {
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The amount to stake as a validator"},
            {"description", RPCArg::Type::STR, RPCArg::Default{""}, "Optional description for the validator"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The validator registration transaction ID"},
                {RPCResult::Type::STR_AMOUNT, "amount", "The validator stake amount"},
                {RPCResult::Type::STR, "validatorpubkey", "The validator public key"},
                {RPCResult::Type::STR, "description", "The validator description"},
            }
        },
        RPCExamples{
            HelpExampleCli("registervalidator", "1000")
            + HelpExampleCli("registervalidator", "1000 \"My Validator\"")
            + HelpExampleRpc("registervalidator", "1000")
            + HelpExampleRpc("registervalidator", "1000, \"My Validator\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    CAmount nAmount = AmountFromValue(request.params[0]);
    std::string strDescription;
    if (!request.params[1].isNull()) {
        strDescription = request.params[1].get_str();
    }
    
    CTransactionRef txNew;
    std::string strError;
    
    if (!pwallet->RegisterValidator(nAmount, strDescription, txNew, strError)) {
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    
    // Submit to mempool
    NodeContext& node = EnsureAnyNodeContext(request.context);
    const TransactionError err = BroadcastTransaction(node, txNew, strError, CFeeRate(COIN / 10).GetFeePerK(), true, true);
    if (TransactionError::OK != err) {
        throw JSONRPCTransactionError(err, strError);
    }
    
    // Extract validator public key from transaction
    std::string validatorPubKey;
    if (!txNew->vExtraPayload.empty()) {
        try {
            CDataStream ds(txNew->vExtraPayload, SER_NETWORK, PROTOCOL_VERSION);
            CValidatorRegistrationPayload payload;
            ds >> payload;
            validatorPubKey = HexStr(payload.validatorPubKey);
        } catch (const std::exception& e) {
            // Ignore parsing errors for response
        }
    }
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("txid", txNew->GetHash().GetHex());
    result.pushKV("amount", ValueFromAmount(nAmount));
    result.pushKV("validatorpubkey", validatorPubKey);
    result.pushKV("description", strDescription);
    
    return result;
},
    };
}

static RPCHelpMan listvalidators_wallet()
{
    return RPCHelpMan{"listvalidators",
        "\nReturns a list of validators owned by this wallet.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR, "pubkey", "Validator public key"},
                    {RPCResult::Type::STR_AMOUNT, "stake", "Validator stake amount"},
                    {RPCResult::Type::STR, "description", "Validator description"},
                    {RPCResult::Type::NUM_TIME, "registrationtime", "Registration timestamp"},
                    {RPCResult::Type::STR_HEX, "txid", "Registration transaction ID"},
                    {RPCResult::Type::BOOL, "active", "Whether validator is active"},
                    {RPCResult::Type::BOOL, "slashed", "Whether validator has been slashed"},
                    {RPCResult::Type::BOOL, "blacklisted", "Whether validator is blacklisted"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("listvalidators", "")
            + HelpExampleRpc("listvalidators", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    std::vector<CValidatorInfo> validators = pwallet->GetValidatorInfo();
    
    UniValue result(UniValue::VARR);
    
    for (const CValidatorInfo& info : validators) {
        UniValue obj(UniValue::VOBJ);
        
        obj.pushKV("pubkey", HexStr(info.pubkey));
        obj.pushKV("stake", ValueFromAmount(info.nStakeAmount));
        obj.pushKV("description", info.strDescription);
        obj.pushKV("registrationtime", info.nRegistrationTime);
        obj.pushKV("txid", info.txid.GetHex());
        obj.pushKV("active", info.fActive);
        obj.pushKV("slashed", info.fSlashed);
        obj.pushKV("blacklisted", info.fBlacklisted);
        
        result.push_back(obj);
    }
    
    return result;
},
    };
}

Span<const CRPCCommand> GetWalletStakingRPCCommands()
{
    static const CRPCCommand commands[] = {
        {"wallet", &setstaking},
        {"wallet", &getstakingbalance},
        {"wallet", &liststaking},
        {"wallet", &unstakecoin},
        {"wallet", &registervalidator},
        {"wallet", &listvalidators_wallet},

    };
    
    return MakeSpan(commands);
}

} // namespace wallet
