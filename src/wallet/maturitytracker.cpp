// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/maturitytracker.h>
#include <wallet/wallet.h>
#include <validation.h>
#include <chainparams.h>
#include <util/time.h>
#include <logging.h>

namespace wallet {

CMaturityTracker::CMaturityTracker(CWallet* wallet) :
    wallet(wallet),
    maturityThreshold(100),         // Default 100 confirmations for maturity
    nearMaturityThreshold(10)       // Notify 10 blocks before maturity
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
    info.isStakeable = (currentDepth >= maturityThreshold);
    info.estimatedMaturityTime = EstimateBlockTime(std::max(0, maturityThreshold - currentDepth));
    
    trackedCoins[outpoint] = info;
    
    LogPrint(BCLog::STAKING, "CMaturityTracker::TrackCoin: Tracking coin %s:%d, amount=%s, depth=%d\n",
             txid.ToString(), vout, FormatMoney(amount), currentDepth);
}

void CMaturityTracker::UntrackCoin(const COutPoint& outpoint)
{
    auto it = trackedCoins.find(outpoint);
    if (it != trackedCoins.end()) {
        LogPrint(BCLog::STAKING, "CMaturityTracker::UntrackCoin: Stopped tracking coin %s:%d\n",
                 it->second.txid.ToString(), it->second.vout);
        trackedCoins.erase(it);
    }
}

void CMaturityTracker::UpdateMaturityStatus()
{
    if (!wallet) return;
    
    LOCK(wallet->cs_wallet);
    
    std::vector<COutPoint> toRemove;
    
    for (auto& [outpoint, coinInfo] : trackedCoins) {
        // Get current transaction
        auto it = wallet->mapWallet.find(coinInfo.txid);
        if (it == wallet->mapWallet.end()) {
            // Transaction no longer in wallet, stop tracking
            toRemove.push_back(outpoint);
            continue;
        }
        
        const CWalletTx& wtx = it->second;
        
        // Check if output still exists and is unspent
        if (coinInfo.vout >= (int)wtx.tx->vout.size()) {
            toRemove.push_back(outpoint);
            continue;
        }
        
        // Update depth
        int oldDepth = coinInfo.currentDepth;
        coinInfo.currentDepth = wtx.GetDepthInMainChain();
        
        // Check if coin was spent
        if (wallet->IsSpent(outpoint)) {
            toRemove.push_back(outpoint);
            continue;
        }
        
        // Update maturity status
        bool wasStakeable = coinInfo.isStakeable;
        coinInfo.isStakeable = (coinInfo.currentDepth >= maturityThreshold);
        coinInfo.estimatedMaturityTime = EstimateBlockTime(std::max(0, maturityThreshold - coinInfo.currentDepth));
        
        // Check for maturity events
        if (!wasStakeable && coinInfo.isStakeable) {
            // Coin just became mature
            TriggerMaturityCallbacks(coinInfo);
        } else if (!coinInfo.isStakeable && 
                   coinInfo.currentDepth >= (maturityThreshold - nearMaturityThreshold) &&
                   oldDepth < (maturityThreshold - nearMaturityThreshold)) {
            // Coin is approaching maturity
            TriggerNearMaturityCallbacks(coinInfo);
        }
    }
    
    // Remove coins that are no longer valid
    for (const auto& outpoint : toRemove) {
        trackedCoins.erase(outpoint);
    }
}

CoinMaturityInfo CMaturityTracker::GetCoinMaturityInfo(const COutPoint& outpoint) const
{
    auto it = trackedCoins.find(outpoint);
    if (it != trackedCoins.end()) {
        return it->second;
    }
    return CoinMaturityInfo(); // Return empty info if not found
}

std::vector<CoinMaturityInfo> CMaturityTracker::GetAllTrackedCoins() const
{
    std::vector<CoinMaturityInfo> result;
    result.reserve(trackedCoins.size());
    
    for (const auto& [outpoint, coinInfo] : trackedCoins) {
        result.push_back(coinInfo);
    }
    
    return result;
}

std::vector<CoinMaturityInfo> CMaturityTracker::GetMatureCoins() const
{
    std::vector<CoinMaturityInfo> result;
    
    for (const auto& [outpoint, coinInfo] : trackedCoins) {
        if (coinInfo.IsMature()) {
            result.push_back(coinInfo);
        }
    }
    
    return result;
}

std::vector<CoinMaturityInfo> CMaturityTracker::GetNearMatureCoins() const
{
    std::vector<CoinMaturityInfo> result;
    
    for (const auto& [outpoint, coinInfo] : trackedCoins) {
        if (!coinInfo.IsMature() && 
            coinInfo.currentDepth >= (maturityThreshold - nearMaturityThreshold)) {
            result.push_back(coinInfo);
        }
    }
    
    return result;
}

CAmount CMaturityTracker::GetMatureAmount() const
{
    CAmount total = 0;
    
    for (const auto& [outpoint, coinInfo] : trackedCoins) {
        if (coinInfo.IsMature()) {
            total += coinInfo.amount;
        }
    }
    
    return total;
}

CAmount CMaturityTracker::GetNearMatureAmount() const
{
    CAmount total = 0;
    
    for (const auto& [outpoint, coinInfo] : trackedCoins) {
        if (!coinInfo.IsMature() && 
            coinInfo.currentDepth >= (maturityThreshold - nearMaturityThreshold)) {
            total += coinInfo.amount;
        }
    }
    
    return total;
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
    maturityThreshold = std::max(1, blocks);
    
    // Update all tracked coins with new threshold
    for (auto& [outpoint, coinInfo] : trackedCoins) {
        coinInfo.requiredDepth = maturityThreshold;
        coinInfo.isStakeable = (coinInfo.currentDepth >= maturityThreshold);
        coinInfo.estimatedMaturityTime = EstimateBlockTime(std::max(0, maturityThreshold - coinInfo.currentDepth));
    }
}

void CMaturityTracker::SetNearMaturityThreshold(int blocks)
{
    nearMaturityThreshold = std::max(1, blocks);
}

int64_t CMaturityTracker::GetTimeUntilNextMaturity() const
{
    int64_t nextMaturityTime = std::numeric_limits<int64_t>::max();
    
    for (const auto& [outpoint, coinInfo] : trackedCoins) {
        if (!coinInfo.IsMature() && coinInfo.estimatedMaturityTime < nextMaturityTime) {
            nextMaturityTime = coinInfo.estimatedMaturityTime;
        }
    }
    
    return (nextMaturityTime == std::numeric_limits<int64_t>::max()) ? 0 : nextMaturityTime;
}

size_t CMaturityTracker::GetTrackedCoinCount() const
{
    return trackedCoins.size();
}

void CMaturityTracker::CheckForMaturityEvents()
{
    // This is called by UpdateMaturityStatus()
    // Events are triggered there to avoid duplicate processing
}

int64_t CMaturityTracker::EstimateBlockTime(int blocks) const
{
    if (blocks <= 0) return GetTime();
    
    // Estimate based on average block time (2.5 minutes for BabaChain)
    const int64_t averageBlockTime = 150; // 2.5 minutes in seconds
    return GetTime() + (blocks * averageBlockTime);
}

void CMaturityTracker::TriggerMaturityCallbacks(const CoinMaturityInfo& coinInfo)
{
    LogPrint(BCLog::STAKING, "CMaturityTracker: Coin %s:%d became mature (amount=%s)\n",
             coinInfo.txid.ToString(), coinInfo.vout, FormatMoney(coinInfo.amount));
    
    for (const auto& callback : maturityCallbacks) {
        try {
            callback(coinInfo);
        } catch (const std::exception& e) {
            LogPrintf("CMaturityTracker: Exception in maturity callback: %s\n", e.what());
        }
    }
}

void CMaturityTracker::TriggerNearMaturityCallbacks(const CoinMaturityInfo& coinInfo)
{
    LogPrint(BCLog::STAKING, "CMaturityTracker: Coin %s:%d approaching maturity (blocks left=%d)\n",
             coinInfo.txid.ToString(), coinInfo.vout, coinInfo.BlocksUntilMature());
    
    for (const auto& callback : nearMaturityCallbacks) {
        try {
            callback(coinInfo);
        } catch (const std::exception& e) {
            LogPrintf("CMaturityTracker: Exception in near-maturity callback: %s\n", e.what());
        }
    }
}

} // namespace wallet