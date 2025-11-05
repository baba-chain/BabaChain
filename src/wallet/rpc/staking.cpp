// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/rpc/util.h>
#include <wallet/wallet.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/moneystr.h>
#include <key_io.h>
#include <span.h>

#include <univalue.h>

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
        "\nReturns the balance available for staking.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_AMOUNT, "stakingbalance", "Balance available for staking"},
                {RPCResult::Type::STR_AMOUNT, "stakedbalance", "Balance currently staked"},
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
    
    CAmount stakingBalance = pwallet->GetStakingBalance();
    CAmount stakedBalance = pwallet->GetStakedBalance();
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("stakingbalance", FormatMoney(stakingBalance));
    result.pushKV("stakedbalance", FormatMoney(stakedBalance));
    
    return result;
},
    };
}

Span<const CRPCCommand> GetStakingRPCCommands()
{
    static const CRPCCommand commands[] = {
        {"wallet", &setstaking},
        {"wallet", &getstakingbalance},
    };
    
    return commands;
}

} // namespace wallet