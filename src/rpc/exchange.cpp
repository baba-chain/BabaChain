// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/util.h>
#include <validation.h>
#include <chainparams.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <key_io.h>
#include <wallet/wallet.h>
#include <wallet/rpc/util.h>
#include <node/context.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <consensus/validation.h>

#include <univalue.h>
#include <map>
#include <vector>

using node::NodeContext;

// Exchange order book structure
struct ExchangeOrder {
    uint256 orderId;
    std::string fromAsset;
    std::string toAsset;
    CAmount fromAmount;
    CAmount toAmount;
    CTxDestination userAddress;
    int64_t timestamp;
    bool isActive;
    
    ExchangeOrder() : fromAmount(0), toAmount(0), timestamp(0), isActive(false) {}
    ExchangeOrder(const uint256& id, const std::string& from, const std::string& to, 
                  CAmount fromAmt, CAmount toAmt, const CTxDestination& addr)
        : orderId(id), fromAsset(from), toAsset(to), fromAmount(fromAmt), 
          toAmount(toAmt), userAddress(addr), timestamp(GetTime()), isActive(true) {}
};

// Global order book (in production, this would be persistent storage)
static std::map<uint256, ExchangeOrder> g_orderBook;
static std::map<std::string, CAmount> g_liquidityPools;

// DCA (Dollar Cost Averaging) schedule structure
struct DCASchedule {
    uint256 scheduleId;
    std::string fromAsset;
    std::string toAsset;
    CAmount amountPerPurchase;
    int64_t intervalSeconds;
    int64_t nextExecutionTime;
    int totalPurchases;
    int remainingPurchases;
    CTxDestination userAddress;
    bool isActive;
    
    DCASchedule() : amountPerPurchase(0), intervalSeconds(0), nextExecutionTime(0),
                   totalPurchases(0), remainingPurchases(0), isActive(false) {}
};

static std::map<uint256, DCASchedule> g_dcaSchedules;

static RPCHelpMan createexchangeorder()
{
    return RPCHelpMan{"createexchangeorder",
        "\nCreate a new exchange order to trade one asset for another.\n",
        {
            {"fromasset", RPCArg::Type::STR, RPCArg::Optional::NO, "Asset to sell (e.g., 'BABA', 'BTC', 'ETH')"},
            {"toasset", RPCArg::Type::STR, RPCArg::Optional::NO, "Asset to buy"},
            {"fromamount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount to sell"},
            {"toamount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount to receive"},
            {"useraddress", RPCArg::Type::STR, RPCArg::Default{""}, "User address for the trade"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "orderid", "The order ID"},
                {RPCResult::Type::STR, "fromasset", "Asset being sold"},
                {RPCResult::Type::STR, "toasset", "Asset being bought"},
                {RPCResult::Type::STR_AMOUNT, "fromamount", "Amount being sold"},
                {RPCResult::Type::STR_AMOUNT, "toamount", "Amount to receive"},
                {RPCResult::Type::NUM_TIME, "timestamp", "Order creation time"},
            }
        },
        RPCExamples{
            HelpExampleCli("createexchangeorder", "\"BABA\" \"BTC\" 1000 0.01")
            + HelpExampleRpc("createexchangeorder", "\"BABA\", \"BTC\", 1000, 0.01")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    std::string fromAsset = request.params[0].get_str();
    std::string toAsset = request.params[1].get_str();
    CAmount fromAmount = AmountFromValue(request.params[2]);
    CAmount toAmount = AmountFromValue(request.params[3]);
    
    // Validate assets
    if (fromAsset == toAsset) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot trade the same asset");
    }
    
    if (fromAmount <= 0 || toAmount <= 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Amounts must be positive");
    }
    
    // Get user address
    CTxDestination userAddress;
    if (!request.params[4].isNull() && !request.params[4].get_str().empty()) {
        userAddress = DecodeDestination(request.params[4].get_str());
        if (!IsValidDestination(userAddress)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
        }
    } else {
        if (!pwallet->GetNewDestination(OutputType::LEGACY, "", userAddress)) {
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out");
        }
    }
    
    // Generate order ID
    uint256 orderId = GetRandHash();
    
    // Create order
    ExchangeOrder order(orderId, fromAsset, toAsset, fromAmount, toAmount, userAddress);
    g_orderBook[orderId] = order;
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("orderid", orderId.GetHex());
    result.pushKV("fromasset", fromAsset);
    result.pushKV("toasset", toAsset);
    result.pushKV("fromamount", ValueFromAmount(fromAmount));
    result.pushKV("toamount", ValueFromAmount(toAmount));
    result.pushKV("timestamp", order.timestamp);
    
    return result;
#else
    throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Wallet functionality not available");
#endif
},
    };
}

static RPCHelpMan getexchangeorders()
{
    return RPCHelpMan{"getexchangeorders",
        "\nGet all active exchange orders or orders for specific assets.\n",
        {
            {"fromasset", RPCArg::Type::STR, RPCArg::Default{""}, "Filter by from asset"},
            {"toasset", RPCArg::Type::STR, RPCArg::Default{""}, "Filter by to asset"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "orderid", "Order ID"},
                    {RPCResult::Type::STR, "fromasset", "Asset being sold"},
                    {RPCResult::Type::STR, "toasset", "Asset being bought"},
                    {RPCResult::Type::STR_AMOUNT, "fromamount", "Amount being sold"},
                    {RPCResult::Type::STR_AMOUNT, "toamount", "Amount to receive"},
                    {RPCResult::Type::STR, "useraddress", "User address"},
                    {RPCResult::Type::NUM_TIME, "timestamp", "Order creation time"},
                    {RPCResult::Type::BOOL, "active", "Whether order is active"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getexchangeorders", "")
            + HelpExampleCli("getexchangeorders", "\"BABA\" \"BTC\"")
            + HelpExampleRpc("getexchangeorders", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string fromAssetFilter = "";
    std::string toAssetFilter = "";
    
    if (!request.params[0].isNull()) {
        fromAssetFilter = request.params[0].get_str();
    }
    if (!request.params[1].isNull()) {
        toAssetFilter = request.params[1].get_str();
    }
    
    UniValue result(UniValue::VARR);
    
    for (const auto& [orderId, order] : g_orderBook) {
        if (!order.isActive) continue;
        
        if (!fromAssetFilter.empty() && order.fromAsset != fromAssetFilter) continue;
        if (!toAssetFilter.empty() && order.toAsset != toAssetFilter) continue;
        
        UniValue orderObj(UniValue::VOBJ);
        orderObj.pushKV("orderid", orderId.GetHex());
        orderObj.pushKV("fromasset", order.fromAsset);
        orderObj.pushKV("toasset", order.toAsset);
        orderObj.pushKV("fromamount", ValueFromAmount(order.fromAmount));
        orderObj.pushKV("toamount", ValueFromAmount(order.toAmount));
        orderObj.pushKV("useraddress", EncodeDestination(order.userAddress));
        orderObj.pushKV("timestamp", order.timestamp);
        orderObj.pushKV("active", order.isActive);
        
        result.push_back(orderObj);
    }
    
    return result;
},
    };
}

static RPCHelpMan cancelexchangeorder()
{
    return RPCHelpMan{"cancelexchangeorder",
        "\nCancel an active exchange order.\n",
        {
            {"orderid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Order ID to cancel"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "orderid", "Cancelled order ID"},
                {RPCResult::Type::BOOL, "success", "Whether cancellation was successful"},
            }
        },
        RPCExamples{
            HelpExampleCli("cancelexchangeorder", "\"abc123...\"")
            + HelpExampleRpc("cancelexchangeorder", "\"abc123...\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    uint256 orderId = ParseHashV(request.params[0], "orderid");
    
    auto it = g_orderBook.find(orderId);
    if (it == g_orderBook.end()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Order not found");
    }
    
    if (!it->second.isActive) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Order is already inactive");
    }
    
    it->second.isActive = false;
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("orderid", orderId.GetHex());
    result.pushKV("success", true);
    
    return result;
},
    };
}

static RPCHelpMan createdcaschedule()
{
    return RPCHelpMan{"createdcaschedule",
        "\nCreate a Dollar Cost Averaging (DCA) schedule for automatic purchases.\n",
        {
            {"fromasset", RPCArg::Type::STR, RPCArg::Optional::NO, "Asset to spend (e.g., 'USD', 'BABA')"},
            {"toasset", RPCArg::Type::STR, RPCArg::Optional::NO, "Asset to buy"},
            {"amountperpurchase", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount to spend per purchase"},
            {"intervalseconds", RPCArg::Type::NUM, RPCArg::Optional::NO, "Interval between purchases in seconds"},
            {"totalpurchases", RPCArg::Type::NUM, RPCArg::Optional::NO, "Total number of purchases to make"},
            {"useraddress", RPCArg::Type::STR, RPCArg::Default{""}, "User address"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "scheduleid", "DCA schedule ID"},
                {RPCResult::Type::STR, "fromasset", "Asset being spent"},
                {RPCResult::Type::STR, "toasset", "Asset being bought"},
                {RPCResult::Type::STR_AMOUNT, "amountperpurchase", "Amount per purchase"},
                {RPCResult::Type::NUM, "intervalseconds", "Interval in seconds"},
                {RPCResult::Type::NUM, "totalpurchases", "Total purchases"},
                {RPCResult::Type::NUM_TIME, "nextexecution", "Next execution time"},
            }
        },
        RPCExamples{
            HelpExampleCli("createdcaschedule", "\"USD\" \"BABA\" 100 86400 30")
            + HelpExampleRpc("createdcaschedule", "\"USD\", \"BABA\", 100, 86400, 30")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    std::string fromAsset = request.params[0].get_str();
    std::string toAsset = request.params[1].get_str();
    CAmount amountPerPurchase = AmountFromValue(request.params[2]);
    int64_t intervalSeconds = request.params[3].get_int64();
    int totalPurchases = request.params[4].get_int();
    
    if (fromAsset == toAsset) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot DCA the same asset");
    }
    
    if (amountPerPurchase <= 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Amount per purchase must be positive");
    }
    
    if (intervalSeconds < 3600) { // Minimum 1 hour
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Interval must be at least 1 hour (3600 seconds)");
    }
    
    if (totalPurchases <= 0 || totalPurchases > 1000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Total purchases must be between 1 and 1000");
    }
    
    // Get user address
    CTxDestination userAddress;
    if (!request.params[5].isNull() && !request.params[5].get_str().empty()) {
        userAddress = DecodeDestination(request.params[5].get_str());
        if (!IsValidDestination(userAddress)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
        }
    } else {
        if (!pwallet->GetNewDestination(OutputType::LEGACY, "", userAddress)) {
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out");
        }
    }
    
    // Generate schedule ID
    uint256 scheduleId = GetRandHash();
    
    // Create DCA schedule
    DCASchedule schedule;
    schedule.scheduleId = scheduleId;
    schedule.fromAsset = fromAsset;
    schedule.toAsset = toAsset;
    schedule.amountPerPurchase = amountPerPurchase;
    schedule.intervalSeconds = intervalSeconds;
    schedule.nextExecutionTime = GetTime() + intervalSeconds;
    schedule.totalPurchases = totalPurchases;
    schedule.remainingPurchases = totalPurchases;
    schedule.userAddress = userAddress;
    schedule.isActive = true;
    
    g_dcaSchedules[scheduleId] = schedule;
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("scheduleid", scheduleId.GetHex());
    result.pushKV("fromasset", fromAsset);
    result.pushKV("toasset", toAsset);
    result.pushKV("amountperpurchase", ValueFromAmount(amountPerPurchase));
    result.pushKV("intervalseconds", intervalSeconds);
    result.pushKV("totalpurchases", totalPurchases);
    result.pushKV("nextexecution", schedule.nextExecutionTime);
    
    return result;
#else
    throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Wallet functionality not available");
#endif
},
    };
}

static RPCHelpMan getdcaschedules()
{
    return RPCHelpMan{"getdcaschedules",
        "\nGet all active DCA schedules.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "scheduleid", "Schedule ID"},
                    {RPCResult::Type::STR, "fromasset", "Asset being spent"},
                    {RPCResult::Type::STR, "toasset", "Asset being bought"},
                    {RPCResult::Type::STR_AMOUNT, "amountperpurchase", "Amount per purchase"},
                    {RPCResult::Type::NUM, "intervalseconds", "Interval in seconds"},
                    {RPCResult::Type::NUM, "totalpurchases", "Total purchases"},
                    {RPCResult::Type::NUM, "remainingpurchases", "Remaining purchases"},
                    {RPCResult::Type::NUM_TIME, "nextexecution", "Next execution time"},
                    {RPCResult::Type::BOOL, "active", "Whether schedule is active"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getdcaschedules", "")
            + HelpExampleRpc("getdcaschedules", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue result(UniValue::VARR);
    
    for (const auto& [scheduleId, schedule] : g_dcaSchedules) {
        if (!schedule.isActive) continue;
        
        UniValue scheduleObj(UniValue::VOBJ);
        scheduleObj.pushKV("scheduleid", scheduleId.GetHex());
        scheduleObj.pushKV("fromasset", schedule.fromAsset);
        scheduleObj.pushKV("toasset", schedule.toAsset);
        scheduleObj.pushKV("amountperpurchase", ValueFromAmount(schedule.amountPerPurchase));
        scheduleObj.pushKV("intervalseconds", schedule.intervalSeconds);
        scheduleObj.pushKV("totalpurchases", schedule.totalPurchases);
        scheduleObj.pushKV("remainingpurchases", schedule.remainingPurchases);
        scheduleObj.pushKV("nextexecution", schedule.nextExecutionTime);
        scheduleObj.pushKV("active", schedule.isActive);
        
        result.push_back(scheduleObj);
    }
    
    return result;
},
    };
}

static RPCHelpMan addliquidity()
{
    return RPCHelpMan{"addliquidity",
        "\nAdd liquidity to a trading pair for yield farming.\n",
        {
            {"asset1", RPCArg::Type::STR, RPCArg::Optional::NO, "First asset in the pair"},
            {"asset2", RPCArg::Type::STR, RPCArg::Optional::NO, "Second asset in the pair"},
            {"amount1", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of first asset"},
            {"amount2", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of second asset"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "Transaction ID"},
                {RPCResult::Type::STR, "pair", "Trading pair"},
                {RPCResult::Type::STR_AMOUNT, "liquidity_tokens", "LP tokens received"},
                {RPCResult::Type::NUM, "share_percentage", "Share of the pool"},
            }
        },
        RPCExamples{
            HelpExampleCli("addliquidity", "\"BABA\" \"BTC\" 1000 0.1")
            + HelpExampleRpc("addliquidity", "\"BABA\", \"BTC\", 1000, 0.1")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    std::string asset1 = request.params[0].get_str();
    std::string asset2 = request.params[1].get_str();
    CAmount amount1 = AmountFromValue(request.params[2]);
    CAmount amount2 = AmountFromValue(request.params[3]);
    
    if (asset1 == asset2) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot create pair with same asset");
    }
    
    if (amount1 <= 0 || amount2 <= 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Amounts must be positive");
    }
    
    // Create trading pair key (sorted alphabetically)
    std::string pairKey = asset1 < asset2 ? asset1 + "/" + asset2 : asset2 + "/" + asset1;
    
    // Add to liquidity pools
    g_liquidityPools[pairKey + "_" + asset1] += amount1;
    g_liquidityPools[pairKey + "_" + asset2] += amount2;
    
    // Calculate LP tokens (simplified: geometric mean)
    CAmount liquidityTokens = static_cast<CAmount>(sqrt(static_cast<double>(amount1) * static_cast<double>(amount2)));
    
    // Calculate share percentage
    CAmount totalLiquidity1 = g_liquidityPools[pairKey + "_" + asset1];
    CAmount totalLiquidity2 = g_liquidityPools[pairKey + "_" + asset2];
    double sharePercentage = (static_cast<double>(amount1) / static_cast<double>(totalLiquidity1)) * 100.0;
    
    // Generate transaction ID (in real implementation, this would be a real transaction)
    uint256 txid = GetRandHash();
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("txid", txid.GetHex());
    result.pushKV("pair", pairKey);
    result.pushKV("liquidity_tokens", ValueFromAmount(liquidityTokens));
    result.pushKV("share_percentage", sharePercentage);
    
    return result;
#else
    throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Wallet functionality not available");
#endif
},
    };
}

static RPCHelpMan getliquiditypools()
{
    return RPCHelpMan{"getliquiditypools",
        "\nGet information about all liquidity pools.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR, "pair", "Trading pair"},
                    {RPCResult::Type::STR_AMOUNT, "liquidity1", "Liquidity of first asset"},
                    {RPCResult::Type::STR_AMOUNT, "liquidity2", "Liquidity of second asset"},
                    {RPCResult::Type::NUM, "apy", "Annual Percentage Yield"},
                    {RPCResult::Type::STR_AMOUNT, "volume24h", "24-hour trading volume"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getliquiditypools", "")
            + HelpExampleRpc("getliquiditypools", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue result(UniValue::VARR);
    
    std::map<std::string, std::pair<CAmount, CAmount>> pairs;
    
    // Group liquidity by pairs
    for (const auto& [key, amount] : g_liquidityPools) {
        size_t lastUnderscore = key.find_last_of('_');
        if (lastUnderscore == std::string::npos) continue;
        
        std::string pairKey = key.substr(0, lastUnderscore);
        std::string asset = key.substr(lastUnderscore + 1);
        
        if (pairs.find(pairKey) == pairs.end()) {
            pairs[pairKey] = std::make_pair(0, 0);
        }
        
        // Determine which asset this is (simplified logic)
        if (asset < pairKey.substr(pairKey.find('/') + 1)) {
            pairs[pairKey].first = amount;
        } else {
            pairs[pairKey].second = amount;
        }
    }
    
    for (const auto& [pairKey, liquidity] : pairs) {
        if (liquidity.first == 0 && liquidity.second == 0) continue;
        
        UniValue poolObj(UniValue::VOBJ);
        poolObj.pushKV("pair", pairKey);
        poolObj.pushKV("liquidity1", ValueFromAmount(liquidity.first));
        poolObj.pushKV("liquidity2", ValueFromAmount(liquidity.second));
        
        // Calculate APY (simplified: based on liquidity size)
        double apy = std::min(50.0, 1000000.0 / static_cast<double>(liquidity.first + liquidity.second));
        poolObj.pushKV("apy", apy);
        
        // Mock 24h volume
        CAmount volume24h = (liquidity.first + liquidity.second) / 10;
        poolObj.pushKV("volume24h", ValueFromAmount(volume24h));
        
        result.push_back(poolObj);
    }
    
    return result;
},
    };
}

void RegisterExchangeRPCCommands(CRPCTable &t)
{
    static const CRPCCommand commands[] = {
        {"exchange", &createexchangeorder},
        {"exchange", &getexchangeorders},
        {"exchange", &cancelexchangeorder},
        {"exchange", &createdcaschedule},
        {"exchange", &getdcaschedules},
        {"exchange", &addliquidity},
        {"exchange", &getliquiditypools},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}