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
    InitializeChallenges();
    InitializeNFTRewards();
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

void CGamificationManager::InitializeChallenges()
{
    challenges.clear();
    
    // Generate initial challenges
    GenerateDailyChallenges();
    GenerateWeeklyChallenges();
    GenerateMonthlyChallenges();
    
    LogPrint(BCLog::WALLET, "CGamificationManager::%s: Initialized %d challenges\n", 
             __func__, challenges.size());
}

void CGamificationManager::InitializeNFTRewards()
{
    nftRewards.clear();
    
    // Long-term staker NFTs
    std::vector<std::tuple<std::string, std::string, CAmount, int64_t, AchievementRarity>> nftData = {
        {"LTST_BRONZE", "Bronze Long-Term Staker", 1000 * COIN, 30 * 24 * 60 * 60, AchievementRarity::COMMON},
        {"LTST_SILVER", "Silver Long-Term Staker", 5000 * COIN, 90 * 24 * 60 * 60, AchievementRarity::UNCOMMON},
        {"LTST_GOLD", "Gold Long-Term Staker", 10000 * COIN, 180 * 24 * 60 * 60, AchievementRarity::RARE},
        {"LTST_PLATINUM", "Platinum Long-Term Staker", 50000 * COIN, 365 * 24 * 60 * 60, AchievementRarity::EPIC},
        {"LTST_DIAMOND", "Diamond Long-Term Staker", 100000 * COIN, 730 * 24 * 60 * 60, AchievementRarity::LEGENDARY}
    };
    
    for (const auto& [tokenId, name, requirement, duration, rarity] : nftData) {
        NFTReward nft;
        nft.tokenId = tokenId;
        nft.name = name;
        nft.description = strprintf("Exclusive NFT for staking %s for %d days", 
                                   FormatMoney(requirement), duration / (24 * 60 * 60));
        nft.imageUrl = strprintf("https://babachain.org/nft/%s.png", tokenId);
        nft.rarity = rarity;
        nft.stakingRequirement = requirement;
        nft.stakingDuration = duration;
        nft.isOwned = false;
        
        nftRewards[tokenId] = nft;
    }
    
    LogPrint(BCLog::WALLET, "CGamificationManager::%s: Initialized %d NFT rewards\n", 
             __func__, nftRewards.size());
}

void CGamificationManager::UpdateChallenges()
{
    int64_t currentTime = GetTime();
    std::vector<uint256> expiredChallenges;
    
    // Check for expired challenges
    for (auto& [id, challenge] : challenges) {
        if (challenge.IsExpired() && challenge.status == ChallengeStatus::ACTIVE) {
            challenge.status = ChallengeStatus::EXPIRED;
            expiredChallenges.push_back(id);
        }
    }
    
    // Generate new challenges if needed
    static int64_t lastDailyGeneration = 0;
    static int64_t lastWeeklyGeneration = 0;
    static int64_t lastMonthlyGeneration = 0;
    
    int64_t daysSinceEpoch = currentTime / (24 * 60 * 60);
    int64_t weeksSinceEpoch = currentTime / (7 * 24 * 60 * 60);
    int64_t monthsSinceEpoch = currentTime / (30 * 24 * 60 * 60);
    
    if (daysSinceEpoch > lastDailyGeneration) {
        GenerateDailyChallenges();
        lastDailyGeneration = daysSinceEpoch;
    }
    
    if (weeksSinceEpoch > lastWeeklyGeneration) {
        GenerateWeeklyChallenges();
        lastWeeklyGeneration = weeksSinceEpoch;
    }
    
    if (monthsSinceEpoch > lastMonthlyGeneration) {
        GenerateMonthlyChallenges();
        lastMonthlyGeneration = monthsSinceEpoch;
    }
}

void CGamificationManager::CheckChallengeProgress(CAmount stakingAmount, CAmount earnings)
{
    for (auto& [id, challenge] : challenges) {
        if (challenge.status != ChallengeStatus::ACTIVE) {
            continue;
        }
        
        bool progressMade = false;
        
        // Update progress based on challenge type and current activity
        if (challenge.name.find("Stake") != std::string::npos) {
            challenge.currentProgress += stakingAmount;
            progressMade = true;
        } else if (challenge.name.find("Earn") != std::string::npos) {
            challenge.currentProgress += earnings;
            progressMade = true;
        }
        
        // Check if challenge is completed
        if (challenge.IsCompleted() && challenge.status == ChallengeStatus::ACTIVE) {
            challenge.status = ChallengeStatus::COMPLETED;
            TriggerChallengeCompleted(challenge);
            
            // Award experience and rewards
            AddExperience(challenge.experienceReward);
            if (challenge.rewardAmount > 0) {
                // In a real implementation, add coins to wallet
                LogPrintf("CGamificationManager::%s: Challenge reward: %s\n", 
                         __func__, FormatMoney(challenge.rewardAmount));
            }
        }
    }
}

void CGamificationManager::GenerateDailyChallenges()
{
    int64_t currentTime = GetTime();
    int64_t dayStart = (currentTime / (24 * 60 * 60)) * (24 * 60 * 60);
    int64_t dayEnd = dayStart + (24 * 60 * 60);
    
    // Daily staking challenge
    Challenge dailyStake;
    dailyStake.id = uint256S(strprintf("0xD%d", dayStart));
    dailyStake.type = ChallengeType::DAILY;
    dailyStake.status = ChallengeStatus::ACTIVE;
    dailyStake.name = "Daily Staker";
    dailyStake.description = "Stake 100 BabaChain today";
    dailyStake.targetValue = 100 * COIN;
    dailyStake.currentProgress = 0;
    dailyStake.startTime = dayStart;
    dailyStake.endTime = dayEnd;
    dailyStake.rewardAmount = 5 * COIN;
    dailyStake.experienceReward = 50;
    
    challenges[dailyStake.id] = dailyStake;
    
    // Daily earnings challenge
    Challenge dailyEarnings;
    dailyEarnings.id = uint256S(strprintf("0xDE%d", dayStart));
    dailyEarnings.type = ChallengeType::DAILY;
    dailyEarnings.status = ChallengeStatus::ACTIVE;
    dailyEarnings.name = "Daily Earner";
    dailyEarnings.description = "Earn 10 BabaChain from staking today";
    dailyEarnings.targetValue = 10 * COIN;
    dailyEarnings.currentProgress = 0;
    dailyEarnings.startTime = dayStart;
    dailyEarnings.endTime = dayEnd;
    dailyEarnings.rewardAmount = 2 * COIN;
    dailyEarnings.experienceReward = 30;
    
    challenges[dailyEarnings.id] = dailyEarnings;
}

void CGamificationManager::GenerateWeeklyChallenges()
{
    int64_t currentTime = GetTime();
    int64_t weekStart = (currentTime / (7 * 24 * 60 * 60)) * (7 * 24 * 60 * 60);
    int64_t weekEnd = weekStart + (7 * 24 * 60 * 60);
    
    // Weekly staking challenge
    Challenge weeklyStake;
    weeklyStake.id = uint256S(strprintf("0xW%d", weekStart));
    weeklyStake.type = ChallengeType::WEEKLY;
    weeklyStake.status = ChallengeStatus::ACTIVE;
    weeklyStake.name = "Weekly Warrior";
    weeklyStake.description = "Stake 1000 BabaChain this week";
    weeklyStake.targetValue = 1000 * COIN;
    weeklyStake.currentProgress = 0;
    weeklyStake.startTime = weekStart;
    weeklyStake.endTime = weekEnd;
    weeklyStake.rewardAmount = 50 * COIN;
    weeklyStake.experienceReward = 500;
    
    challenges[weeklyStake.id] = weeklyStake;
}

void CGamificationManager::GenerateMonthlyChallenges()
{
    int64_t currentTime = GetTime();
    int64_t monthStart = (currentTime / (30 * 24 * 60 * 60)) * (30 * 24 * 60 * 60);
    int64_t monthEnd = monthStart + (30 * 24 * 60 * 60);
    
    // Monthly staking challenge
    Challenge monthlyStake;
    monthlyStake.id = uint256S(strprintf("0xM%d", monthStart));
    monthlyStake.type = ChallengeType::MONTHLY;
    monthlyStake.status = ChallengeStatus::ACTIVE;
    monthlyStake.name = "Monthly Master";
    monthlyStake.description = "Stake 10000 BabaChain this month";
    monthlyStake.targetValue = 10000 * COIN;
    monthlyStake.currentProgress = 0;
    monthlyStake.startTime = monthStart;
    monthlyStake.endTime = monthEnd;
    monthlyStake.rewardAmount = 500 * COIN;
    monthlyStake.experienceReward = 2000;
    
    challenges[monthlyStake.id] = monthlyStake;
}

std::vector<Challenge> CGamificationManager::GetActiveChallenges() const
{
    std::vector<Challenge> active;
    
    for (const auto& [id, challenge] : challenges) {
        if (challenge.status == ChallengeStatus::ACTIVE && !challenge.IsExpired()) {
            active.push_back(challenge);
        }
    }
    
    return active;
}

std::vector<Challenge> CGamificationManager::GetCompletedChallenges() const
{
    std::vector<Challenge> completed;
    
    for (const auto& [id, challenge] : challenges) {
        if (challenge.status == ChallengeStatus::COMPLETED || challenge.status == ChallengeStatus::CLAIMED) {
            completed.push_back(challenge);
        }
    }
    
    return completed;
}

bool CGamificationManager::CompleteChallenge(const uint256& challengeId)
{
    auto it = challenges.find(challengeId);
    if (it == challenges.end()) {
        return false;
    }
    
    Challenge& challenge = it->second;
    if (challenge.status != ChallengeStatus::COMPLETED) {
        return false;
    }
    
    challenge.status = ChallengeStatus::CLAIMED;
    profile.challengesCompleted++;
    
    AddNotification("challenge", "Challenge Completed!", 
                   strprintf("You completed the challenge: %s", challenge.name));
    
    return true;
}

uint256 CGamificationManager::CreatePet(PetType type, const std::string& name)
{
    StakingPet pet;
    pet.id = uint256S(strprintf("0xPET%d%d", static_cast<int>(type), GetTime()));
    pet.type = type;
    pet.stage = PetStage::EGG;
    pet.name = name;
    pet.birthTime = GetTime();
    pet.totalFed = 0;
    pet.experience = 0;
    pet.happiness = 100; // Start with full happiness
    pet.lastFeedTime = GetTime();
    
    // Set pet image based on type and stage
    std::string typeStr;
    switch (type) {
        case PetType::CRYPTO_CAT: typeStr = "cat"; break;
        case PetType::STAKE_DRAGON: typeStr = "dragon"; break;
        case PetType::COIN_PUPPY: typeStr = "puppy"; break;
        case PetType::VALIDATOR_PHOENIX: typeStr = "phoenix"; break;
        case PetType::EARNINGS_EAGLE: typeStr = "eagle"; break;
    }
    pet.imageUrl = strprintf("https://babachain.org/pets/%s_egg.png", typeStr);
    
    pets[pet.id] = pet;
    profile.petsOwned++;
    
    AddNotification("pet", "New Pet Created!", 
                   strprintf("You created a new %s named %s!", typeStr, name));
    
    LogPrintf("CGamificationManager::%s: Created pet %s (type: %d)\n", 
             __func__, name, static_cast<int>(type));
    
    return pet.id;
}

std::vector<StakingPet> CGamificationManager::GetAllPets() const
{
    std::vector<StakingPet> allPets;
    
    for (const auto& [id, pet] : pets) {
        allPets.push_back(pet);
    }
    
    return allPets;
}

StakingPet CGamificationManager::GetPet(const uint256& petId) const
{
    auto it = pets.find(petId);
    if (it != pets.end()) {
        return it->second;
    }
    return StakingPet(); // Return empty pet if not found
}

bool CGamificationManager::FeedPet(const uint256& petId, CAmount earnings)
{
    auto it = pets.find(petId);
    if (it == pets.end()) {
        return false;
    }
    
    StakingPet& pet = it->second;
    
    // Calculate experience gain from feeding
    int64_t experienceGain = static_cast<int64_t>(earnings / COIN); // 1 XP per coin
    pet.experience += experienceGain;
    pet.totalFed += earnings;
    pet.happiness = std::min(100LL, pet.happiness + 10); // Feeding increases happiness
    pet.lastFeedTime = GetTime();
    
    // Check if pet can evolve
    if (pet.CanEvolve()) {
        EvolvePet(petId);
    }
    
    AddNotification("pet", "Pet Fed!", 
                   strprintf("You fed %s and gained %d experience!", pet.name, experienceGain));
    
    LogPrintf("CGamificationManager::%s: Fed pet %s with %s, gained %d XP\n", 
             __func__, pet.name, FormatMoney(earnings), experienceGain);
    
    return true;
}

bool CGamificationManager::EvolvePet(const uint256& petId)
{
    auto it = pets.find(petId);
    if (it == pets.end()) {
        return false;
    }
    
    StakingPet& pet = it->second;
    
    if (!pet.CanEvolve()) {
        return false;
    }
    
    PetStage oldStage = pet.stage;
    pet.stage = pet.GetNextStage();
    
    // Update pet image for new stage
    std::string typeStr;
    switch (pet.type) {
        case PetType::CRYPTO_CAT: typeStr = "cat"; break;
        case PetType::STAKE_DRAGON: typeStr = "dragon"; break;
        case PetType::COIN_PUPPY: typeStr = "puppy"; break;
        case PetType::VALIDATOR_PHOENIX: typeStr = "phoenix"; break;
        case PetType::EARNINGS_EAGLE: typeStr = "eagle"; break;
    }
    
    std::string stageStr;
    switch (pet.stage) {
        case PetStage::EGG: stageStr = "egg"; break;
        case PetStage::BABY: stageStr = "baby"; break;
        case PetStage::JUVENILE: stageStr = "juvenile"; break;
        case PetStage::ADULT: stageStr = "adult"; break;
        case PetStage::LEGENDARY: stageStr = "legendary"; break;
    }
    
    pet.imageUrl = strprintf("https://babachain.org/pets/%s_%s.png", typeStr, stageStr);
    
    // Add special abilities based on evolution
    if (pet.stage == PetStage::ADULT) {
        pet.abilities.push_back("Bonus Earnings: +5%");
    } else if (pet.stage == PetStage::LEGENDARY) {
        pet.abilities.push_back("Bonus Earnings: +10%");
        pet.abilities.push_back("Lucky Staking: Chance for double rewards");
    }
    
    TriggerPetEvolution(pet);
    
    LogPrintf("CGamificationManager::%s: Pet %s evolved from %d to %d\n", 
             __func__, pet.name, static_cast<int>(oldStage), static_cast<int>(pet.stage));
    
    return true;
}

void CGamificationManager::UpdatePetHappiness()
{
    for (auto& [id, pet] : pets) {
        pet.UpdateHappiness();
    }
}

std::vector<NFTReward> CGamificationManager::GetAvailableNFTs() const
{
    std::vector<NFTReward> available;
    
    for (const auto& [tokenId, nft] : nftRewards) {
        if (!nft.isOwned) {
            available.push_back(nft);
        }
    }
    
    return available;
}

std::vector<NFTReward> CGamificationManager::GetOwnedNFTs() const
{
    std::vector<NFTReward> owned;
    
    for (const auto& [tokenId, nft] : nftRewards) {
        if (nft.isOwned) {
            owned.push_back(nft);
        }
    }
    
    return owned;
}

bool CGamificationManager::MintNFT(const std::string& tokenId, CAmount stakingAmount, int64_t stakingDuration)
{
    auto it = nftRewards.find(tokenId);
    if (it == nftRewards.end()) {
        return false;
    }
    
    NFTReward& nft = it->second;
    
    if (nft.isOwned) {
        return false; // Already owned
    }
    
    if (!CanMintNFT(tokenId, stakingAmount, stakingDuration)) {
        return false;
    }
    
    nft.isOwned = true;
    nft.mintTime = GetTime();
    profile.nftsOwned++;
    
    TriggerNFTMinted(nft);
    
    LogPrintf("CGamificationManager::%s: Minted NFT %s for staking %s for %d seconds\n", 
             __func__, tokenId, FormatMoney(stakingAmount), stakingDuration);
    
    return true;
}

bool CGamificationManager::CanMintNFT(const std::string& tokenId, CAmount stakingAmount, int64_t stakingDuration) const
{
    auto it = nftRewards.find(tokenId);
    if (it == nftRewards.end()) {
        return false;
    }
    
    const NFTReward& nft = it->second;
    
    return !nft.isOwned && 
           stakingAmount >= nft.stakingRequirement && 
           stakingDuration >= nft.stakingDuration;
}

void CGamificationManager::RecordEarnings(CAmount earnings)
{
    profile.totalEarnings += earnings;
    profile.lastStakeTime = GetTime();
    
    // Award experience based on earnings
    int64_t experienceGain = CalculateExperienceReward(earnings);
    AddExperience(experienceGain);
    
    // Check for NFT eligibility
    for (const auto& [tokenId, nft] : nftRewards) {
        if (!nft.isOwned && profile.totalStaked >= nft.stakingRequirement) {
            int64_t stakingDuration = GetTime() - profile.firstStakeTime;
            if (stakingDuration >= nft.stakingDuration) {
                MintNFT(tokenId, profile.totalStaked, stakingDuration);
            }
        }
    }
    
    // Feed all pets with earnings
    for (const auto& [petId, pet] : pets) {
        FeedPet(petId, earnings / pets.size()); // Distribute earnings among pets
    }
}

void CGamificationManager::UpdateStakingStreak()
{
    int64_t currentTime = GetTime();
    int64_t daysSinceLastStake = (currentTime - profile.lastStakeTime) / (24 * 60 * 60);
    
    if (daysSinceLastStake <= 1) {
        // Maintain or increase streak
        if (daysSinceLastStake == 1) {
            profile.consecutiveStakingDays++;
        }
    } else {
        // Streak broken
        profile.consecutiveStakingDays = 1; // Reset to 1 for today's stake
    }
    
    // Update max streak
    if (profile.consecutiveStakingDays > profile.maxStakingStreak) {
        profile.maxStakingStreak = profile.consecutiveStakingDays;
    }
}

void CGamificationManager::AddExperience(int64_t xp)
{
    profile.totalExperience += xp;
    
    int oldLevel = profile.level;
    profile.level = profile.GetLevelFromExperience();
    
    if (profile.level > oldLevel) {
        AddNotification("level", "Level Up!", 
                       strprintf("Congratulations! You reached level %d!", profile.level));
        
        LogPrintf("CGamificationManager::%s: Level up! New level: %d (XP: %d)\n", 
                 __func__, profile.level, profile.totalExperience);
    }
}

void CGamificationManager::RecordStakingActivity(CAmount amount)
{
    profile.totalStaked += amount;
    
    if (profile.firstStakeTime == 0) {
        profile.firstStakeTime = GetTime();
    }
    
    profile.lastStakeTime = GetTime();
    UpdateStakingStreak();
}

std::vector<CGamificationManager::LeaderboardEntry> CGamificationManager::GetStakingLeaderboard(int limit) const
{
    // In a real implementation, this would query a database of all users
    // For now, return a mock leaderboard with current user
    std::vector<LeaderboardEntry> leaderboard;
    
    LeaderboardEntry currentUser;
    currentUser.address = "Current User"; // In real implementation, get actual address
    currentUser.totalStaked = profile.totalStaked;
    currentUser.totalEarnings = profile.totalEarnings;
    currentUser.level = profile.level;
    currentUser.achievementsCount = profile.achievementsUnlocked;
    currentUser.stakingStreak = profile.consecutiveStakingDays;
    
    leaderboard.push_back(currentUser);
    
    return leaderboard;
}

std::vector<CGamificationManager::LeaderboardEntry> CGamificationManager::GetEarningsLeaderboard(int limit) const
{
    // Similar to staking leaderboard, but sorted by earnings
    return GetStakingLeaderboard(limit);
}

std::vector<CGamificationManager::Notification> CGamificationManager::GetNotifications() const
{
    return notifications;
}

void CGamificationManager::AddNotification(const std::string& type, const std::string& title, const std::string& message)
{
    Notification notification;
    notification.type = type;
    notification.title = title;
    notification.message = message;
    notification.timestamp = GetTime();
    notification.isRead = false;
    
    notifications.push_back(notification);
    
    // Keep only last 100 notifications
    if (notifications.size() > 100) {
        notifications.erase(notifications.begin());
    }
}

void CGamificationManager::MarkNotificationRead(int index)
{
    if (index >= 0 && index < static_cast<int>(notifications.size())) {
        notifications[index].isRead = true;
    }
}

Achievement CGamificationManager::GetAchievement(const uint256& id) const
{
    auto it = achievements.find(id);
    if (it != achievements.end()) {
        return it->second;
    }
    return Achievement(); // Return empty achievement if not found
}

void CGamificationManager::UpdateProfileLevel()
{
    int newLevel = profile.GetLevelFromExperience();
    if (newLevel > profile.level) {
        profile.level = newLevel;
        AddNotification("level", "Level Up!", 
                       strprintf("You reached level %d!", profile.level));
    }
}

int64_t CGamificationManager::CalculateExperienceReward(CAmount earnings) const
{
    // 1 XP per coin earned, with bonus for higher amounts
    int64_t baseXP = static_cast<int64_t>(earnings / COIN);
    
    // Bonus XP for larger earnings
    if (earnings >= 1000 * COIN) {
        baseXP *= 2; // Double XP for 1000+ coin earnings
    } else if (earnings >= 100 * COIN) {
        baseXP = (baseXP * 3) / 2; // 1.5x XP for 100+ coin earnings
    }
    
    return baseXP;
}

void CGamificationManager::TriggerAchievementUnlocked(const Achievement& achievement)
{
    // In a real implementation, this would trigger UI notifications,
    // sound effects, and potentially award the achievement reward
    LogPrintf("CGamificationManager::%s: 🏆 Achievement Unlocked: %s - %s\n", 
             __func__, achievement.name, achievement.description);
    
    AddNotification("achievement", "Achievement Unlocked!", 
                   strprintf("🏆 %s: %s", achievement.name, achievement.description));
    
    profile.achievementsUnlocked++;
    
    // Award the achievement reward if applicable
    if (achievement.rewardAmount > 0 && wallet) {
        // In a real implementation, this would add the reward to the wallet
        LogPrintf("CGamificationManager::%s: Awarded %s for achievement %s\n", 
                 __func__, FormatMoney(achievement.rewardAmount), achievement.name);
    }
}

void CGamificationManager::TriggerChallengeCompleted(const Challenge& challenge)
{
    LogPrintf("CGamificationManager::%s: ✅ Challenge Completed: %s\n", 
             __func__, challenge.name);
    
    AddNotification("challenge", "Challenge Completed!", 
                   strprintf("✅ %s: %s", challenge.name, challenge.description));
}

void CGamificationManager::TriggerPetEvolution(const StakingPet& pet)
{
    std::string stageStr;
    switch (pet.stage) {
        case PetStage::BABY: stageStr = "Baby"; break;
        case PetStage::JUVENILE: stageStr = "Juvenile"; break;
        case PetStage::ADULT: stageStr = "Adult"; break;
        case PetStage::LEGENDARY: stageStr = "Legendary"; break;
        default: stageStr = "Unknown"; break;
    }
    
    LogPrintf("CGamificationManager::%s: 🐾 Pet Evolution: %s evolved to %s stage!\n", 
             __func__, pet.name, stageStr);
    
    AddNotification("pet", "Pet Evolution!", 
                   strprintf("🐾 %s evolved to %s stage!", pet.name, stageStr));
}

void CGamificationManager::TriggerNFTMinted(const NFTReward& nft)
{
    LogPrintf("CGamificationManager::%s: 🎨 NFT Minted: %s\n", 
             __func__, nft.name);
    
    AddNotification("nft", "NFT Reward!", 
                   strprintf("🎨 You earned the NFT: %s", nft.name));
}

} // namespace wallet