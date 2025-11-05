// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/util.h>
#include <validation.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <key_io.h>
#include <wallet/wallet.h>
#include <wallet/rpc/util.h>
#include <node/context.h>

#include <univalue.h>
#include <map>
#include <string>

using node::NodeContext;

// Fiat purchase order structure
struct FiatPurchaseOrder {
    uint256 orderId;
    std::string fiatCurrency;
    std::string cryptoAsset;
    CAmount fiatAmount;
    CAmount cryptoAmount;
    std::string paymentMethod;
    std::string status; // "pending", "processing", "completed", "failed"
    CTxDestination userAddress;
    int64_t timestamp;
    std::string externalOrderId;
    
    FiatPurchaseOrder() : fiatAmount(0), cryptoAmount(0), timestamp(0) {}
};

// Global fiat orders (in production, this would be persistent storage)
static std::map<uint256, FiatPurchaseOrder> g_fiatOrders;

// Supported fiat currencies and their exchange rates (mock data)
static std::map<std::string, double> g_exchangeRates = {
    {"USD", 1.0},
    {"EUR", 0.85},
    {"GBP", 0.73},
    {"JPY", 110.0},
    {"CAD", 1.25},
    {"AUD", 1.35}
};

// Supported payment methods
static std::vector<std::string> g_paymentMethods = {
    "credit_card",
    "debit_card", 
    "bank_transfer",
    "paypal",
    "apple_pay",
    "google_pay"
};

static RPCHelpMan buywithfiat()
{
    return RPCHelpMan{"buywithfiat",
        "\nPurchase cryptocurrency with fiat currency using credit card or other payment methods.\n",
        {
            {"fiatcurrency", RPCArg::Type::STR, RPCArg::Optional::NO, "Fiat currency (USD, EUR, GBP, etc.)"},
            {"cryptoasset", RPCArg::Type::STR, RPCArg::Optional::NO, "Cryptocurrency to buy (BABA, BTC, ETH)"},
            {"fiatamount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount in fiat currency"},
            {"paymentmethod", RPCArg::Type::STR, RPCArg::Optional::NO, "Payment method (credit_card, debit_card, bank_transfer, etc.)"},
            {"useraddress", RPCArg::Type::STR, RPCArg::Default{""}, "Address to receive cryptocurrency"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "orderid", "Purchase order ID"},
                {RPCResult::Type::STR, "fiatcurrency", "Fiat currency"},
                {RPCResult::Type::STR, "cryptoasset", "Cryptocurrency asset"},
                {RPCResult::Type::STR_AMOUNT, "fiatamount", "Fiat amount"},
                {RPCResult::Type::STR_AMOUNT, "cryptoamount", "Estimated crypto amount"},
                {RPCResult::Type::STR, "paymentmethod", "Payment method"},
                {RPCResult::Type::STR, "status", "Order status"},
                {RPCResult::Type::STR, "paymenturl", "URL to complete payment"},
                {RPCResult::Type::NUM_TIME, "timestamp", "Order creation time"},
            }
        },
        RPCExamples{
            HelpExampleCli("buywithfiat", "\"USD\" \"BABA\" 100 \"credit_card\"")
            + HelpExampleRpc("buywithfiat", "\"USD\", \"BABA\", 100, \"credit_card\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    std::string fiatCurrency = request.params[0].get_str();
    std::string cryptoAsset = request.params[1].get_str();
    CAmount fiatAmount = AmountFromValue(request.params[2]);
    std::string paymentMethod = request.params[3].get_str();
    
    // Validate fiat currency
    if (g_exchangeRates.find(fiatCurrency) == g_exchangeRates.end()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unsupported fiat currency");
    }
    
    // Validate payment method
    if (std::find(g_paymentMethods.begin(), g_paymentMethods.end(), paymentMethod) == g_paymentMethods.end()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unsupported payment method");
    }
    
    if (fiatAmount <= 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Fiat amount must be positive");
    }
    
    // Minimum purchase amount (equivalent to $10 USD)
    double minFiatAmount = 10.0 * g_exchangeRates[fiatCurrency];
    if (static_cast<double>(fiatAmount) / COIN < minFiatAmount) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, 
            strprintf("Minimum purchase amount is %.2f %s", minFiatAmount, fiatCurrency));
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
    
    // Calculate crypto amount (mock exchange rate: 1 BABA = $0.50)
    double cryptoPrice = 0.50; // USD per BABA
    if (cryptoAsset == "BTC") cryptoPrice = 45000.0;
    else if (cryptoAsset == "ETH") cryptoPrice = 3000.0;
    
    double fiatInUSD = static_cast<double>(fiatAmount) / COIN / g_exchangeRates[fiatCurrency];
    CAmount cryptoAmount = static_cast<CAmount>((fiatInUSD / cryptoPrice) * COIN);
    
    // Generate order ID
    uint256 orderId = GetRandHash();
    
    // Create purchase order
    FiatPurchaseOrder order;
    order.orderId = orderId;
    order.fiatCurrency = fiatCurrency;
    order.cryptoAsset = cryptoAsset;
    order.fiatAmount = fiatAmount;
    order.cryptoAmount = cryptoAmount;
    order.paymentMethod = paymentMethod;
    order.status = "pending";
    order.userAddress = userAddress;
    order.timestamp = GetTime();
    order.externalOrderId = "EXT_" + orderId.GetHex().substr(0, 16);
    
    g_fiatOrders[orderId] = order;
    
    // Generate payment URL (mock)
    std::string paymentUrl = "https://payments.babachain.org/pay/" + order.externalOrderId;
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("orderid", orderId.GetHex());
    result.pushKV("fiatcurrency", fiatCurrency);
    result.pushKV("cryptoasset", cryptoAsset);
    result.pushKV("fiatamount", ValueFromAmount(fiatAmount));
    result.pushKV("cryptoamount", ValueFromAmount(cryptoAmount));
    result.pushKV("paymentmethod", paymentMethod);
    result.pushKV("status", order.status);
    result.pushKV("paymenturl", paymentUrl);
    result.pushKV("timestamp", order.timestamp);
    
    return result;
#else
    throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Wallet functionality not available");
#endif
},
    };
}

static RPCHelpMan getfiatorders()
{
    return RPCHelpMan{"getfiatorders",
        "\nGet all fiat purchase orders.\n",
        {
            {"status", RPCArg::Type::STR, RPCArg::Default{""}, "Filter by status (pending, processing, completed, failed)"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "orderid", "Order ID"},
                    {RPCResult::Type::STR, "fiatcurrency", "Fiat currency"},
                    {RPCResult::Type::STR, "cryptoasset", "Cryptocurrency asset"},
                    {RPCResult::Type::STR_AMOUNT, "fiatamount", "Fiat amount"},
                    {RPCResult::Type::STR_AMOUNT, "cryptoamount", "Crypto amount"},
                    {RPCResult::Type::STR, "paymentmethod", "Payment method"},
                    {RPCResult::Type::STR, "status", "Order status"},
                    {RPCResult::Type::STR, "useraddress", "User address"},
                    {RPCResult::Type::NUM_TIME, "timestamp", "Order creation time"},
                    {RPCResult::Type::STR, "externalorderid", "External order ID"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getfiatorders", "")
            + HelpExampleCli("getfiatorders", "\"completed\"")
            + HelpExampleRpc("getfiatorders", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string statusFilter = "";
    if (!request.params[0].isNull()) {
        statusFilter = request.params[0].get_str();
    }
    
    UniValue result(UniValue::VARR);
    
    for (const auto& [orderId, order] : g_fiatOrders) {
        if (!statusFilter.empty() && order.status != statusFilter) continue;
        
        UniValue orderObj(UniValue::VOBJ);
        orderObj.pushKV("orderid", orderId.GetHex());
        orderObj.pushKV("fiatcurrency", order.fiatCurrency);
        orderObj.pushKV("cryptoasset", order.cryptoAsset);
        orderObj.pushKV("fiatamount", ValueFromAmount(order.fiatAmount));
        orderObj.pushKV("cryptoamount", ValueFromAmount(order.cryptoAmount));
        orderObj.pushKV("paymentmethod", order.paymentMethod);
        orderObj.pushKV("status", order.status);
        orderObj.pushKV("useraddress", EncodeDestination(order.userAddress));
        orderObj.pushKV("timestamp", order.timestamp);
        orderObj.pushKV("externalorderid", order.externalOrderId);
        
        result.push_back(orderObj);
    }
    
    return result;
},
    };
}

static RPCHelpMan getsupportedcurrencies()
{
    return RPCHelpMan{"getsupportedcurrencies",
        "\nGet list of supported fiat currencies and payment methods.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::ARR, "fiatcurrencies", "",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "currency", "Currency code"},
                        {RPCResult::Type::NUM, "exchangerate", "Exchange rate to USD"},
                    }},
                }},
                {RPCResult::Type::ARR, "paymentmethods", "",
                {
                    {RPCResult::Type::STR, "", "Payment method"},
                }},
                {RPCResult::Type::ARR, "cryptoassets", "",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "asset", "Asset symbol"},
                        {RPCResult::Type::NUM, "priceusd", "Price in USD"},
                    }},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getsupportedcurrencies", "")
            + HelpExampleRpc("getsupportedcurrencies", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue result(UniValue::VOBJ);
    
    // Fiat currencies
    UniValue fiatCurrencies(UniValue::VARR);
    for (const auto& [currency, rate] : g_exchangeRates) {
        UniValue currencyObj(UniValue::VOBJ);
        currencyObj.pushKV("currency", currency);
        currencyObj.pushKV("exchangerate", rate);
        fiatCurrencies.push_back(currencyObj);
    }
    result.pushKV("fiatcurrencies", fiatCurrencies);
    
    // Payment methods
    UniValue paymentMethods(UniValue::VARR);
    for (const std::string& method : g_paymentMethods) {
        paymentMethods.push_back(method);
    }
    result.pushKV("paymentmethods", paymentMethods);
    
    // Crypto assets
    UniValue cryptoAssets(UniValue::VARR);
    
    UniValue babaObj(UniValue::VOBJ);
    babaObj.pushKV("asset", "BABA");
    babaObj.pushKV("priceusd", 0.50);
    cryptoAssets.push_back(babaObj);
    
    UniValue btcObj(UniValue::VOBJ);
    btcObj.pushKV("asset", "BTC");
    btcObj.pushKV("priceusd", 45000.0);
    cryptoAssets.push_back(btcObj);
    
    UniValue ethObj(UniValue::VOBJ);
    ethObj.pushKV("asset", "ETH");
    ethObj.pushKV("priceusd", 3000.0);
    cryptoAssets.push_back(ethObj);
    
    result.pushKV("cryptoassets", cryptoAssets);
    
    return result;
},
    };
}

static RPCHelpMan updatefiatorderstatus()
{
    return RPCHelpMan{"updatefiatorderstatus",
        "\nUpdate the status of a fiat purchase order (for testing/admin purposes).\n",
        {
            {"orderid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Order ID"},
            {"status", RPCArg::Type::STR, RPCArg::Optional::NO, "New status (pending, processing, completed, failed)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "orderid", "Order ID"},
                {RPCResult::Type::STR, "oldstatus", "Previous status"},
                {RPCResult::Type::STR, "newstatus", "New status"},
                {RPCResult::Type::BOOL, "success", "Whether update was successful"},
            }
        },
        RPCExamples{
            HelpExampleCli("updatefiatorderstatus", "\"abc123...\" \"completed\"")
            + HelpExampleRpc("updatefiatorderstatus", "\"abc123...\", \"completed\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    uint256 orderId = ParseHashV(request.params[0], "orderid");
    std::string newStatus = request.params[1].get_str();
    
    // Validate status
    std::vector<std::string> validStatuses = {"pending", "processing", "completed", "failed"};
    if (std::find(validStatuses.begin(), validStatuses.end(), newStatus) == validStatuses.end()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid status");
    }
    
    auto it = g_fiatOrders.find(orderId);
    if (it == g_fiatOrders.end()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Order not found");
    }
    
    std::string oldStatus = it->second.status;
    it->second.status = newStatus;
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("orderid", orderId.GetHex());
    result.pushKV("oldstatus", oldStatus);
    result.pushKV("newstatus", newStatus);
    result.pushKV("success", true);
    
    return result;
},
    };
}

void RegisterFiatOnRampRPCCommands(CRPCTable &t)
{
    static const CRPCCommand commands[] = {
        {"fiatonramp", &buywithfiat},
        {"fiatonramp", &getfiatorders},
        {"fiatonramp", &getsupportedcurrencies},
        {"fiatonramp", &updatefiatorderstatus},
    };
    
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}