// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/gamification.h>

#include <wallet/wallet.h>
#include <logging.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <tinyformat.h>

namespace wallet {

CGamificationManager::CGamificationManager(CWallet* wallet) : wallet(wallet)
{
    InitializeAchievements();
}

CGamificationManager::~CGamificationManager()
{
}

void CGamificationManager::InitializeAchievements()
{
    achievements.clear();
    
    // First Stake Achievement
    Achievement firstStake;
    firstStake.id = uint256S("0x1");
    firstStake.type = AchievementType::FIRST_STAKE;
    firstStake.rarity = AchievementRarity::COMMON;
    firstStake.name = "First Steps";
    firstStake.description = "Make your first stake in BabaChain";
    firstStake.targetValue = 1 * COIN;
    firstStake.rewardAmount = 10 * COIN;
    achievements[firstStake.id] = firstStake;
    
    // Stake Milestones
    std::vector<std::pair<CAmount, std::string>> stakeMilestones = {
        {100 * COIN, "Hundred Club"},
        {1000 * COIN, "Thousand Strong"},
        {10000 * COIN, "Ten Thousand Elite"},
        {100000 * COIN, "Hundred Thousand Legend"}
    };
    
    for (size_t i = 0; i < stakeMilestones.size(); ++i) {
        Achievement milestone;
        milestone.id = uint256S(strprintf("0x%d", 10 + i));
        milestone.type = AchievementType::STAKE_MILESTONE;
        milestone.rarity = static_cast<AchievementRarity>(i + 1);
        milestone.name = stakeMilestones[i].second;
        milestone.description = strprintf("Stake %s BabaChain", FormatMoney(stakeMilestones[i].first));
        milestone.targetValue = stakeMilestones[i].first;
        milestone.rewardAmount = stakeMilestones[i].first / 100; // 1% bonus
        achievements[milestone.id] = milestone;
    }
    
    // Consecutive Days Achievements
    std::vector<std::pair<int, std::string>> dayMilestones = {
        {7, "Week Warrior"},
        {30, "Monthly Master"},
        {100, "Hundred Day Hero"},
        {365, "Year Long Legend"}
    };
    
    for (size_t i = 0; i < dayMilestones.size(); ++i) {
        Achievement dayAchievement;
        dayAchievement.id = uint256S(strprintf("0x%d", 20 + i));
        dayAchievement.type = AchievementType::CONSECUTIVE_DAYS;
        dayAchievement.rarity = static_cast<AchievementRarity>(i + 1);
        dayAchievement.name = dayMilestones[i].second;
        dayAchievement.description = strprintf("Stake for %d consecutive days", dayMilestones[i].first);
        dayAchievement.targetValue = dayMilestones[i].first;
        dayAchievement.rewardAmount = dayMilestones[i].first * COIN; // 1 coin per day
        achievements[dayAchievement.id] = dayAchievement;
    }
    
    // Earnings Milestones
    std::vector<std::pair<CAmount, std::string>> earningsMilestones = {
        {100 * COIN, "First Hundred Earned"},
        {1000 * COIN, "Thousand Earner"},
        {10000 * COIN, "Ten Thousand Profit"},
        {100000 * COIN, "Hundred Thousand Mogul"}
    };
    
    for (size_t i = 0; i < earningsMilestones.size(); ++i) {
        Achievement earnings;
        earnings.id = uint256S(strprintf("0x%d", 30 + i));
        earnings.type = AchievementType::EARNINGS_MILESTONE;
        earnings.rarity = static_cast<AchievementRarity>(i + 1);
        earnings.name = earningsMilestones[i].second;
        earnings.description = strprintf("Earn %s from staking", FormatMoney(earningsMilestones[i].first));
        earnings.targetValue = earningsMilestones[i].first;
        earnings.rewardAmount = earningsMilestones[i].first / 50; // 2% bonus
        achievements[earnings.id] = earnings;
    }
    
    LogPrint(BCLog::WALLET, "CGamificationManager::%s: Initialized %d achievements\n", 
             __func__, achievements.size());
}

void CGamificationManager::CheckAchievements(CAmount stakingAmount, CAmount earnings, int consecutiveDays)
{
    std::vector<uint256> newlyUnlocked;
    
    for (auto& [id, achievement] : achievements) {
        if (achievement.isUnlocked) {
            continue; // Already unlocked
        }
        
        bool shouldUnlock = false;
        
        switch (achievement.type) {
            case AchievementType::FIRST_STAKE:
                achievement.currentProgress = stakingAmount;
                shouldUnlock = (stakingAmount >= achievement.targetValue);
                break;
                
            case AchievementType::STAKE_MILESTONE:
                achievement.currentProgress = stakingAmount;
                shouldUnlock = (stakingAmount >= achievement.targetValue);
                break;
                
            case AchievementType::CONSECUTIVE_DAYS:
                achievement.currentProgress = consecutiveDays;
                shouldUnlock = (consecutiveDays >= achievement.targetValue);
                break;
                
            case AchievementType::EARNINGS_MILESTONE:
                achievement.currentProgress = earnings;
                shouldUnlock = (earnings >= achievement.targetValue);
                break;
                
            default:
                break;
        }
        
        if (shouldUnlock) {
            achievement.isUnlocked = true;
            achievement.unlockTime = GetTime();
            newlyUnlocked.push_back(id);
            
            LogPrintf("CGamificationManager::%s: Achievement unlocked: %s\n", 
                     __func__, achievement.name);
        }
    }
    
    // Trigger notifications for newly unlocked achievements
    for (const auto& achievementId : newlyUnlocked) {
        TriggerAchievementUnlocked(achievements[achievementId]);
    }
    
    if (!newlyUnlocked.empty()) {
        LogPrint(BCLog::WALLET, "CGamificationManager::%s: %d new achievements unlocked\n", 
                 __func__, newlyUnlocked.size());
    }
}

std::vector<Achievement> CGamificationManager::GetUnlockedAchievements() const
{
    std::vector<Achievement> unlocked;
    
    for (const auto& [id, achievement] : achievements) {
        if (achievement.isUnlocked) {
            unlocked.push_back(achievement);
        }
    }
    
    return unlocked;
}

std::vector<Achievement> CGamificationManager::GetLockedAchievements() const
{
    std::vector<Achievement> locked;
    
    for (const auto& [id, achievement] : achievements) {
        if (!achievement.isUnlocked) {
            locked.push_back(achievement);
        }
    }
    
    return locked;
}

void CGamificationManager::TriggerAchievementUnlocked(const Achievement& achievement)
{
    // In a real implementation, this would trigger UI notifications,
    // sound effects, and potentially award the achievement reward
    LogPrintf("CGamificationManager::%s: 🏆 Achievement Unlocked: %s - %s\n", 
             __func__, achievement.name, achievement.description);
    
    // Award the achievement reward if applicable
    if (achievement.rewardAmount > 0 && wallet) {
        // In a real implementation, this would add the reward to the wallet
        LogPrintf("CGamificationManager::%s: Awarded %s for achievement %s\n", 
                 __func__, FormatMoney(achievement.rewardAmount), achievement.name);
    }
}

} // namespace wallet