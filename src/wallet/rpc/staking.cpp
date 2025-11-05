// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/rpc/util.h>
#include <wallet/wallet.h>
#include <wallet/staking.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/moneystr.h>
#include <key_io.h>
#include <node/context.h>
#include <node/transaction.h>

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
            obj.pushKV("stakepubkey", info.stakePubKey.ToString());
        }
        
        if (info.nType == TRANSACTION_VALIDATOR_REGISTER) {
            if (info.validatorPubKey.IsValid()) {
                obj.pushKV("validatorpubkey", info.validatorPubKey.ToString());
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
    int vout = request.params[1].get_int();
    
    COutPoint stakeOutpoint(txid, vout);
    
    CTransactionRef txNew;
    std::string strError;
    
    if (!pwallet->CreateUnstakingTransaction(stakeOutpoint, txNew, strError)) {
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    
    // Submit to mempool
    NodeContext& node = EnsureAnyNodeContext(request.context);
    const TransactionError err = BroadcastTransaction(node, txNew, strError, DEFAULT_MAX_RAW_TX_FEE_RATE.GetFeePerK(), true, true);
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
    const TransactionError err = BroadcastTransaction(node, txNew, strError, DEFAULT_MAX_RAW_TX_FEE_RATE.GetFeePerK(), true, true);
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
            validatorPubKey = payload.validatorPubKey.ToString();
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
        
        obj.pushKV("pubkey", info.pubkey.ToString());
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
        // Gamification commands
        {"wallet", &getgamificationprofile},
        {"wallet", &listachievements},
        {"wallet", &listchallenges},
        {"wallet", &createpet},
        {"wallet", &listpets},
        {"wallet", &evolvepet},
        {"wallet", &listnfts},
    };
    
    return MakeSpan(commands);
}

} // namespace wallet/
/ Gamification RPC Commands

static RPCHelpMan getgamificationprofile()
{
    return RPCHelpMan{"getgamificationprofile",
        "\nReturns the user's gamification profile including level, XP, and achievements.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "level", "Player level"},
                {RPCResult::Type::NUM, "experience", "Total experience points"},
                {RPCResult::Type::NUM, "experience_next_level", "Experience needed for next level"},
                {RPCResult::Type::NUM, "level_progress", "Progress to next level (percentage)"},
                {RPCResult::Type::NUM, "consecutive_days", "Current consecutive staking days"},
                {RPCResult::Type::NUM, "longest_streak", "Longest staking streak achieved"},
                {RPCResult::Type::STR_AMOUNT, "gamification_earnings", "Total earnings from gamification"},
                {RPCResult::Type::NUM, "achievements_unlocked", "Number of achievements unlocked"},
                {RPCResult::Type::NUM, "challenges_completed", "Number of challenges completed"},
                {RPCResult::Type::ARR, "owned_nfts", "List of owned NFT IDs"},
                {RPCResult::Type::ARR, "active_pets", "List of active pet IDs"},
            }
        },
        RPCExamples{
            HelpExampleCli("getgamificationprofile", "")
            + HelpExampleRpc("getgamificationprofile", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    auto profile = pwallet->GetGamificationManager().GetProfile();
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("level", profile.playerLevel);
    result.pushKV("experience", profile.totalExperiencePoints);
    result.pushKV("experience_next_level", profile.GetExperienceForNextLevel());
    result.pushKV("level_progress", profile.GetLevelProgress());
    result.pushKV("consecutive_days", profile.consecutiveStakingDays);
    result.pushKV("longest_streak", profile.longestStakingStreak);
    result.pushKV("gamification_earnings", ValueFromAmount(profile.totalEarningsFromGamification));
    result.pushKV("achievements_unlocked", profile.achievementsUnlocked);
    result.pushKV("challenges_completed", profile.challengesCompleted);
    
    UniValue nfts(UniValue::VARR);
    for (const auto& nftId : profile.ownedNFTs) {
        nfts.push_back(nftId.GetHex());
    }
    result.pushKV("owned_nfts", nfts);
    
    UniValue pets(UniValue::VARR);
    for (const auto& petId : profile.activePets) {
        pets.push_back(petId.GetHex());
    }
    result.pushKV("active_pets", pets);
    
    return result;
},
    };
}

static RPCHelpMan listachievements()
{
    return RPCHelpMan{"listachievements",
        "\nReturns a list of all achievements (unlocked and locked).\n",
        {
            {"unlocked_only", RPCArg::Type::BOOL, RPCArg::Default{false}, "Show only unlocked achievements"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "id", "Achievement ID"},
                    {RPCResult::Type::STR, "name", "Achievement name"},
                    {RPCResult::Type::STR, "description", "Achievement description"},
                    {RPCResult::Type::NUM, "type", "Achievement type"},
                    {RPCResult::Type::NUM, "rarity", "Achievement rarity (0=Common, 4=Legendary)"},
                    {RPCResult::Type::STR_AMOUNT, "target_value", "Target value to achieve"},
                    {RPCResult::Type::STR_AMOUNT, "current_progress", "Current progress"},
                    {RPCResult::Type::NUM, "progress_percentage", "Progress percentage"},
                    {RPCResult::Type::BOOL, "unlocked", "Whether achievement is unlocked"},
                    {RPCResult::Type::NUM_TIME, "unlock_time", "When achievement was unlocked (0 if locked)"},
                    {RPCResult::Type::STR_AMOUNT, "reward", "Reward amount"},
                    {RPCResult::Type::STR, "nft_token_id", "NFT token ID (if applicable)"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("listachievements", "")
            + HelpExampleCli("listachievements", "true")
            + HelpExampleRpc("listachievements", "false")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    bool unlockedOnly = false;
    if (!request.params[0].isNull()) {
        unlockedOnly = request.params[0].get_bool();
    }
    
    auto& gamificationMgr = pwallet->GetGamificationManager();
    std::vector<Achievement> achievements;
    
    if (unlockedOnly) {
        achievements = gamificationMgr.GetUnlockedAchievements();
    } else {
        auto unlocked = gamificationMgr.GetUnlockedAchievements();
        auto locked = gamificationMgr.GetLockedAchievements();
        achievements.insert(achievements.end(), unlocked.begin(), unlocked.end());
        achievements.insert(achievements.end(), locked.begin(), locked.end());
    }
    
    UniValue result(UniValue::VARR);
    
    for (const auto& achievement : achievements) {
        UniValue obj(UniValue::VOBJ);
        
        obj.pushKV("id", achievement.id.GetHex());
        obj.pushKV("name", achievement.name);
        obj.pushKV("description", achievement.description);
        obj.pushKV("type", static_cast<int>(achievement.type));
        obj.pushKV("rarity", static_cast<int>(achievement.rarity));
        obj.pushKV("target_value", ValueFromAmount(achievement.targetValue));
        obj.pushKV("current_progress", ValueFromAmount(achievement.currentProgress));
        obj.pushKV("progress_percentage", achievement.GetProgressPercentage());
        obj.pushKV("unlocked", achievement.isUnlocked);
        obj.pushKV("unlock_time", achievement.unlockTime);
        obj.pushKV("reward", ValueFromAmount(achievement.rewardAmount));
        obj.pushKV("nft_token_id", achievement.nftTokenId);
        
        result.push_back(obj);
    }
    
    return result;
},
    };
}

static RPCHelpMan listchallenges()
{
    return RPCHelpMan{"listchallenges",
        "\nReturns a list of active challenges.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "id", "Challenge ID"},
                    {RPCResult::Type::STR, "name", "Challenge name"},
                    {RPCResult::Type::STR, "description", "Challenge description"},
                    {RPCResult::Type::NUM, "type", "Challenge type (0=Daily, 1=Weekly, 2=Monthly)"},
                    {RPCResult::Type::NUM, "status", "Challenge status"},
                    {RPCResult::Type::STR_AMOUNT, "target_value", "Target value to complete"},
                    {RPCResult::Type::STR_AMOUNT, "current_progress", "Current progress"},
                    {RPCResult::Type::NUM, "progress_percentage", "Progress percentage"},
                    {RPCResult::Type::NUM_TIME, "start_time", "Challenge start time"},
                    {RPCResult::Type::NUM_TIME, "end_time", "Challenge end time"},
                    {RPCResult::Type::STR_AMOUNT, "reward", "Reward amount"},
                    {RPCResult::Type::NUM, "experience_points", "XP reward"},
                    {RPCResult::Type::STR, "nft_token_id", "NFT reward token ID"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("listchallenges", "")
            + HelpExampleRpc("listchallenges", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    auto challenges = pwallet->GetGamificationManager().GetActiveChallenges();
    
    UniValue result(UniValue::VARR);
    
    for (const auto& challenge : challenges) {
        UniValue obj(UniValue::VOBJ);
        
        obj.pushKV("id", challenge.id.GetHex());
        obj.pushKV("name", challenge.name);
        obj.pushKV("description", challenge.description);
        obj.pushKV("type", static_cast<int>(challenge.type));
        obj.pushKV("status", static_cast<int>(challenge.status));
        obj.pushKV("target_value", ValueFromAmount(challenge.targetValue));
        obj.pushKV("current_progress", ValueFromAmount(challenge.currentProgress));
        obj.pushKV("progress_percentage", challenge.GetProgressPercentage());
        obj.pushKV("start_time", challenge.startTime);
        obj.pushKV("end_time", challenge.endTime);
        obj.pushKV("reward", ValueFromAmount(challenge.rewardAmount));
        obj.pushKV("experience_points", challenge.experiencePoints);
        obj.pushKV("nft_token_id", challenge.nftTokenId);
        
        result.push_back(obj);
    }
    
    return result;
},
    };
}

static RPCHelpMan createpet()
{
    return RPCHelpMan{"createpet",
        "\nCreate a new virtual staking pet.\n",
        {
            {"type", RPCArg::Type::NUM, RPCArg::Optional::NO, "Pet type (0=CryptoCat, 1=StakeDragon, 2=CoinPuppy, 3=ValidatorPhoenix, 4=EarningsEagle)"},
            {"name", RPCArg::Type::STR, RPCArg::Optional::NO, "Pet name"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "pet_id", "Unique pet ID"},
                {RPCResult::Type::STR, "name", "Pet name"},
                {RPCResult::Type::NUM, "type", "Pet type"},
                {RPCResult::Type::NUM, "stage", "Evolution stage"},
                {RPCResult::Type::NUM, "level", "Pet level"},
                {RPCResult::Type::NUM, "happiness", "Pet happiness (0-100)"},
            }
        },
        RPCExamples{
            HelpExampleCli("createpet", "0 \"Fluffy\"")
            + HelpExampleCli("createpet", "1 \"Draco\"")
            + HelpExampleRpc("createpet", "0, \"Fluffy\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    int petTypeInt = request.params[0].get_int();
    if (petTypeInt < 0 || petTypeInt > 4) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid pet type. Must be 0-4.");
    }
    
    PetType petType = static_cast<PetType>(petTypeInt);
    std::string name = request.params[1].get_str();
    
    if (name.empty() || name.length() > 50) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Pet name must be 1-50 characters long.");
    }
    
    auto& gamificationMgr = pwallet->GetGamificationManager();
    uint256 petId = gamificationMgr.CreatePet(petType, name);
    
    auto pet = gamificationMgr.GetPet(petId);
    
    UniValue result(UniValue::VOBJ);
    result.pushKV("pet_id", petId.GetHex());
    result.pushKV("name", pet.name);
    result.pushKV("type", static_cast<int>(pet.type));
    result.pushKV("stage", static_cast<int>(pet.stage));
    result.pushKV("level", pet.level);
    result.pushKV("happiness", pet.happiness);
    
    return result;
},
    };
}

static RPCHelpMan listpets()
{
    return RPCHelpMan{"listpets",
        "\nReturns a list of all virtual staking pets.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "id", "Pet ID"},
                    {RPCResult::Type::STR, "name", "Pet name"},
                    {RPCResult::Type::NUM, "type", "Pet type"},
                    {RPCResult::Type::NUM, "stage", "Evolution stage"},
                    {RPCResult::Type::NUM, "level", "Pet level"},
                    {RPCResult::Type::NUM, "experience", "Experience points"},
                    {RPCResult::Type::NUM, "experience_next_level", "XP needed for next level"},
                    {RPCResult::Type::NUM_TIME, "birth_time", "When pet was created"},
                    {RPCResult::Type::STR_AMOUNT, "total_earnings_contributed", "Total earnings contributed to pet"},
                    {RPCResult::Type::NUM, "happiness", "Happiness level (0-100)"},
                    {RPCResult::Type::NUM_TIME, "last_feed_time", "Last time pet was fed"},
                    {RPCResult::Type::BOOL, "can_evolve", "Whether pet can evolve"},
                    {RPCResult::Type::OBJ, "traits", "Pet traits"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("listpets", "")
            + HelpExampleRpc("listpets", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    auto pets = pwallet->GetGamificationManager().GetAllPets();
    
    UniValue result(UniValue::VARR);
    
    for (const auto& pet : pets) {
        UniValue obj(UniValue::VOBJ);
        
        obj.pushKV("id", pet.id.GetHex());
        obj.pushKV("name", pet.name);
        obj.pushKV("type", static_cast<int>(pet.type));
        obj.pushKV("stage", static_cast<int>(pet.stage));
        obj.pushKV("level", pet.level);
        obj.pushKV("experience", pet.experiencePoints);
        obj.pushKV("experience_next_level", pet.GetExperienceForNextLevel());
        obj.pushKV("birth_time", pet.birthTime);
        obj.pushKV("total_earnings_contributed", ValueFromAmount(pet.totalEarningsContributed));
        obj.pushKV("happiness", pet.happiness);
        obj.pushKV("last_feed_time", pet.lastFeedTime);
        obj.pushKV("can_evolve", pet.CanEvolve());
        
        UniValue traits(UniValue::VOBJ);
        for (const auto& trait : pet.traits) {
            traits.pushKV(trait.first, trait.second);
        }
        obj.pushKV("traits", traits);
        
        result.push_back(obj);
    }
    
    return result;
},
    };
}

static RPCHelpMan evolvepet()
{
    return RPCHelpMan{"evolvepet",
        "\nEvolve a pet to its next stage.\n",
        {
            {"pet_id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Pet ID to evolve"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "success", "Whether evolution was successful"},
                {RPCResult::Type::STR, "message", "Result message"},
                {RPCResult::Type::NUM, "new_stage", "New evolution stage (if successful)"},
            }
        },
        RPCExamples{
            HelpExampleCli("evolvepet", "\"1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef\"")
            + HelpExampleRpc("evolvepet", "\"1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    uint256 petId = ParseHashV(request.params[0], "pet_id");
    
    auto& gamificationMgr = pwallet->GetGamificationManager();
    auto pet = gamificationMgr.GetPet(petId);
    
    if (pet.id.IsNull()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Pet not found");
    }
    
    UniValue result(UniValue::VOBJ);
    
    if (!pet.CanEvolve()) {
        result.pushKV("success", false);
        result.pushKV("message", "Pet cannot evolve yet. Needs higher level.");
        return result;
    }
    
    bool success = gamificationMgr.EvolvePet(petId);
    
    if (success) {
        auto evolvedPet = gamificationMgr.GetPet(petId);
        result.pushKV("success", true);
        result.pushKV("message", "Pet evolved successfully!");
        result.pushKV("new_stage", static_cast<int>(evolvedPet.stage));
    } else {
        result.pushKV("success", false);
        result.pushKV("message", "Evolution failed");
    }
    
    return result;
},
    };
}

static RPCHelpMan listnfts()
{
    return RPCHelpMan{"listnfts",
        "\nReturns a list of owned NFT rewards.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR, "token_id", "NFT token ID"},
                    {RPCResult::Type::STR, "name", "NFT name"},
                    {RPCResult::Type::STR, "description", "NFT description"},
                    {RPCResult::Type::STR, "image_url", "NFT image URL"},
                    {RPCResult::Type::NUM, "rarity", "NFT rarity level"},
                    {RPCResult::Type::NUM_TIME, "mint_time", "When NFT was minted"},
                    {RPCResult::Type::STR_HEX, "achievement_id", "Associated achievement ID"},
                    {RPCResult::Type::OBJ, "attributes", "NFT attributes"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("listnfts", "")
            + HelpExampleRpc("listnfts", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);
    
    auto nfts = pwallet->GetGamificationManager().GetOwnedNFTs();
    
    UniValue result(UniValue::VARR);
    
    for (const auto& nft : nfts) {
        UniValue obj(UniValue::VOBJ);
        
        obj.pushKV("token_id", nft.tokenId);
        obj.pushKV("name", nft.name);
        obj.pushKV("description", nft.description);
        obj.pushKV("image_url", nft.imageUrl);
        obj.pushKV("rarity", static_cast<int>(nft.rarity));
        obj.pushKV("mint_time", nft.mintTime);
        obj.pushKV("achievement_id", nft.achievementId.GetHex());
        
        UniValue attributes(UniValue::VOBJ);
        for (const auto& attr : nft.attributes) {
            attributes.pushKV(attr.first, attr.second);
        }
        obj.pushKV("attributes", attributes);
        
        result.push_back(obj);
    }
    
    return result;
},
    };
}