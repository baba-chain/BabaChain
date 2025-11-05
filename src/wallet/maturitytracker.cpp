// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/maturitytracker.h>

#include <wallet/wallet.h>
#include <logging.h>
#include <util/time.h>
#include <util/moneystr.h>

namespace wallet {

CMaturityTracker::CMaturityTracker(CWallet* wallet) 
    : wallet(wallet), maturityThreshold(100), nearMaturityThreshold(10)
{
}

CMaturityTracker::~CMaturityTracker()
{
}

void CMaturityTracker::TrackCoin(const uint256& txid, int vout, CAmount amount, int currentDepth)
{
    COutPoint outpoint(txid, vout);
    
    CoinMaturityInfo info;
    info.txid = txid;
    info.vout = vout;
    info.amount = amount;
    info.currentDepth = currentDepth;
    info.requiredDepth = maturityThreshold;
    info.estimatedMaturityTime = EstimateBlockTime(maturityThreshold - currentDepth);
    info.isStakeable = (currentDepth >= maturityThreshold);
    
    trackedCoins[outpoint] = info;
    
    LogPrint(BCLog::WALLET, "CMaturityTracker::%s: Tracking coin %s:%d, amount %s, depth %d/%d\n", 
             __func__, txid.ToString(), vout, FormatMoney(amount), currentDepth, maturityThreshold);
}

void CMaturityTracker::UntrackCoin(const COutPoint& outpoint)
{
    auto it = trackedCoins.find(outpoint);
    if (it != trackedCoins.end()) {
        LogPrint(BCLog::WALLET, "CMaturityTracker::%s: Untracking coin %s\n", 
                 __func__, outpoint.ToString());
        trackedCoins.erase(it);
    }
}

void CMaturityTracker::UpdateMaturityStatus()
{
    std::vector<CoinMaturityInfo> newlyMature;
    std::vector<CoinMaturityInfo> nearMature;
    
    for (auto& [outpoint, coinInfo] : trackedCoins) {
        // Update current depth (this would normally come from chain state)
        // For now, we'll simulate depth increase
        bool wasImmature = !coinInfo.IsMature();
        bool wasNotNearMature = coinInfo.BlocksUntilMature() > nearMaturityThreshold;
        
        // In a real implementation, we'd get the actual depth from the blockchain
        // coinInfo.currentDepth = GetActualDepthFromChain(coinInfo.txid);
        
        coinInfo.estimatedMaturityTime = EstimateBlockTime(coinInfo.BlocksUntilMature());
        coinInfo.isStakeable = coinInfo.IsMature();
        
        // Check for maturity events
        if (wasImmature && coinInfo.IsMature()) {
            newlyMature.push_back(coinInfo);
        }
        
        // Check for near-maturity events
        if (wasNotNearMature && coinInfo.BlocksUntilMature() <= nearMaturityThreshold && !coinInfo.IsMature()) {
            nearMature.push_back(coinInfo);
        }
    }
    
    // Trigger callbacks for newly mature coins
    for (const auto& coinInfo : newlyMature) {
        TriggerMaturityCallbacks(coinInfo);
    }
    
    // Trigger callbacks for near-mature coins
    for (const auto& coinInfo : nearMature) {
        TriggerNearMaturityCallbacks(coinInfo);
    }
    
    if (!newlyMature.empty() || !nearMature.empty()) {
        LogPrint(BCLog::WALLET, "CMaturityTracker::%s: %d coins became mature, %d coins near maturity\n", 
                 __func__, newlyMature.size(), nearMature.size());
    }
}

CoinMaturityInfo CMaturityTracker::GetCoinMaturityInfo(const COutPoint& outpoint) const
{
    auto it = trackedCoins.find(outpoint);
    if (it != trackedCoins.end()) {
        return it->second;
    }
    return CoinMaturityInfo();
}

std::vector<CoinMaturityInfo> CMaturityTracker::GetAllTrackedCoins() const
{
    std::vector<CoinMaturityInfo> coins;
    coins.reserve(trackedCoins.size());
    
    for (const auto& [outpoint, coinInfo] : trackedCoins) {
        coins.push_back(coinInfo);
    }
    
    return coins;
}

std::vector<CoinMaturityInfo> CMaturityTracker::GetMatureCoins() const
{
    std::vector<CoinMaturityInfo> matureCoins;
    
    for (const auto& [outpoint, coinInfo] : trackedCoins) {
        if (coinInfo.IsMature()) {
            matureCoins.push_back(coinInfo);
        }
    }
    
    return matureCoins;
}

std::vector<CoinMaturityInfo> CMaturityTracker::GetNearMatureCoins() const
{
    std::vector<CoinMaturityInfo> nearMatureCoins;
    
    for (const auto& [outpoint, coinInfo] : trackedCoins) {
        if (!coinInfo.IsMature() && coinInfo.BlocksUntilMature() <= nearMaturityThreshold) {
            nearMatureCoins.push_back(coinInfo);
        }
    }
    
    return nearMatureCoins;
}

CAmount CMaturityTracker::GetMatureAmount() const
{
    CAmount totalMature = 0;
    
    for (const auto& [outpoint, coinInfo] : trackedCoins) {
        if (coinInfo.IsMature()) {
            totalMature += coinInfo.amount;
        }
    }
    
    return totalMature;
}

CAmount CMaturityTracker::GetNearMatureAmount() const
{
    CAmount totalNearMature = 0;
    
    for (const auto& [outpoint, coinInfo] : trackedCoins) {
        if (!coinInfo.IsMature() && coinInfo.BlocksUntilMature() <= nearMaturityThreshold) {
            totalNearMature += coinInfo.amount;
        }
    }
    
    return totalNearMature;
}

void CMaturityTracker::RegisterMaturityCallback(const MaturityCallback& callback)
{
    maturityCallbacks.push_back(callback);
}

void CMaturityTracker::RegisterNearMaturityCallback(const MaturityCallback& callback)
{
    nearMaturityCallbacks.push_back(callback);
}

void CMaturityTracker::SetMaturityThreshold(int blocks)
{
    maturityThreshold = blocks;
    
    // Update all tracked coins with new threshold
    for (auto& [outpoint, coinInfo] : trackedCoins) {
        coinInfo.requiredDepth = maturityThreshold;
        coinInfo.estimatedMaturityTime = EstimateBlockTime(coinInfo.BlocksUntilMature());
        coinInfo.isStakeable = coinInfo.IsMature();
    }
    
    LogPrint(BCLog::WALLET, "CMaturityTracker::%s: Set maturity threshold to %d blocks\n", 
             __func__, blocks);
}

void CMaturityTracker::SetNearMaturityThreshold(int blocks)
{
    nearMaturityThreshold = blocks;
    
    LogPrint(BCLog::WALLET, "CMaturityTracker::%s: Set near-maturity threshold to %d blocks\n", 
             __func__, blocks);
}

int64_t CMaturityTracker::GetTimeUntilNextMaturity() const
{
    int64_t earliestMaturity = 0;
    
    for (const auto& [outpoint, coinInfo] : trackedCoins) {
        if (!coinInfo.IsMature()) {
            if (earliestMaturity == 0 || coinInfo.estimatedMaturityTime < earliestMaturity) {
                earliestMaturity = coinInfo.estimatedMaturityTime;
            }
        }
    }
    
    if (earliestMaturity > 0) {
        return std::max(int64_t(0), earliestMaturity - GetTime());
    }
    
    return 0;
}

size_t CMaturityTracker::GetTrackedCoinCount() const
{
    return trackedCoins.size();
}

int64_t CMaturityTracker::EstimateBlockTime(int blocks) const
{
    if (blocks <= 0) {
        return GetTime();
    }
    
    // Assume 2.5 minute block time (150 seconds)
    const int64_t BLOCK_TIME = 150;
    return GetTime() + (blocks * BLOCK_TIME);
}

void CMaturityTracker::TriggerMaturityCallbacks(const CoinMaturityInfo& coinInfo)
{
    for (const auto& callback : maturityCallbacks) {
        try {
            callback(coinInfo);
        } catch (const std::exception& e) {
            LogPrint(BCLog::WALLET, "CMaturityTracker::%s: Maturity callback exception: %s\n", 
                     __func__, e.what());
        }
    }
}

void CMaturityTracker::TriggerNearMaturityCallbacks(const CoinMaturityInfo& coinInfo)
{
    for (const auto& callback : nearMaturityCallbacks) {
        try {
            callback(coinInfo);
        } catch (const std::exception& e) {
            LogPrint(BCLog::WALLET, "CMaturityTracker::%s: Near-maturity callback exception: %s\n", 
                     __func__, e.what());
        }
    }
}

} // namespace wallet