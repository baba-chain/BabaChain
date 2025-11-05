// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_GAMIFICATION_H
#define BITCOIN_WALLET_GAMIFICATION_H

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <serialize.h>
#include <util/time.h>

#include <map>
#include <vector>
#include <string>
#include <functional>

namespace wallet {

class CWallet;

/** Achievement types */
enum class AchievementType : uint8_t {
    FIRST_STAKE = 0,
    STAKE_MILESTONE = 1,
    CONSECUTIVE_DAYS = 2,
    EARNINGS_MILESTONE = 3,
    VALIDATOR_REGISTRATION = 4,
    LONG_TERM_STAKER = 5,
    HIGH_APY = 6,
    NETWORK_CONTRIBUTION = 7,
    SOCIAL_REFERRAL = 8,
    CHALLENGE_COMPLETION = 9
};

/** Achievement rarity levels */
enum class AchievementRarity : uint8_t {
    COMMON = 0,
    UNCOMMON = 1,
    RARE = 2,
    EPIC = 3,
    LEGENDARY = 4
};

/** Challenge types */
enum class ChallengeType : uint8_t {
    DAILY = 0,
    WEEKLY = 1,
    MONTHLY = 2,
    SPECIAL_EVENT = 3
};

/** Challenge status */
enum class ChallengeStatus : uint8_t {
    ACTIVE = 0,
    COMPLETED = 1,
    EXPIRED = 2,
    CLAIMED = 3
};

/** Pet types */
enum class PetType : uint8_t {
    CRYPTO_CAT = 0,
    STAKE_DRAGON = 1,
    COIN_PUPPY = 2,
    VALIDATOR_PHOENIX = 3,
    EARNINGS_EAGLE = 4
};

/** Pet evolution stages */
enum class PetStage : uint8_t {
    EGG = 0,
    BABY = 1,
    JUVENILE = 2,
    ADULT = 3,
    LEGENDARY = 4
};

/**
 * Achievement definition and progress tracking
 */
struct Achievement {
    uint256 id;                     // Unique achievement ID
    AchievementType type;           // Achievement type
    AchievementRarity rarity;       // Rarity level
    std::string name;               // Achievement name
    std::string description;        // Achievement description
    std::string iconUrl;            // Icon URL or path
    CAmount targetValue;            // Target value to achieve
    CAmount currentProgress;        // Current progress towards target
    int64_t unlockTime;             // When achievement was unlocked (0 if not unlocked)
    bool isUnlocked;                // Whether achievement is unlocked
    CAmount rewardAmount;           // Reward amount in coins
    std::string nftTokenId;         // NFT token ID if applicable
    
    Achievement() : type(AchievementType::FIRST_STAKE), rarity(AchievementRarity::COMMON),
                   targetValue(0), currentProgress(0), unlockTime(0), isUnlocked(false),
                   rewardAmount(0) {}
    
    SERIALIZE_METHODS(Achievement, obj) {
        READWRITE(obj.id, obj.type, obj.rarity, obj.name, obj.description, obj.iconUrl);
        READWRITE(obj.targetValue, obj.currentProgress, obj.unlockTime, obj.isUnlocked);
        READWRITE(obj.rewardAmount, obj.nftTokenId);
    }
    
    double GetProgressPercentage() const {
        if (targetValue <= 0) return 0.0;
        return std::min(100.0, (static_cast<double>(currentProgress) / static_cast<double>(targetValue)) * 100.0);
    }
};

/**
 * Main gamification system manager
 */
class CGamificationManager
{
private:
    CWallet* wallet;
    
public:
    explicit CGamificationManager(CWallet* wallet);
    ~CGamificationManager();
    
    // Achievement system
    void InitializeAchievements();
    void CheckAchievements(CAmount stakingAmount, CAmount earnings, int consecutiveDays);
    std::vector<Achievement> GetUnlockedAchievements() const;
    std::vector<Achievement> GetLockedAchievements() const;
    
    // Placeholder methods for compilation
    struct GamificationProfile {
        int consecutiveStakingDays = 0;
    };
    
    GamificationProfile GetProfile() const { return GamificationProfile(); }
    void RecordEarnings(CAmount earnings) {}
    void UpdateStakingStreak() {}
    void AddExperience(int64_t xp) {}
    
    struct StakingPet {
        uint256 id;
        std::string name;
    };
    
    std::vector<StakingPet> GetAllPets() const { return {}; }
    uint256 CreatePet(PetType type, const std::string& name) { return uint256(); }
    void FeedPet(const uint256& petId, CAmount earnings) {}
    
    struct NFTReward {
        std::string tokenId;
    };
    
    std::vector<NFTReward> GetOwnedNFTs() const { return {}; }
};

} // namespace wallet

#endif // BITCOIN_WALLET_GAMIFICATION_H