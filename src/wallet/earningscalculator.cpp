// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/earningscalculator.h>
#include <wallet/wallet.h>
#include <util/time.h>
#include <util/moneystr.h>
#include <logging.h>

#include <algorithm>
#include <cmath>

namespace wallet {

CEarningsCalculator::CEarningsCalculator(CWallet* wallet) :
    wallet(wallet),
    baseAPY(3.65),                  // 365% APY
    networkDifficultyFactor(1.0),   // No adjustment initially
    lastUpdateTime(0)
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
    
    // Keep history manageable (last 1 year of data)
    int64_t oneYearAgo = GetTime() - (365 * 24 * 60 * 60);
    earningsHistory.erase(
        std::remove_if(earningsHistory.begin(), earningsHistory.end(),
            [oneYearAgo](const EarningsDataPoint& point) {
                return point.timestamp < oneYearAgo;
            }),
        earningsHistory.end()
    );
    
    // Update statistics
    CalculateStats();
    
    LogPrint(BCLog::STAKING, "CEarningsCalculator::RecordEarning: Recorded earning of %s, total staked: %s\n",
             FormatMoney(amount), FormatMoney(totalStaked));
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

std::vector<EarningsDataPoint> CEarningsCalculator::GetEarningsHistory(int64_t fromTime, int64_t toTime) const
{
    std::vector<EarningsDataPoint> result;
    
    for (const auto& point : earningsHistory) {
        if (point.timestamp >= fromTime && point.timestamp <= toTime) {
            result.push_back(point);
        }
    }
    
    return result;
}

EarningsProjection CEarningsCalculator::CalculateProjection(CAmount stakeAmount) const
{
    EarningsProjection projection;
    
    if (stakeAmount <= 0) {
        return projection; // Return empty projection
    }
    
    // Apply network difficulty adjustment
    double adjustedAPY = AdjustAPYForDifficulty(baseAPY);
    
    // Calculate projections
    double dailyRate = adjustedAPY / 365.0 / 100.0;
    double weeklyRate = dailyRate * 7.0;
    double monthlyRate = dailyRate * 30.0;
    double yearlyRate = adjustedAPY / 100.0;
    
    projection.daily = stakeAmount * dailyRate;
    projection.weekly = stakeAmount * weeklyRate;
    projection.monthly = stakeAmount * monthlyRate;
    projection.yearly = stakeAmount * yearlyRate;
    projection.currentAPY = CalculateEffectiveAPY();
    projection.projectedAPY = adjustedAPY;
    projection.totalStaked = stakeAmount;
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
    
    CAmount totalEarnings = 0;
    CAmount averageStaked = 0;
    int dataPoints = 0;
    
    for (const auto& point : earningsHistory) {
        if (point.timestamp >= thirtyDaysAgo) {
            totalEarnings += point.amount;
            averageStaked += point.totalStaked;
            dataPoints++;
        }
    }
    
    if (dataPoints == 0 || averageStaked == 0) {
        return baseAPY;
    }
    
    averageStaked /= dataPoints;
    
    // Annualize the earnings
    double dailyEarnings = static_cast<double>(totalEarnings) / 30.0;
    double yearlyEarnings = dailyEarnings * 365.0;
    double effectiveAPY = (yearlyEarnings / averageStaked) * 100.0;
    
    return std::max(0.0, effectiveAPY);
}

CAmount CEarningsCalculator::GetEarningsForPeriod(int64_t fromTime, int64_t toTime) const
{
    CAmount total = 0;
    
    for (const auto& point : earningsHistory) {
        if (point.timestamp >= fromTime && point.timestamp <= toTime) {
            total += point.amount;
        }
    }
    
    return total;
}

CAmount CEarningsCalculator::GetAverageDailyEarnings(int days) const
{
    int64_t fromTime = GetTime() - (days * 24 * 60 * 60);
    CAmount totalEarnings = GetEarningsForPeriod(fromTime, GetTime());
    
    return totalEarnings / days;
}

int64_t CEarningsCalculator::EstimateTimeToTarget(CAmount targetAmount, CAmount currentStake) const
{
    if (currentStake <= 0) return 0;
    
    EarningsProjection projection = CalculateProjection(currentStake);
    if (projection.daily <= 0) return 0;
    
    int64_t daysToTarget = targetAmount / projection.daily;
    return daysToTarget * 24 * 60 * 60; // Convert to seconds
}

EarningsProjection CEarningsCalculator::CalculateCompoundProjection(CAmount initialStake, int days, bool reinvest) const
{
    EarningsProjection projection;
    
    if (initialStake <= 0 || days <= 0) {
        return projection;
    }
    
    double adjustedAPY = AdjustAPYForDifficulty(baseAPY);
    double dailyRate = adjustedAPY / 365.0 / 100.0;
    
    CAmount currentStake = initialStake;
    CAmount totalEarnings = 0;
    
    for (int day = 0; day < days; day++) {
        CAmount dailyEarning = currentStake * dailyRate;
        totalEarnings += dailyEarning;
        
        if (reinvest) {
            currentStake += dailyEarning;
        }
    }
    
    projection.daily = totalEarnings / days;
    projection.weekly = projection.daily * 7;
    projection.monthly = projection.daily * 30;
    projection.yearly = totalEarnings * (365.0 / days);
    projection.currentAPY = CalculateEffectiveAPY();
    projection.projectedAPY = adjustedAPY;
    projection.totalStaked = currentStake;
    projection.lastUpdated = GetTime();
    
    return projection;
}

double CEarningsCalculator::GetROI() const
{
    if (currentStats.totalEarned <= 0 || currentProjection.totalStaked <= 0) {
        return 0.0;
    }
    
    return (static_cast<double>(currentStats.totalEarned) / currentProjection.totalStaked) * 100.0;
}

void CEarningsCalculator::SetBaseAPY(double apy)
{
    baseAPY = std::max(0.0, apy);
}

void CEarningsCalculator::UpdateNetworkDifficulty(double factor)
{
    networkDifficultyFactor = std::max(0.1, std::min(10.0, factor)); // Clamp between 0.1 and 10.0
}

void CEarningsCalculator::ClearHistory()
{
    earningsHistory.clear();
    currentStats = EarningsStats();
    currentProjection = EarningsProjection();
}

bool CEarningsCalculator::LoadFromDatabase()
{
    // This would load from the wallet database
    // For now, return true as a placeholder
    return true;
}

bool CEarningsCalculator::SaveToDatabase()
{
    // This would save to the wallet database
    // For now, return true as a placeholder
    return true;
}

void CEarningsCalculator::CalculateStats()
{
    currentStats = EarningsStats();
    
    if (earningsHistory.empty()) {
        return;
    }
    
    // Calculate totals
    for (const auto& point : earningsHistory) {
        currentStats.totalEarned += point.amount;
        currentStats.totalRewards++;
        
        if (currentStats.firstReward == 0 || point.timestamp < currentStats.firstReward) {
            currentStats.firstReward = point.timestamp;
        }
        
        if (point.timestamp > currentStats.lastReward) {
            currentStats.lastReward = point.timestamp;
        }
        
        // Track APY statistics
        if (currentStats.bestAPY == 0.0 || point.effectiveAPY > currentStats.bestAPY) {
            currentStats.bestAPY = point.effectiveAPY;
        }
        
        if (currentStats.worstAPY == 0.0 || point.effectiveAPY < currentStats.worstAPY) {
            currentStats.worstAPY = point.effectiveAPY;
        }
    }
    
    // Calculate period-specific earnings
    int64_t now = GetTime();
    currentStats.todayEarnings = GetEarningsForPeriod(GetStartOfDay(now), now);
    currentStats.weekEarnings = GetEarningsForPeriod(GetStartOfWeek(now), now);
    currentStats.monthEarnings = GetEarningsForPeriod(GetStartOfMonth(now), now);
    currentStats.yearEarnings = GetEarningsForPeriod(GetStartOfYear(now), now);
    
    // Calculate average APY
    double totalAPY = 0.0;
    int apyCount = 0;
    for (const auto& point : earningsHistory) {
        if (point.effectiveAPY > 0.0) {
            totalAPY += point.effectiveAPY;
            apyCount++;
        }
    }
    
    if (apyCount > 0) {
        currentStats.averageAPY = totalAPY / apyCount;
    }
    
    // Calculate consecutive days
    currentStats.consecutiveDays = CalculateConsecutiveDays();
}

void CEarningsCalculator::UpdateProjectionsFromHistory()
{
    // This would update projections based on historical performance
    // Currently handled by CalculateProjection()
}

CAmount CEarningsCalculator::GetTodayEarnings() const
{
    int64_t now = GetTime();
    return GetEarningsForPeriod(GetStartOfDay(now), now);
}

CAmount CEarningsCalculator::GetWeekEarnings() const
{
    int64_t now = GetTime();
    return GetEarningsForPeriod(GetStartOfWeek(now), now);
}

CAmount CEarningsCalculator::GetMonthEarnings() const
{
    int64_t now = GetTime();
    return GetEarningsForPeriod(GetStartOfMonth(now), now);
}

CAmount CEarningsCalculator::GetYearEarnings() const
{
    int64_t now = GetTime();
    return GetEarningsForPeriod(GetStartOfYear(now), now);
}

int CEarningsCalculator::CalculateConsecutiveDays() const
{
    if (earningsHistory.empty()) {
        return 0;
    }
    
    // Sort by timestamp (should already be sorted, but ensure it)
    std::vector<EarningsDataPoint> sortedHistory = earningsHistory;
    std::sort(sortedHistory.begin(), sortedHistory.end(),
        [](const EarningsDataPoint& a, const EarningsDataPoint& b) {
            return a.timestamp < b.timestamp;
        });
    
    int consecutiveDays = 0;
    int64_t lastDay = 0;
    
    for (const auto& point : sortedHistory) {
        int64_t currentDay = GetStartOfDay(point.timestamp);
        
        if (lastDay == 0) {
            consecutiveDays = 1;
        } else if (currentDay == lastDay + (24 * 60 * 60)) {
            consecutiveDays++;
        } else if (currentDay != lastDay) {
            consecutiveDays = 1; // Reset count
        }
        
        lastDay = currentDay;
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
    // Assuming week starts on Monday
    int64_t dayOfWeek = (timestamp / (24 * 60 * 60)) % 7;
    return timestamp - (dayOfWeek * 24 * 60 * 60);
}

int64_t CEarningsCalculator::GetStartOfMonth(int64_t timestamp) const
{
    // Simplified - assumes 30-day months
    int64_t dayOfMonth = (timestamp / (24 * 60 * 60)) % 30;
    return timestamp - (dayOfMonth * 24 * 60 * 60);
}

int64_t CEarningsCalculator::GetStartOfYear(int64_t timestamp) const
{
    // Simplified - assumes 365-day years
    int64_t dayOfYear = (timestamp / (24 * 60 * 60)) % 365;
    return timestamp - (dayOfYear * 24 * 60 * 60);
}

} // namespace wallet