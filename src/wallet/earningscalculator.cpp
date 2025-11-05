// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/earningscalculator.h>

#include <wallet/wallet.h>
#include <wallet/walletdb.h>
#include <logging.h>
#include <util/time.h>
#include <util/moneystr.h>

namespace wallet {

CEarningsCalculator::CEarningsCalculator(CWallet* wallet) 
    : wallet(wallet), baseAPY(365.0), networkDifficultyFactor(1.0), lastUpdateTime(0)
{
    LoadFromDatabase();
}

CEarningsCalculator::~CEarningsCalculator()
{
    SaveToDatabase();
}

void CEarningsCalculator::RecordEarning(CAmount amount, const uint256& txid, CAmount totalStaked)
{
    EarningsDataPoint dataPoint;
    dataPoint.timestamp = GetTime();
    dataPoint.amount = amount;
    dataPoint.txid = txid;
    dataPoint.totalStaked = totalStaked;
    dataPoint.effectiveAPY = CalculateEffectiveAPY();
    
    earningsHistory.push_back(dataPoint);
    
    // Keep only last 1000 entries to prevent unlimited growth
    if (earningsHistory.size() > 1000) {
        earningsHistory.erase(earningsHistory.begin());
    }
    
    CalculateStats();
    UpdateProjectionsFromHistory();
    
    LogPrint(BCLog::WALLET, "CEarningsCalculator::%s: Recorded earning %s, total staked %s\n", 
             __func__, FormatMoney(amount), FormatMoney(totalStaked));
}

void CEarningsCalculator::UpdateProjections(CAmount currentStakeAmount)
{
    currentProjection = CalculateProjection(currentStakeAmount);
    lastUpdateTime = GetTime();
}

EarningsProjection CEarningsCalculator::GetProjection() const
{
    return currentProjection;
}

EarningsStats CEarningsCalculator::GetStats() const
{
    return currentStats;
}

EarningsProjection CEarningsCalculator::CalculateProjection(CAmount stakeAmount) const
{
    EarningsProjection projection;
    
    if (stakeAmount <= 0) {
        return projection;
    }
    
    double effectiveAPY = AdjustAPYForDifficulty(baseAPY);
    double dailyRate = effectiveAPY / 365.0 / 100.0;
    
    projection.totalStaked = stakeAmount;
    projection.currentAPY = effectiveAPY;
    projection.projectedAPY = effectiveAPY;
    projection.daily = static_cast<CAmount>(stakeAmount * dailyRate);
    projection.weekly = projection.daily * 7;
    projection.monthly = projection.daily * 30;
    projection.yearly = static_cast<CAmount>(stakeAmount * effectiveAPY / 100.0);
    projection.lastUpdated = GetTime();
    
    return projection;
}

double CEarningsCalculator::CalculateEffectiveAPY() const
{
    if (earningsHistory.empty()) {
        return baseAPY;
    }
    
    // Calculate APY based on recent earnings (last 30 days)
    int64_t thirtyDaysAgo = GetTime() - (30 * 24 * 60 * 60);
    CAmount recentEarnings = 0;
    CAmount averageStaked = 0;
    int count = 0;
    
    for (const auto& dataPoint : earningsHistory) {
        if (dataPoint.timestamp >= thirtyDaysAgo) {
            recentEarnings += dataPoint.amount;
            averageStaked += dataPoint.totalStaked;
            count++;
        }
    }
    
    if (count == 0 || averageStaked == 0) {
        return baseAPY;
    }
    
    averageStaked /= count;
    
    // Annualize the 30-day earnings
    double thirtyDayReturn = static_cast<double>(recentEarnings) / static_cast<double>(averageStaked);
    double annualizedReturn = thirtyDayReturn * (365.0 / 30.0) * 100.0;
    
    return std::max(0.0, std::min(1000.0, annualizedReturn)); // Cap between 0% and 1000%
}

CAmount CEarningsCalculator::GetEarningsForPeriod(int64_t fromTime, int64_t toTime) const
{
    CAmount totalEarnings = 0;
    
    for (const auto& dataPoint : earningsHistory) {
        if (dataPoint.timestamp >= fromTime && dataPoint.timestamp <= toTime) {
            totalEarnings += dataPoint.amount;
        }
    }
    
    return totalEarnings;
}

void CEarningsCalculator::CalculateStats()
{
    currentStats = EarningsStats();
    
    if (earningsHistory.empty()) {
        return;
    }
    
    int64_t now = GetTime();
    
    // Calculate total earnings
    for (const auto& dataPoint : earningsHistory) {
        currentStats.totalEarned += dataPoint.amount;
        currentStats.totalRewards++;
        
        if (currentStats.firstReward == 0 || dataPoint.timestamp < currentStats.firstReward) {
            currentStats.firstReward = dataPoint.timestamp;
        }
        
        if (dataPoint.timestamp > currentStats.lastReward) {
            currentStats.lastReward = dataPoint.timestamp;
        }
    }
    
    // Calculate period-specific earnings
    currentStats.todayEarnings = GetTodayEarnings();
    currentStats.weekEarnings = GetWeekEarnings();
    currentStats.monthEarnings = GetMonthEarnings();
    currentStats.yearEarnings = GetYearEarnings();
    
    // Calculate APY statistics
    if (!earningsHistory.empty()) {
        double totalAPY = 0;
        double minAPY = 1000.0;
        double maxAPY = 0.0;
        int count = 0;
        
        for (const auto& dataPoint : earningsHistory) {
            if (dataPoint.effectiveAPY > 0) {
                totalAPY += dataPoint.effectiveAPY;
                minAPY = std::min(minAPY, dataPoint.effectiveAPY);
                maxAPY = std::max(maxAPY, dataPoint.effectiveAPY);
                count++;
            }
        }
        
        if (count > 0) {
            currentStats.averageAPY = totalAPY / count;
            currentStats.bestAPY = maxAPY;
            currentStats.worstAPY = minAPY;
        }
    }
    
    currentStats.consecutiveDays = CalculateConsecutiveDays();
}

CAmount CEarningsCalculator::GetTodayEarnings() const
{
    int64_t startOfDay = GetStartOfDay(GetTime());
    return GetEarningsForPeriod(startOfDay, GetTime());
}

CAmount CEarningsCalculator::GetWeekEarnings() const
{
    int64_t startOfWeek = GetStartOfWeek(GetTime());
    return GetEarningsForPeriod(startOfWeek, GetTime());
}

CAmount CEarningsCalculator::GetMonthEarnings() const
{
    int64_t startOfMonth = GetStartOfMonth(GetTime());
    return GetEarningsForPeriod(startOfMonth, GetTime());
}

CAmount CEarningsCalculator::GetYearEarnings() const
{
    int64_t startOfYear = GetStartOfYear(GetTime());
    return GetEarningsForPeriod(startOfYear, GetTime());
}

int CEarningsCalculator::CalculateConsecutiveDays() const
{
    if (earningsHistory.empty()) {
        return 0;
    }
    
    int64_t now = GetTime();
    int consecutiveDays = 0;
    
    // Check each day going backwards from today
    for (int day = 0; day < 365; day++) { // Max 1 year
        int64_t dayStart = GetStartOfDay(now - (day * 24 * 60 * 60));
        int64_t dayEnd = dayStart + 24 * 60 * 60;
        
        bool hasEarningsThisDay = false;
        for (const auto& dataPoint : earningsHistory) {
            if (dataPoint.timestamp >= dayStart && dataPoint.timestamp < dayEnd) {
                hasEarningsThisDay = true;
                break;
            }
        }
        
        if (hasEarningsThisDay) {
            consecutiveDays++;
        } else {
            break; // Streak broken
        }
    }
    
    return consecutiveDays;
}

double CEarningsCalculator::AdjustAPYForDifficulty(double baseAPY) const
{
    return baseAPY * networkDifficultyFactor;
}

int64_t CEarningsCalculator::GetStartOfDay(int64_t timestamp) const
{
    return (timestamp / (24 * 60 * 60)) * (24 * 60 * 60);
}

int64_t CEarningsCalculator::GetStartOfWeek(int64_t timestamp) const
{
    int64_t daysSinceEpoch = timestamp / (24 * 60 * 60);
    int64_t weeksSinceEpoch = daysSinceEpoch / 7;
    return weeksSinceEpoch * 7 * 24 * 60 * 60;
}

int64_t CEarningsCalculator::GetStartOfMonth(int64_t timestamp) const
{
    // Simplified: assume 30-day months
    int64_t daysSinceEpoch = timestamp / (24 * 60 * 60);
    int64_t monthsSinceEpoch = daysSinceEpoch / 30;
    return monthsSinceEpoch * 30 * 24 * 60 * 60;
}

int64_t CEarningsCalculator::GetStartOfYear(int64_t timestamp) const
{
    // Simplified: assume 365-day years
    int64_t daysSinceEpoch = timestamp / (24 * 60 * 60);
    int64_t yearsSinceEpoch = daysSinceEpoch / 365;
    return yearsSinceEpoch * 365 * 24 * 60 * 60;
}

void CEarningsCalculator::UpdateProjectionsFromHistory()
{
    // Update projections based on historical performance
    if (!earningsHistory.empty()) {
        CAmount averageStaked = 0;
        int count = 0;
        
        for (const auto& dataPoint : earningsHistory) {
            averageStaked += dataPoint.totalStaked;
            count++;
        }
        
        if (count > 0) {
            averageStaked /= count;
            UpdateProjections(averageStaked);
        }
    }
}

bool CEarningsCalculator::LoadFromDatabase()
{
    if (!wallet) {
        LogPrintf("CEarningsCalculator::%s: Wallet not available\n", __func__);
        return false;
    }
    
    LOCK(wallet->cs_wallet);
    
    // Load earnings history from wallet database
    CWalletDB walletdb(wallet->GetDatabase());
    
    // Load earnings data points
    std::vector<EarningsDataPoint> loadedHistory;
    if (walletdb.ReadEarningsHistory(loadedHistory)) {
        earningsHistory = std::move(loadedHistory);
        LogPrint(BCLog::WALLET, "CEarningsCalculator::%s: Loaded %d earnings data points\n", 
                 __func__, earningsHistory.size());
    }
    
    // Load configuration
    double loadedBaseAPY;
    if (walletdb.ReadEarningsBaseAPY(loadedBaseAPY)) {
        baseAPY = loadedBaseAPY;
    }
    
    double loadedDifficultyFactor;
    if (walletdb.ReadEarningsNetworkDifficulty(loadedDifficultyFactor)) {
        networkDifficultyFactor = loadedDifficultyFactor;
    }
    
    // Recalculate stats and projections
    CalculateStats();
    UpdateProjectionsFromHistory();
    
    LogPrint(BCLog::WALLET, "CEarningsCalculator::%s: Successfully loaded earnings data\n", __func__);
    return true;
}

bool CEarningsCalculator::SaveToDatabase()
{
    if (!wallet) {
        LogPrintf("CEarningsCalculator::%s: Wallet not available\n", __func__);
        return false;
    }
    
    LOCK(wallet->cs_wallet);
    
    // Save earnings history to wallet database
    CWalletDB walletdb(wallet->GetDatabase());
    
    // Save earnings data points
    if (!walletdb.WriteEarningsHistory(earningsHistory)) {
        LogPrintf("CEarningsCalculator::%s: Failed to save earnings history\n", __func__);
        return false;
    }
    
    // Save configuration
    if (!walletdb.WriteEarningsBaseAPY(baseAPY)) {
        LogPrintf("CEarningsCalculator::%s: Failed to save base APY\n", __func__);
        return false;
    }
    
    if (!walletdb.WriteEarningsNetworkDifficulty(networkDifficultyFactor)) {
        LogPrintf("CEarningsCalculator::%s: Failed to save network difficulty factor\n", __func__);
        return false;
    }
    
    LogPrint(BCLog::WALLET, "CEarningsCalculator::%s: Successfully saved earnings data (%d points)\n", 
             __func__, earningsHistory.size());
    return true;
}

void CEarningsCalculator::SetBaseAPY(double apy)
{
    baseAPY = apy;
    UpdateProjectionsFromHistory();
}

void CEarningsCalculator::UpdateNetworkDifficulty(double factor)
{
    networkDifficultyFactor = factor;
    UpdateProjectionsFromHistory();
}

std::vector<EarningsDataPoint> CEarningsCalculator::GetEarningsHistory(int64_t fromTime, int64_t toTime) const
{
    std::vector<EarningsDataPoint> filteredHistory;
    
    for (const auto& dataPoint : earningsHistory) {
        if (dataPoint.timestamp >= fromTime && dataPoint.timestamp <= toTime) {
            filteredHistory.push_back(dataPoint);
        }
    }
    
    return filteredHistory;
}

CAmount CEarningsCalculator::GetAverageDailyEarnings(int days) const
{
    if (days <= 0 || earningsHistory.empty()) {
        return 0;
    }
    
    int64_t fromTime = GetTime() - (days * 24 * 60 * 60);
    CAmount totalEarnings = GetEarningsForPeriod(fromTime, GetTime());
    
    return totalEarnings / days;
}

int64_t CEarningsCalculator::EstimateTimeToTarget(CAmount targetAmount, CAmount currentStake) const
{
    if (targetAmount <= 0 || currentStake <= 0) {
        return 0;
    }
    
    EarningsProjection projection = CalculateProjection(currentStake);
    if (projection.daily <= 0) {
        return 0; // Cannot reach target with zero daily earnings
    }
    
    int64_t daysToTarget = targetAmount / projection.daily;
    return daysToTarget * 24 * 60 * 60; // Convert to seconds
}

EarningsProjection CEarningsCalculator::CalculateCompoundProjection(CAmount initialStake, int days, bool reinvest) const
{
    EarningsProjection projection;
    
    if (initialStake <= 0 || days <= 0) {
        return projection;
    }
    
    CAmount currentStake = initialStake;
    CAmount totalEarnings = 0;
    double effectiveAPY = AdjustAPYForDifficulty(baseAPY);
    double dailyRate = effectiveAPY / 365.0 / 100.0;
    
    for (int day = 0; day < days; day++) {
        CAmount dailyEarning = static_cast<CAmount>(currentStake * dailyRate);
        totalEarnings += dailyEarning;
        
        if (reinvest) {
            currentStake += dailyEarning;
        }
    }
    
    projection.totalStaked = currentStake;
    projection.daily = static_cast<CAmount>(currentStake * dailyRate);
    projection.weekly = projection.daily * 7;
    projection.monthly = projection.daily * 30;
    projection.yearly = totalEarnings * (365 / days); // Annualized
    projection.currentAPY = effectiveAPY;
    projection.projectedAPY = effectiveAPY;
    projection.lastUpdated = GetTime();
    
    return projection;
}

double CEarningsCalculator::GetROI() const
{
    if (currentStats.totalEarned <= 0 || currentProjection.totalStaked <= 0) {
        return 0.0;
    }
    
    return (static_cast<double>(currentStats.totalEarned) / static_cast<double>(currentProjection.totalStaked)) * 100.0;
}

void CEarningsCalculator::ClearHistory()
{
    earningsHistory.clear();
    currentStats = EarningsStats();
    currentProjection = EarningsProjection();
    SaveToDatabase(); // Persist the cleared state
}

} // namespace wallet