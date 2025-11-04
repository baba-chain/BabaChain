// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <social/socialfeatures.h>

#include <logging.h>
#include <util/time.h>
#include <util/moneystr.h>
#include <key_io.h>

#include <algorithm>
#include <regex>

CSocialFeatures::CSocialFeatures() :
    referralBonusPercent(5.0),      // 5% bonus for referrals
    minStakeForReferral(100 * COIN), // 100 BABACHAIN minimum
    maxReferralLevels(3),           // 3 levels deep
    socialFeaturesEnabled(true)
{
    InitializeAchievements();
    LogPrint(BCLog::NET, "CSocialFeatures: Initialized social features system\n");
}

CSocialFeatures::~CSocialFeatures()
{
}

bool CSocialFeatures::CreateUserProfile(const CPubKey& pubkey, const std::string& username)
{
    LOCK(cs_social);
    
    if (!IsValidUsername(username)) {
        return false;
    }
    
    // Check if user already exists
    if (userProfiles.find(pubkey) != userProfiles.end()) {
        return false;
    }
    
    // Check if username is already taken
    for (const auto& [key, profile] : userProfiles) {
        if (profile.username == username) {
            return false;
        }
    }
    
    UserProfile profile;
    profile.pubkey = pubkey;
    profile.username = username;
    profile.joinDate = GetTime();
    profile.reputation = 50.0; // Start with neutral reputation
    
    userProfiles[pubkey] = profile;
    
    LogPrint(BCLog::NET, "CSocialFeatures::CreateUserProfile: Created profile for %s (%s)\n",
             username.c_str(), EncodeDestination(PKHash(pubkey)).c_str());
    
    return true;
}

bool CSocialFeatures::UpdateUserProfile(const CPubKey& pubkey, const UserProfile& profile)
{
    LOCK(cs_social);
    
    auto it = userProfiles.find(pubkey);
    if (it == userProfiles.end()) {
        return false;
    }
    
    // Preserve certain fields that shouldn't be changed
    UserProfile updatedProfile = profile;
    updatedProfile.pubkey = pubkey;
    updatedProfile.joinDate = it->second.joinDate;
    updatedProfile.referralCount = it->second.referralCount;
    
    userProfiles[pubkey] = updatedProfile;
    
    LogPrint(BCLog::NET, "CSocialFeatures::UpdateUserProfile: Updated profile for %s\n",
             profile.username.c_str());
    
    return true;
}

UserProfile CSocialFeatures::GetUserProfile(const CPubKey& pubkey) const
{
    LOCK(cs_social);
    
    auto it = userProfiles.find(pubkey);
    if (it != userProfiles.end()) {
        return it->second;
    }
    
    return UserProfile(); // Return empty profile if not found
}

bool CSocialFeatures::UserExists(const CPubKey& pubkey) const
{
    LOCK(cs_social);
    return userProfiles.find(pubkey) != userProfiles.end();
}

bool CSocialFeatures::CreateReferral(const CPubKey& referrer, const CPubKey& referee)
{
    LOCK(cs_social);
    
    if (!socialFeaturesEnabled) {
        return false;
    }
    
    // Check if both users exist
    if (!UserExists(referrer) || !UserExists(referee)) {
        return false;
    }
    
    // Check if referee already has a referrer
    for (const auto& [user, referrals] : userReferrals) {
        for (const auto& referral : referrals) {
            if (referral.referee == referee) {
                return false; // Already referred
            }
        }
    }
    
    ReferralInfo referral;
    referral.referrer = referrer;
    referral.referee = referee;
    referral.referralDate = GetTime();
    referral.isActive = true;
    referral.lastActivityDate = GetTime();
    
    userReferrals[referrer].push_back(referral);
    
    // Update referrer's profile
    userProfiles[referrer].referralCount++;
    
    LogPrint(BCLog::NET, "CSocialFeatures::CreateReferral: %s referred %s\n",
             userProfiles[referrer].username.c_str(), userProfiles[referee].username.c_str());
    
    // Check for referral achievements
    CheckReferralAchievements(referrer);
    
    return true;
}

std::vector<ReferralInfo> CSocialFeatures::GetUserReferrals(const CPubKey& user) const
{
    LOCK(cs_social);
    
    auto it = userReferrals.find(user);
    if (it != userReferrals.end()) {
        return it->second;
    }
    
    return std::vector<ReferralInfo>();
}

CAmount CSocialFeatures::CalculateReferralBonus(const CPubKey& referrer, CAmount stakingAmount) const
{
    LOCK(cs_social);
    
    if (!socialFeaturesEnabled || stakingAmount < minStakeForReferral) {
        return 0;
    }
    
    return stakingAmount * referralBonusPercent / 100.0;
}

void CSocialFeatures::ProcessReferralBonus(const CPubKey& referee, CAmount stakingAmount)
{
    LOCK(cs_social);
    
    if (!socialFeaturesEnabled) {
        return;
    }
    
    // Find who referred this user
    CPubKey referrer;
    bool foundReferrer = false;
    
    for (const auto& [user, referrals] : userReferrals) {
        for (auto& referral : referrals) {
            if (referral.referee == referee) {
                referrer = referral.referrer;
                foundReferrer = true;
                referral.lastActivityDate = GetTime();
                break;
            }
        }
        if (foundReferrer) break;
    }
    
    if (!foundReferrer) {
        return;
    }
    
    CAmount bonus = CalculateReferralBonus(referrer, stakingAmount);
    if (bonus > 0) {
        // Update referral bonus earned
        for (auto& referral : userReferrals[referrer]) {
            if (referral.referee == referee) {
                referral.bonusEarned += bonus;
                break;
            }
        }
        
        LogPrint(BCLog::NET, "CSocialFeatures::ProcessReferralBonus: %s earned %s bonus from %s's stake\n",
                 userProfiles[referrer].username.c_str(), FormatMoney(bonus), 
                 userProfiles[referee].username.c_str());
        
        TriggerReferralBonusCallbacks(referrer, bonus);
    }
}

int CSocialFeatures::GetReferralCount(const CPubKey& user) const
{
    LOCK(cs_social);
    
    auto it = userReferrals.find(user);
    if (it != userReferrals.end()) {
        return it->second.size();
    }
    
    return 0;
}

void CSocialFeatures::InitializeAchievements()
{
    LOCK(cs_social);
    
    availableAchievements.clear();
    
    // Staking achievements
    Achievement firstStake;
    firstStake.id = "first_stake";
    firstStake.name = "First Stake";
    firstStake.description = "Make your first staking transaction";
    firstStake.rewardAmount = 10 * COIN;
    firstStake.isRepeatable = false;
    availableAchievements.push_back(firstStake);
    
    Achievement bigStaker;
    bigStaker.id = "big_staker";
    bigStaker.name = "Big Staker";
    bigStaker.description = "Stake 1,000 BABACHAIN or more";
    bigStaker.rewardAmount = 50 * COIN;
    bigStaker.isRepeatable = false;
    availableAchievements.push_back(bigStaker);
    
    Achievement whaleStaker;
    whaleStaker.id = "whale_staker";
    whaleStaker.name = "Whale Staker";
    whaleStaker.description = "Stake 10,000 BABACHAIN or more";
    whaleStaker.rewardAmount = 500 * COIN;
    whaleStaker.isRepeatable = false;
    availableAchievements.push_back(whaleStaker);
    
    // Referral achievements
    Achievement firstReferral;
    firstReferral.id = "first_referral";
    firstReferral.name = "Recruiter";
    firstReferral.description = "Refer your first user";
    firstReferral.rewardAmount = 25 * COIN;
    firstReferral.isRepeatable = false;
    availableAchievements.push_back(firstReferral);
    
    Achievement superReferrer;
    superReferrer.id = "super_referrer";
    superReferrer.name = "Super Referrer";
    superReferrer.description = "Refer 10 or more users";
    superReferrer.rewardAmount = 250 * COIN;
    superReferrer.isRepeatable = false;
    availableAchievements.push_back(superReferrer);
    
    // Earnings achievements
    Achievement firstEarning;
    firstEarning.id = "first_earning";
    firstEarning.name = "First Reward";
    firstEarning.description = "Earn your first staking reward";
    firstEarning.rewardAmount = 5 * COIN;
    firstEarning.isRepeatable = false;
    availableAchievements.push_back(firstEarning);
    
    Achievement earningsMillionaire;
    earningsMillionaire.id = "earnings_millionaire";
    earningsMillionaire.name = "Earnings Millionaire";
    earningsMillionaire.description = "Earn 1,000,000 BABACHAIN from staking";
    earningsMillionaire.rewardAmount = 10000 * COIN;
    earningsMillionaire.isRepeatable = false;
    availableAchievements.push_back(earningsMillionaire);
    
    // Social achievements
    Achievement socialButterfly;
    socialButterfly.id = "social_butterfly";
    socialButterfly.name = "Social Butterfly";
    socialButterfly.description = "Share 10 achievements on social media";
    socialButterfly.rewardAmount = 100 * COIN;
    socialButterfly.isRepeatable = false;
    availableAchievements.push_back(socialButterfly);
    
    LogPrint(BCLog::NET, "CSocialFeatures::InitializeAchievements: Initialized %d achievements\n",
             availableAchievements.size());
}

bool CSocialFeatures::UnlockAchievement(const CPubKey& user, const std::string& achievementId)
{
    LOCK(cs_social);
    
    if (!UserExists(user)) {
        return false;
    }
    
    // Find the achievement
    Achievement* achievement = nullptr;
    for (auto& ach : availableAchievements) {
        if (ach.id == achievementId) {
            achievement = &ach;
            break;
        }
    }
    
    if (!achievement) {
        return false;
    }
    
    // Check if user already has this achievement (and it's not repeatable)
    auto& userAchs = userAchievements[user];
    for (const auto& userAch : userAchs) {
        if (userAch.id == achievementId && !achievement->isRepeatable) {
            return false; // Already unlocked
        }
    }
    
    // Unlock the achievement
    Achievement unlockedAchievement = *achievement;
    unlockedAchievement.unlockedDate = GetTime();
    userAchs.push_back(unlockedAchievement);
    
    LogPrint(BCLog::NET, "CSocialFeatures::UnlockAchievement: %s unlocked '%s'\n",
             userProfiles[user].username.c_str(), achievement->name.c_str());
    
    TriggerAchievementCallbacks(user, unlockedAchievement);
    
    return true;
}

std::vector<Achievement> CSocialFeatures::GetUserAchievements(const CPubKey& user) const
{
    LOCK(cs_social);
    
    auto it = userAchievements.find(user);
    if (it != userAchievements.end()) {
        return it->second;
    }
    
    return std::vector<Achievement>();
}

std::vector<Achievement> CSocialFeatures::GetAvailableAchievements() const
{
    LOCK(cs_social);
    return availableAchievements;
}

void CSocialFeatures::CheckForNewAchievements(const CPubKey& user)
{
    if (!socialFeaturesEnabled || !UserExists(user)) {
        return;
    }
    
    CheckStakingAchievements(user);
    CheckReferralAchievements(user);
    CheckEarningsAchievements(user);
    CheckSocialAchievements(user);
}

bool CSocialFeatures::ShareAchievement(const CPubKey& user, const std::string& achievementId, const std::string& platform)
{
    LOCK(cs_social);
    
    if (!socialFeaturesEnabled || !UserExists(user)) {
        return false;
    }
    
    // Check if user has this achievement
    bool hasAchievement = false;
    for (const auto& ach : userAchievements[user]) {
        if (ach.id == achievementId) {
            hasAchievement = true;
            break;
        }
    }
    
    if (!hasAchievement) {
        return false;
    }
    
    SocialShare share;
    share.user = user;
    share.platform = platform;
    share.achievementId = achievementId;
    share.content = GenerateShareContent(user, "achievement", achievementId);
    share.shareDate = GetTime();
    
    userShares[user].push_back(share);
    
    LogPrint(BCLog::NET, "CSocialFeatures::ShareAchievement: %s shared achievement '%s' on %s\n",
             userProfiles[user].username.c_str(), achievementId.c_str(), platform.c_str());
    
    // Check for social achievements
    CheckSocialAchievements(user);
    
    return true;
}

bool CSocialFeatures::ShareStakingMilestone(const CPubKey& user, CAmount amount, const std::string& platform)
{
    LOCK(cs_social);
    
    if (!socialFeaturesEnabled || !UserExists(user)) {
        return false;
    }
    
    SocialShare share;
    share.user = user;
    share.platform = platform;
    share.stakingAmount = amount;
    share.content = GenerateShareContent(user, "staking", FormatMoney(amount));
    share.shareDate = GetTime();
    
    userShares[user].push_back(share);
    
    LogPrint(BCLog::NET, "CSocialFeatures::ShareStakingMilestone: %s shared staking milestone of %s on %s\n",
             userProfiles[user].username.c_str(), FormatMoney(amount).c_str(), platform.c_str());
    
    return true;
}

std::vector<SocialShare> CSocialFeatures::GetUserShares(const CPubKey& user) const
{
    LOCK(cs_social);
    
    auto it = userShares.find(user);
    if (it != userShares.end()) {
        return it->second;
    }
    
    return std::vector<SocialShare>();
}

std::vector<SocialShare> CSocialFeatures::GetRecentShares(int count) const
{
    LOCK(cs_social);
    
    std::vector<SocialShare> allShares;
    
    for (const auto& [user, shares] : userShares) {
        for (const auto& share : shares) {
            allShares.push_back(share);
        }
    }
    
    // Sort by date (most recent first)
    std::sort(allShares.begin(), allShares.end(),
        [](const SocialShare& a, const SocialShare& b) {
            return a.shareDate > b.shareDate;
        });
    
    if (allShares.size() > static_cast<size_t>(count)) {
        allShares.resize(count);
    }
    
    return allShares;
}

void CSocialFeatures::UpdateLeaderboards()
{
    LOCK(cs_social);
    
    leaderboards["staking"] = CalculateStakingLeaderboard();
    leaderboards["earnings"] = CalculateEarningsLeaderboard();
    leaderboards["referrals"] = CalculateReferralLeaderboard();
    
    // Trigger callbacks for each leaderboard
    for (const auto& [type, board] : leaderboards) {
        TriggerLeaderboardCallbacks(type, board);
    }
}

std::vector<LeaderboardEntry> CSocialFeatures::GetLeaderboard(const std::string& type, int count) const
{
    LOCK(cs_social);
    
    auto it = leaderboards.find(type);
    if (it != leaderboards.end()) {
        std::vector<LeaderboardEntry> result = it->second;
        if (result.size() > static_cast<size_t>(count)) {
            result.resize(count);
        }
        return result;
    }
    
    return std::vector<LeaderboardEntry>();
}

int CSocialFeatures::GetUserRank(const CPubKey& user, const std::string& type) const
{
    LOCK(cs_social);
    
    auto it = leaderboards.find(type);
    if (it != leaderboards.end()) {
        for (const auto& entry : it->second) {
            if (entry.pubkey == user) {
                return entry.rank;
            }
        }
    }
    
    return 0; // Not ranked
}

bool CSocialFeatures::CreateCompetition(const StakingCompetition& competition)
{
    LOCK(cs_social);
    
    // Check if competition ID already exists
    for (const auto& comp : activeCompetitions) {
        if (comp.id == competition.id) {
            return false;
        }
    }
    
    activeCompetitions.push_back(competition);
    
    LogPrint(BCLog::NET, "CSocialFeatures::CreateCompetition: Created competition '%s'\n",
             competition.name.c_str());
    
    return true;
}

std::vector<StakingCompetition> CSocialFeatures::GetActiveCompetitions() const
{
    LOCK(cs_social);
    
    std::vector<StakingCompetition> active;
    int64_t now = GetTime();
    
    for (const auto& comp : activeCompetitions) {
        if (comp.isActive && now >= comp.startDate && now <= comp.endDate) {
            active.push_back(comp);
        }
    }
    
    return active;
}

std::map<std::string, int> CSocialFeatures::GetSocialStats() const
{
    LOCK(cs_social);
    
    std::map<std::string, int> stats;
    
    stats["total_users"] = userProfiles.size();
    stats["total_referrals"] = 0;
    stats["total_achievements"] = 0;
    stats["total_shares"] = 0;
    
    for (const auto& [user, referrals] : userReferrals) {
        stats["total_referrals"] += referrals.size();
    }
    
    for (const auto& [user, achievements] : userAchievements) {
        stats["total_achievements"] += achievements.size();
    }
    
    for (const auto& [user, shares] : userShares) {
        stats["total_shares"] += shares.size();
    }
    
    return stats;
}

double CSocialFeatures::GetViralityScore() const
{
    LOCK(cs_social);
    
    if (userProfiles.empty()) {
        return 0.0;
    }
    
    int totalUsers = userProfiles.size();
    int totalReferrals = 0;
    int totalShares = 0;
    
    for (const auto& [user, referrals] : userReferrals) {
        totalReferrals += referrals.size();
    }
    
    for (const auto& [user, shares] : userShares) {
        totalShares += shares.size();
    }
    
    // Calculate virality score based on referral rate and sharing activity
    double referralRate = static_cast<double>(totalReferrals) / totalUsers;
    double shareRate = static_cast<double>(totalShares) / totalUsers;
    
    return (referralRate * 0.7 + shareRate * 0.3) * 100.0; // Scale to 0-100
}

void CSocialFeatures::SetReferralBonusPercent(double percent)
{
    referralBonusPercent = std::max(0.0, std::min(50.0, percent)); // Cap at 50%
    LogPrint(BCLog::NET, "CSocialFeatures: Referral bonus set to %.1f%%\n", referralBonusPercent);
}

void CSocialFeatures::SetMinStakeForReferral(CAmount amount)
{
    minStakeForReferral = std::max(CAmount(0), amount);
    LogPrint(BCLog::NET, "CSocialFeatures: Minimum stake for referral set to %s\n", FormatMoney(minStakeForReferral).c_str());
}

void CSocialFeatures::SetSocialFeaturesEnabled(bool enabled)
{
    socialFeaturesEnabled = enabled;
    LogPrint(BCLog::NET, "CSocialFeatures: Social features %s\n", enabled ? "enabled" : "disabled");
}

void CSocialFeatures::RegisterAchievementCallback(const std::function<void(const CPubKey&, const Achievement&)>& callback)
{
    LOCK(cs_social);
    achievementCallbacks.push_back(callback);
}

void CSocialFeatures::RegisterReferralBonusCallback(const std::function<void(const CPubKey&, CAmount)>& callback)
{
    LOCK(cs_social);
    referralBonusCallbacks.push_back(callback);
}

void CSocialFeatures::RegisterLeaderboardCallback(const std::function<void(const std::string&, const std::vector<LeaderboardEntry>&)>& callback)
{
    LOCK(cs_social);
    leaderboardCallbacks.push_back(callback);
}

// Private helper methods implementation continues...
void CSocialFeatures::TriggerAchievementCallbacks(const CPubKey& user, const Achievement& achievement)
{
    for (const auto& callback : achievementCallbacks) {
        try {
            callback(user, achievement);
        } catch (const std::exception& e) {
            LogPrintf("CSocialFeatures: Exception in achievement callback: %s\n", e.what());
        }
    }
}

void CSocialFeatures::TriggerReferralBonusCallbacks(const CPubKey& user, CAmount bonus)
{
    for (const auto& callback : referralBonusCallbacks) {
        try {
            callback(user, bonus);
        } catch (const std::exception& e) {
            LogPrintf("CSocialFeatures: Exception in referral bonus callback: %s\n", e.what());
        }
    }
}

void CSocialFeatures::TriggerLeaderboardCallbacks(const std::string& type, const std::vector<LeaderboardEntry>& leaderboard)
{
    for (const auto& callback : leaderboardCallbacks) {
        try {
            callback(type, leaderboard);
        } catch (const std::exception& e) {
            LogPrintf("CSocialFeatures: Exception in leaderboard callback: %s\n", e.what());
        }
    }
}

void CSocialFeatures::CheckStakingAchievements(const CPubKey& user)
{
    const UserProfile& profile = userProfiles[user];
    
    if (profile.totalStaked > 0) {
        UnlockAchievement(user, "first_stake");
    }
    
    if (profile.totalStaked >= 1000 * COIN) {
        UnlockAchievement(user, "big_staker");
    }
    
    if (profile.totalStaked >= 10000 * COIN) {
        UnlockAchievement(user, "whale_staker");
    }
}

void CSocialFeatures::CheckReferralAchievements(const CPubKey& user)
{
    int referralCount = GetReferralCount(user);
    
    if (referralCount >= 1) {
        UnlockAchievement(user, "first_referral");
    }
    
    if (referralCount >= 10) {
        UnlockAchievement(user, "super_referrer");
    }
}

void CSocialFeatures::CheckEarningsAchievements(const CPubKey& user)
{
    const UserProfile& profile = userProfiles[user];
    
    if (profile.totalEarned > 0) {
        UnlockAchievement(user, "first_earning");
    }
    
    if (profile.totalEarned >= 1000000 * COIN) {
        UnlockAchievement(user, "earnings_millionaire");
    }
}

void CSocialFeatures::CheckSocialAchievements(const CPubKey& user)
{
    int shareCount = GetUserShares(user).size();
    
    if (shareCount >= 10) {
        UnlockAchievement(user, "social_butterfly");
    }
}

std::vector<LeaderboardEntry> CSocialFeatures::CalculateStakingLeaderboard() const
{
    std::vector<LeaderboardEntry> leaderboard;
    
    for (const auto& [pubkey, profile] : userProfiles) {
        LeaderboardEntry entry;
        entry.pubkey = pubkey;
        entry.username = profile.username;
        entry.value = profile.totalStaked;
        leaderboard.push_back(entry);
    }
    
    // Sort by staking amount (descending)
    std::sort(leaderboard.begin(), leaderboard.end(),
        [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            return a.value > b.value;
        });
    
    // Assign ranks
    for (size_t i = 0; i < leaderboard.size(); ++i) {
        leaderboard[i].rank = i + 1;
    }
    
    return leaderboard;
}

std::vector<LeaderboardEntry> CSocialFeatures::CalculateEarningsLeaderboard() const
{
    std::vector<LeaderboardEntry> leaderboard;
    
    for (const auto& [pubkey, profile] : userProfiles) {
        LeaderboardEntry entry;
        entry.pubkey = pubkey;
        entry.username = profile.username;
        entry.value = profile.totalEarned;
        leaderboard.push_back(entry);
    }
    
    // Sort by earnings (descending)
    std::sort(leaderboard.begin(), leaderboard.end(),
        [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            return a.value > b.value;
        });
    
    // Assign ranks
    for (size_t i = 0; i < leaderboard.size(); ++i) {
        leaderboard[i].rank = i + 1;
    }
    
    return leaderboard;
}

std::vector<LeaderboardEntry> CSocialFeatures::CalculateReferralLeaderboard() const
{
    std::vector<LeaderboardEntry> leaderboard;
    
    for (const auto& [pubkey, profile] : userProfiles) {
        LeaderboardEntry entry;
        entry.pubkey = pubkey;
        entry.username = profile.username;
        entry.value = profile.referralCount * COIN; // Convert to CAmount for consistency
        leaderboard.push_back(entry);
    }
    
    // Sort by referral count (descending)
    std::sort(leaderboard.begin(), leaderboard.end(),
        [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            return a.value > b.value;
        });
    
    // Assign ranks
    for (size_t i = 0; i < leaderboard.size(); ++i) {
        leaderboard[i].rank = i + 1;
    }
    
    return leaderboard;
}

bool CSocialFeatures::IsValidUsername(const std::string& username) const
{
    // Username must be 3-20 characters, alphanumeric plus underscore
    if (username.length() < 3 || username.length() > 20) {
        return false;
    }
    
    std::regex validPattern("^[a-zA-Z0-9_]+$");
    return std::regex_match(username, validPattern);
}

std::string CSocialFeatures::GenerateShareContent(const CPubKey& user, const std::string& type, const std::string& data) const
{
    const UserProfile& profile = userProfiles.at(user);
    
    if (type == "achievement") {
        return "🎉 " + profile.username + " just unlocked the '" + data + "' achievement on BabaChain! #BabaChain #Staking #Achievement";
    } else if (type == "staking") {
        return "💰 " + profile.username + " is now staking " + data + " BABACHAIN and earning up to 365% APY! Join the revolution! #BabaChain #Staking #PassiveIncome";
    }
    
    return "Check out BabaChain - the future of staking! #BabaChain";
}