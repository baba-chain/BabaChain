// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SOCIAL_SOCIALFEATURES_H
#define BITCOIN_SOCIAL_SOCIALFEATURES_H

#include <consensus/amount.h>
#include <pubkey.h>
#include <uint256.h>
#include <sync.h>

#include <map>
#include <string>
#include <vector>
#include <functional>

/**
 * User profile information
 */
struct UserProfile {
    CPubKey pubkey;                 // User's public key
    std::string username;           // Display name
    std::string avatar;             // Avatar image hash/URL
    std::string bio;                // User biography
    int64_t joinDate;               // When user joined
    CAmount totalStaked;            // Total amount staked
    CAmount totalEarned;            // Total earnings from staking
    int referralCount;              // Number of successful referrals
    double reputation;              // User reputation score (0-100)
    bool isVerified;                // Whether user is verified
    
    UserProfile() : joinDate(0), totalStaked(0), totalEarned(0), referralCount(0), 
                   reputation(50.0), isVerified(false) {}
};

/**
 * Referral system information
 */
struct ReferralInfo {
    CPubKey referrer;               // Who made the referral
    CPubKey referee;                // Who was referred
    int64_t referralDate;           // When referral was made
    bool isActive;                  // Whether referee is active
    CAmount bonusEarned;            // Bonus earned from this referral
    int64_t lastActivityDate;       // Last activity from referee
    
    ReferralInfo() : referralDate(0), isActive(false), bonusEarned(0), lastActivityDate(0) {}
};

/**
 * Achievement information
 */
struct Achievement {
    std::string id;                 // Achievement ID
    std::string name;               // Achievement name
    std::string description;        // Achievement description
    std::string icon;               // Achievement icon
    CAmount rewardAmount;           // Reward for achieving this
    bool isRepeatable;              // Whether achievement can be earned multiple times
    int64_t unlockedDate;           // When achievement was unlocked (0 if not unlocked)
    
    Achievement() : rewardAmount(0), isRepeatable(false), unlockedDate(0) {}
};

/**
 * Leaderboard entry
 */
struct LeaderboardEntry {
    CPubKey pubkey;                 // User's public key
    std::string username;           // Display name
    CAmount value;                  // Value for ranking (earnings, stake, etc.)
    int rank;                       // Current rank
    int previousRank;               // Previous rank (for change tracking)
    double changePercent;           // Percentage change from previous period
    
    LeaderboardEntry() : value(0), rank(0), previousRank(0), changePercent(0.0) {}
};

/**
 * Social sharing information
 */
struct SocialShare {
    CPubKey user;                   // User who shared
    std::string platform;           // Platform (twitter, facebook, etc.)
    std::string content;            // Shared content
    std::string achievementId;      // Achievement being shared (if any)
    CAmount stakingAmount;          // Staking amount being shared (if any)
    int64_t shareDate;              // When content was shared
    int likes;                      // Number of likes/reactions
    int shares;                     // Number of reshares
    
    SocialShare() : stakingAmount(0), shareDate(0), likes(0), shares(0) {}
};

/**
 * Competition information
 */
struct StakingCompetition {
    std::string id;                 // Competition ID
    std::string name;               // Competition name
    std::string description;        // Competition description
    int64_t startDate;              // Competition start date
    int64_t endDate;                // Competition end date
    CAmount prizePool;              // Total prize pool
    std::string competitionType;    // Type (earnings, referrals, etc.)
    std::vector<LeaderboardEntry> leaderboard; // Current standings
    bool isActive;                  // Whether competition is currently active
    
    StakingCompetition() : startDate(0), endDate(0), prizePool(0), isActive(false) {}
};

/**
 * Social features and viral growth system
 */
class CSocialFeatures
{
private:
    mutable RecursiveMutex cs_social;
    
    // User data
    std::map<CPubKey, UserProfile> userProfiles GUARDED_BY(cs_social);
    std::map<CPubKey, std::vector<ReferralInfo>> userReferrals GUARDED_BY(cs_social);
    std::map<CPubKey, std::vector<Achievement>> userAchievements GUARDED_BY(cs_social);
    std::map<CPubKey, std::vector<SocialShare>> userShares GUARDED_BY(cs_social);
    
    // System data
    std::vector<Achievement> availableAchievements GUARDED_BY(cs_social);
    std::vector<StakingCompetition> activeCompetitions GUARDED_BY(cs_social);
    std::map<std::string, std::vector<LeaderboardEntry>> leaderboards GUARDED_BY(cs_social);
    
    // Configuration
    double referralBonusPercent;    // Percentage bonus for referrals
    CAmount minStakeForReferral;    // Minimum stake to qualify for referral bonus
    int maxReferralLevels;          // Maximum referral chain levels
    bool socialFeaturesEnabled;     // Whether social features are enabled
    
    // Callbacks
    std::vector<std::function<void(const CPubKey&, const Achievement&)>> achievementCallbacks;
    std::vector<std::function<void(const CPubKey&, CAmount)>> referralBonusCallbacks;
    std::vector<std::function<void(const std::string&, const std::vector<LeaderboardEntry>&)>> leaderboardCallbacks;
    
public:
    CSocialFeatures();
    ~CSocialFeatures();
    
    /** User profile management */
    bool CreateUserProfile(const CPubKey& pubkey, const std::string& username);
    bool UpdateUserProfile(const CPubKey& pubkey, const UserProfile& profile);
    UserProfile GetUserProfile(const CPubKey& pubkey) const;
    bool UserExists(const CPubKey& pubkey) const;
    
    /** Referral system */
    bool CreateReferral(const CPubKey& referrer, const CPubKey& referee);
    std::vector<ReferralInfo> GetUserReferrals(const CPubKey& user) const;
    CAmount CalculateReferralBonus(const CPubKey& referrer, CAmount stakingAmount) const;
    void ProcessReferralBonus(const CPubKey& referee, CAmount stakingAmount);
    int GetReferralCount(const CPubKey& user) const;
    
    /** Achievement system */
    void InitializeAchievements();
    bool UnlockAchievement(const CPubKey& user, const std::string& achievementId);
    std::vector<Achievement> GetUserAchievements(const CPubKey& user) const;
    std::vector<Achievement> GetAvailableAchievements() const;
    void CheckForNewAchievements(const CPubKey& user);
    
    /** Social sharing */
    bool ShareAchievement(const CPubKey& user, const std::string& achievementId, const std::string& platform);
    bool ShareStakingMilestone(const CPubKey& user, CAmount amount, const std::string& platform);
    std::vector<SocialShare> GetUserShares(const CPubKey& user) const;
    std::vector<SocialShare> GetRecentShares(int count = 10) const;
    
    /** Leaderboards */
    void UpdateLeaderboards();
    std::vector<LeaderboardEntry> GetLeaderboard(const std::string& type, int count = 100) const;
    int GetUserRank(const CPubKey& user, const std::string& type) const;
    
    /** Competitions */
    bool CreateCompetition(const StakingCompetition& competition);
    std::vector<StakingCompetition> GetActiveCompetitions() const;
    bool JoinCompetition(const CPubKey& user, const std::string& competitionId);
    void UpdateCompetitionStandings();
    void EndCompetition(const std::string& competitionId);
    
    /** Statistics and analytics */
    std::map<std::string, int> GetSocialStats() const;
    double GetViralityScore() const;
    std::vector<CPubKey> GetMostActiveUsers(int count = 10) const;
    std::vector<CPubKey> GetTopReferrers(int count = 10) const;
    
    /** Notification and rewards */
    void NotifyAchievementUnlocked(const CPubKey& user, const Achievement& achievement);
    void NotifyReferralBonus(const CPubKey& user, CAmount bonus);
    void NotifyLeaderboardChange(const CPubKey& user, const std::string& type, int newRank, int oldRank);
    
    /** Configuration */
    void SetReferralBonusPercent(double percent);
    void SetMinStakeForReferral(CAmount amount);
    void SetMaxReferralLevels(int levels);
    void SetSocialFeaturesEnabled(bool enabled);
    
    /** Callbacks */
    void RegisterAchievementCallback(const std::function<void(const CPubKey&, const Achievement&)>& callback);
    void RegisterReferralBonusCallback(const std::function<void(const CPubKey&, CAmount)>& callback);
    void RegisterLeaderboardCallback(const std::function<void(const std::string&, const std::vector<LeaderboardEntry>&)>& callback);
    
    /** Reputation system */
    void UpdateUserReputation(const CPubKey& user, double change);
    double CalculateReputationScore(const CPubKey& user) const;
    
    /** Community features */
    std::vector<std::string> GetTrendingTopics() const;
    std::vector<SocialShare> GetFeaturedContent() const;
    
private:
    /** Helper methods */
    void TriggerAchievementCallbacks(const CPubKey& user, const Achievement& achievement);
    void TriggerReferralBonusCallbacks(const CPubKey& user, CAmount bonus);
    void TriggerLeaderboardCallbacks(const std::string& type, const std::vector<LeaderboardEntry>& leaderboard);
    
    /** Achievement checking */
    void CheckStakingAchievements(const CPubKey& user);
    void CheckReferralAchievements(const CPubKey& user);
    void CheckEarningsAchievements(const CPubKey& user);
    void CheckSocialAchievements(const CPubKey& user);
    
    /** Leaderboard calculation */
    std::vector<LeaderboardEntry> CalculateStakingLeaderboard() const;
    std::vector<LeaderboardEntry> CalculateEarningsLeaderboard() const;
    std::vector<LeaderboardEntry> CalculateReferralLeaderboard() const;
    
    /** Utility methods */
    bool IsValidUsername(const std::string& username) const;
    std::string GenerateShareContent(const CPubKey& user, const std::string& type, const std::string& data) const;
    double CalculateUserActivity(const CPubKey& user) const;
};

#endif // BITCOIN_SOCIAL_SOCIALFEATURES_H