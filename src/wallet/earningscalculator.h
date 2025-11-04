// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_EARNINGSCALCULATOR_H
#define BITCOIN_WALLET_EARNINGSCALCULATOR_H

#include <consensus/amount.h>
#include <uint256.h>

#include <map>
#include <vector>

namespace wallet {

class CWallet;

/**
 * Historical earnings data point
 */
struct EarningsDataPoint {
    int64_t timestamp;              // When the earning occurred
    CAmount amount;                 // Amount earned
    uint256 txid;                   // Transaction ID of the reward
    CAmount totalStaked;            // Total amount staked at the time
    double effectiveAPY;            // Effective APY at the time of earning
    
    EarningsDataPoint() : timestamp(0), amount(0), totalStaked(0), effectiveAPY(0.0) {}
};

/**
 * Earnings projection for different time periods
 */
struct EarningsProjection {
    CAmount daily;                  // Projected daily earnings
    CAmount weekly;                 // Projected weekly earnings
    CAmount monthly;                // Projected monthly earnings
    CAmount yearly;                 // Projected yearly earnings
    double currentAPY;              // Current effective APY
    double projectedAPY;            // Projected APY based on network conditions
    CAmount totalStaked;            // Total amount currently staked
    int64_t lastUpdated;            // When projection was last calculated
    
    EarningsProjection() : daily(0), weekly(0), monthly(0), yearly(0), 
                          currentAPY(0.0), projectedAPY(0.0), totalStaked(0), lastUpdated(0) {}
};

/**
 * Earnings statistics and analytics
 */
struct EarningsStats {
    CAmount totalEarned;            // Total earnings to date
    CAmount todayEarnings;          // Earnings today
    CAmount weekEarnings;           // Earnings this week
    CAmount monthEarnings;          // Earnings this month
    CAmount yearEarnings;           // Earnings this year
    double averageAPY;              // Average APY over time
    double bestAPY;                 // Best APY achieved
    double worstAPY;                // Worst APY achieved
    int totalRewards;               // Number of reward transactions
    int64_t firstReward;            // Timestamp of first reward
    int64_t lastReward;             // Timestamp of last reward
    int consecutiveDays;            // Consecutive days with rewards
    
    EarningsStats() : totalEarned(0), todayEarnings(0), weekEarnings(0), monthEarnings(0), 
                     yearEarnings(0), averageAPY(0.0), bestAPY(0.0), worstAPY(0.0), 
                     totalRewards(0), firstReward(0), lastReward(0), consecutiveDays(0) {}
};

/**
 * Calculator for staking earnings projections and analytics
 */
class CEarningsCalculator
{
private:
    CWallet* wallet;
    std::vector<EarningsDataPoint> earningsHistory;
    EarningsProjection currentProjection;
    EarningsStats currentStats;
    
    // Configuration
    double baseAPY;                 // Base APY rate (365%)
    double networkDifficultyFactor; // Network difficulty adjustment factor
    int64_t lastUpdateTime;         // Last time calculations were updated
    
public:
    explicit CEarningsCalculator(CWallet* wallet);
    ~CEarningsCalculator();
    
    /** Record a new staking reward */
    void RecordEarning(CAmount amount, const uint256& txid, CAmount totalStaked);
    
    /** Update projections based on current staking amount and network conditions */
    void UpdateProjections(CAmount currentStakeAmount);
    
    /** Get current earnings projection */
    EarningsProjection GetProjection() const;
    
    /** Get earnings statistics */
    EarningsStats GetStats() const;
    
    /** Get earnings history for a specific time period */
    std::vector<EarningsDataPoint> GetEarningsHistory(int64_t fromTime, int64_t toTime) const;
    
    /** Calculate projected earnings for a specific stake amount */
    EarningsProjection CalculateProjection(CAmount stakeAmount) const;
    
    /** Calculate effective APY based on recent performance */
    double CalculateEffectiveAPY() const;
    
    /** Get earnings for a specific time period */
    CAmount GetEarningsForPeriod(int64_t fromTime, int64_t toTime) const;
    
    /** Get average daily earnings over the last N days */
    CAmount GetAverageDailyEarnings(int days = 30) const;
    
    /** Estimate time to reach a target earning amount */
    int64_t EstimateTimeToTarget(CAmount targetAmount, CAmount currentStake) const;
    
    /** Calculate compound growth projection */
    EarningsProjection CalculateCompoundProjection(CAmount initialStake, int days, bool reinvest = true) const;
    
    /** Get ROI (Return on Investment) percentage */
    double GetROI() const;
    
    /** Set base APY rate */
    void SetBaseAPY(double apy);
    
    /** Update network difficulty factor */
    void UpdateNetworkDifficulty(double factor);
    
    /** Clear all earnings history */
    void ClearHistory();
    
    /** Load earnings history from wallet database */
    bool LoadFromDatabase();
    
    /** Save earnings history to wallet database */
    bool SaveToDatabase();
    
private:
    /** Calculate statistics from earnings history */
    void CalculateStats();
    
    /** Update projections based on historical data */
    void UpdateProjectionsFromHistory();
    
    /** Get earnings for today */
    CAmount GetTodayEarnings() const;
    
    /** Get earnings for this week */
    CAmount GetWeekEarnings() const;
    
    /** Get earnings for this month */
    CAmount GetMonthEarnings() const;
    
    /** Get earnings for this year */
    CAmount GetYearEarnings() const;
    
    /** Calculate consecutive days with rewards */
    int CalculateConsecutiveDays() const;
    
    /** Apply network difficulty adjustment to APY */
    double AdjustAPYForDifficulty(double baseAPY) const;
    
    /** Get timestamp for start of day */
    int64_t GetStartOfDay(int64_t timestamp) const;
    
    /** Get timestamp for start of week */
    int64_t GetStartOfWeek(int64_t timestamp) const;
    
    /** Get timestamp for start of month */
    int64_t GetStartOfMonth(int64_t timestamp) const;
    
    /** Get timestamp for start of year */
    int64_t GetStartOfYear(int64_t timestamp) const;
};

} // namespace wallet

#endif // BITCOIN_WALLET_EARNINGSCALCULATOR_H