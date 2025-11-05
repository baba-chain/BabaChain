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
 * Challenge definition and progress tracking
 */
struct Challenge {
    uint256 id;                     // Unique challenge ID
    ChallengeType type;             // Challenge type (daily/weekly/monthly)
    ChallengeStatus status;         // Current status
    std::string name;               // Challenge name
    std::string description;        // Challenge description
    CAmount targetValue;            // Target value to achieve
    CAmount currentProgress;        // Current progress towards target
    int64_t startTime;              // Challenge start time
    int64_t endTime;                // Challenge end time
    CAmount rewardAmount;           // Reward amount in coins
    int64_t experienceReward;       // Experience points reward
    std::string badgeId;            // Badge ID if applicable
    
    Challenge() : type(ChallengeType::DAILY), status(ChallengeStatus::ACTIVE),
                 targetValue(0), currentProgress(0), startTime(0), endTime(0),
                 rewardAmount(0), experienceReward(0) {}
    
    SERIALIZE_METHODS(Challenge, obj) {
        READWRITE(obj.id, obj.type, obj.status, obj.name, obj.description);
        READWRITE(obj.targetValue, obj.currentProgress, obj.startTime, obj.endTime);
        READWRITE(obj.rewardAmount, obj.experienceReward, obj.badgeId);
    }
    
    bool IsExpired() const {
        return GetTime() > endTime;
    }
    
    bool IsCompleted() const {
        return currentProgress >= targetValue;
    }
    
    double GetProgressPercentage() const {
        if (targetValue <= 0) return 0.0;
        return std::min(100.0, (static_cast<double>(currentProgress) / static_cast<double>(targetValue)) * 100.0);
    }
};

/**
 * NFT reward structure
 */
struct NFTReward {
    std::string tokenId;            // Unique NFT token ID
    std::string name;               // NFT name
    std::string description;        // NFT description
    std::string imageUrl;           // NFT image URL
    AchievementRarity rarity;       // NFT rarity level
    int64_t mintTime;               // When NFT was minted
    CAmount stakingRequirement;     // Minimum staking requirement to earn
    int64_t stakingDuration;        // Required staking duration in seconds
    bool isOwned;                   // Whether user owns this NFT
    
    NFTReward() : rarity(AchievementRarity::COMMON), mintTime(0),
                 stakingRequirement(0), stakingDuration(0), isOwned(false) {}
    
    SERIALIZE_METHODS(NFTReward, obj) {
        READWRITE(obj.tokenId, obj.name, obj.description, obj.imageUrl);
        READWRITE(obj.rarity, obj.mintTime, obj.stakingRequirement, obj.stakingDuration, obj.isOwned);
    }
};

/**
 * Virtual staking pet structure
 */
struct StakingPet {
    uint256 id;                     // Unique pet ID
    PetType type;                   // Pet type
    PetStage stage;                 // Current evolution stage
    std::string name;               // Pet name (user-defined)
    int64_t birthTime;              // When pet was created
    CAmount totalFed;               // Total earnings fed to pet
    int64_t experience;             // Pet experience points
    int64_t happiness;              // Pet happiness level (0-100)
    int64_t lastFeedTime;           // Last time pet was fed
    std::string imageUrl;           // Pet image URL
    std::vector<std::string> abilities; // Special abilities unlocked
    
    StakingPet() : type(PetType::CRYPTO_CAT), stage(PetStage::EGG),
                  birthTime(0), totalFed(0), experience(0), happiness(50), lastFeedTime(0) {}
    
    SERIALIZE_METHODS(StakingPet, obj) {
        READWRITE(obj.id, obj.type, obj.stage, obj.name, obj.birthTime);
        READWRITE(obj.totalFed, obj.experience, obj.happiness, obj.lastFeedTime, obj.imageUrl, obj.abilities);
    }
    
    bool CanEvolve() const {
        switch (stage) {
            case PetStage::EGG: return experience >= 100;
            case PetStage::BABY: return experience >= 500;
            case PetStage::JUVENILE: return experience >= 2000;
            case PetStage::ADULT: return experience >= 10000;
            case PetStage::LEGENDARY: return false; // Max stage
        }
        return false;
    }
    
    PetStage GetNextStage() const {
        switch (stage) {
            case PetStage::EGG: return PetStage::BABY;
            case PetStage::BABY: return PetStage::JUVENILE;
            case PetStage::JUVENILE: return PetStage::ADULT;
            case PetStage::ADULT: return PetStage::LEGENDARY;
            case PetStage::LEGENDARY: return PetStage::LEGENDARY;
        }
        return stage;
    }
    
    void UpdateHappiness() {
        int64_t timeSinceLastFeed = GetTime() - lastFeedTime;
        int64_t daysSinceLastFeed = timeSinceLastFeed / (24 * 60 * 60);
        
        // Happiness decreases over time if not fed
        if (daysSinceLastFeed > 0) {
            happiness = std::max(0LL, happiness - (daysSinceLastFeed * 5));
        }
    }
};

/**
 * Gamification profile structure
 */
struct GamificationProfile {
    int64_t totalExperience;        // Total experience points earned
    int level;                      // Current level
    int consecutiveStakingDays;     // Current consecutive staking streak
    int maxStakingStreak;           // Maximum staking streak achieved
    CAmount totalEarnings;          // Total earnings from staking
    CAmount totalStaked;            // Total amount ever staked
    int64_t firstStakeTime;         // When user first staked
    int64_t lastStakeTime;          // Last staking activity
    int achievementsUnlocked;       // Number of achievements unlocked
    int challengesCompleted;        // Number of challenges completed
    int nftsOwned;                  // Number of NFTs owned
    int petsOwned;                  // Number of pets owned
    
    GamificationProfile() : totalExperience(0), level(1), consecutiveStakingDays(0),
                           maxStakingStreak(0), totalEarnings(0), totalStaked(0),
                           firstStakeTime(0), lastStakeTime(0), achievementsUnlocked(0),
                           challengesCompleted(0), nftsOwned(0), petsOwned(0) {}
    
    SERIALIZE_METHODS(GamificationProfile, obj) {
        READWRITE(obj.totalExperience, obj.level, obj.consecutiveStakingDays, obj.maxStakingStreak);
        READWRITE(obj.totalEarnings, obj.totalStaked, obj.firstStakeTime, obj.lastStakeTime);
        READWRITE(obj.achievementsUnlocked, obj.challengesCompleted, obj.nftsOwned, obj.petsOwned);
    }
    
    int GetLevelFromExperience() const {
        // Level formula: level = floor(sqrt(experience / 100)) + 1
        return static_cast<int>(std::sqrt(totalExperience / 100.0)) + 1;
    }
    
    int64_t GetExperienceForNextLevel() const {
        int nextLevel = level + 1;
        return (nextLevel - 1) * (nextLevel - 1) * 100;
    }
    
    int64_t GetExperienceToNextLevel() const {
        return GetExperienceForNextLevel() - totalExperience;
    }
};

/**
 * Main gamification system manager
 */
class CGamificationManager
{
private:
    CWallet* wallet;
    std::map<uint256, Achievement> achievements;
    std::map<uint256, Challenge> challenges;
    std::map<uint256, StakingPet> pets;
    std::map<std::string, NFTReward> nftRewards;
    GamificationProfile profile;
    
public:
    explicit CGamificationManager(CWallet* wallet);
    ~CGamificationManager();
    
    // Achievement system
    void InitializeAchievements();
    void CheckAchievements(CAmount stakingAmount, CAmount earnings, int consecutiveDays);
    std::vector<Achievement> GetUnlockedAchievements() const;
    std::vector<Achievement> GetLockedAchievements() const;
    Achievement GetAchievement(const uint256& id) const;
    
    // Challenge system
    void InitializeChallenges();
    void UpdateChallenges();
    void CheckChallengeProgress(CAmount stakingAmount, CAmount earnings);
    std::vector<Challenge> GetActiveChallenges() const;
    std::vector<Challenge> GetCompletedChallenges() const;
    bool CompleteChallenge(const uint256& challengeId);
    void GenerateDailyChallenges();
    void GenerateWeeklyChallenges();
    void GenerateMonthlyChallenges();
    
    // NFT system
    void InitializeNFTRewards();
    std::vector<NFTReward> GetAvailableNFTs() const;
    std::vector<NFTReward> GetOwnedNFTs() const;
    bool MintNFT(const std::string& tokenId, CAmount stakingAmount, int64_t stakingDuration);
    bool CanMintNFT(const std::string& tokenId, CAmount stakingAmount, int64_t stakingDuration) const;
    
    // Pet system
    uint256 CreatePet(PetType type, const std::string& name);
    std::vector<StakingPet> GetAllPets() const;
    StakingPet GetPet(const uint256& petId) const;
    bool FeedPet(const uint256& petId, CAmount earnings);
    bool EvolvePet(const uint256& petId);
    void UpdatePetHappiness();
    
    // Profile and experience system
    GamificationProfile GetProfile() const { return profile; }
    void RecordEarnings(CAmount earnings);
    void UpdateStakingStreak();
    void AddExperience(int64_t xp);
    void RecordStakingActivity(CAmount amount);
    
    // Leaderboard and social features
    struct LeaderboardEntry {
        std::string address;
        CAmount totalStaked;
        CAmount totalEarnings;
        int level;
        int achievementsCount;
        int stakingStreak;
    };
    
    std::vector<LeaderboardEntry> GetStakingLeaderboard(int limit = 100) const;
    std::vector<LeaderboardEntry> GetEarningsLeaderboard(int limit = 100) const;
    
    // Notification system
    struct Notification {
        std::string type;
        std::string title;
        std::string message;
        int64_t timestamp;
        bool isRead;
    };
    
    std::vector<Notification> GetNotifications() const;
    void AddNotification(const std::string& type, const std::string& title, const std::string& message);
    void MarkNotificationRead(int index);

private:
    /** Trigger achievement unlocked notification */
    void TriggerAchievementUnlocked(const Achievement& achievement);
    
    /** Trigger challenge completed notification */
    void TriggerChallengeCompleted(const Challenge& challenge);
    
    /** Trigger pet evolution notification */
    void TriggerPetEvolution(const StakingPet& pet);
    
    /** Trigger NFT minted notification */
    void TriggerNFTMinted(const NFTReward& nft);
    
    /** Update profile level based on experience */
    void UpdateProfileLevel();
    
    /** Calculate experience reward for activity */
    int64_t CalculateExperienceReward(CAmount earnings) const;
    
    /** Storage for notifications */
    mutable std::vector<Notification> notifications;
};

} // namespace wallet

#endif // BITCOIN_WALLET_GAMIFICATION_H