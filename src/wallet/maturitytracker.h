// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_MATURITYTRACKER_H
#define BITCOIN_WALLET_MATURITYTRACKER_H

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <uint256.h>

#include <map>
#include <vector>
#include <functional>

namespace wallet {

class CWallet;

/**
 * Information about a coin's maturity status
 */
struct CoinMaturityInfo {
    uint256 txid;                   // Transaction ID
    int vout;                       // Output index
    CAmount amount;                 // Coin amount
    int currentDepth;               // Current confirmation depth
    int requiredDepth;              // Required depth for maturity
    int64_t estimatedMaturityTime;  // Estimated time when coin will mature
    bool isStakeable;               // Whether coin can be used for staking
    
    CoinMaturityInfo() : vout(0), amount(0), currentDepth(0), requiredDepth(100), 
                        estimatedMaturityTime(0), isStakeable(false) {}
    
    bool IsMature() const { return currentDepth >= requiredDepth; }
    int BlocksUntilMature() const { return std::max(0, requiredDepth - currentDepth); }
};

/**
 * Notification callback for maturity events
 */
using MaturityCallback = std::function<void(const CoinMaturityInfo&)>;

/**
 * Tracks coin maturity and provides notifications when coins become stakeable
 */
class CMaturityTracker
{
private:
    CWallet* wallet;
    std::map<COutPoint, CoinMaturityInfo> trackedCoins;
    std::vector<MaturityCallback> maturityCallbacks;
    std::vector<MaturityCallback> nearMaturityCallbacks;
    
    // Configuration
    int maturityThreshold;          // Blocks required for maturity (default 100)
    int nearMaturityThreshold;      // Blocks before maturity to send "near" notification (default 10)
    
public:
    explicit CMaturityTracker(CWallet* wallet);
    ~CMaturityTracker();
    
    /** Add a coin to track for maturity */
    void TrackCoin(const uint256& txid, int vout, CAmount amount, int currentDepth);
    
    /** Remove a coin from tracking (e.g., when spent) */
    void UntrackCoin(const COutPoint& outpoint);
    
    /** Update tracking information for all coins */
    void UpdateMaturityStatus();
    
    /** Get maturity information for a specific coin */
    CoinMaturityInfo GetCoinMaturityInfo(const COutPoint& outpoint) const;
    
    /** Get all tracked coins */
    std::vector<CoinMaturityInfo> GetAllTrackedCoins() const;
    
    /** Get coins that are mature and ready for staking */
    std::vector<CoinMaturityInfo> GetMatureCoins() const;
    
    /** Get coins that are close to maturity */
    std::vector<CoinMaturityInfo> GetNearMatureCoins() const;
    
    /** Get total amount of mature coins */
    CAmount GetMatureAmount() const;
    
    /** Get total amount of coins approaching maturity */
    CAmount GetNearMatureAmount() const;
    
    /** Register callback for when coins become mature */
    void RegisterMaturityCallback(const MaturityCallback& callback);
    
    /** Register callback for when coins are near maturity */
    void RegisterNearMaturityCallback(const MaturityCallback& callback);
    
    /** Set maturity threshold (blocks required) */
    void SetMaturityThreshold(int blocks);
    
    /** Set near-maturity notification threshold */
    void SetNearMaturityThreshold(int blocks);
    
    /** Get estimated time until next coin matures */
    int64_t GetTimeUntilNextMaturity() const;
    
    /** Get number of coins currently being tracked */
    size_t GetTrackedCoinCount() const;
    
private:
    /** Check for newly mature coins and trigger callbacks */
    void CheckForMaturityEvents();
    
    /** Estimate block time for maturity calculations */
    int64_t EstimateBlockTime(int blocks) const;
    
    /** Trigger maturity callbacks */
    void TriggerMaturityCallbacks(const CoinMaturityInfo& coinInfo);
    
    /** Trigger near-maturity callbacks */
    void TriggerNearMaturityCallbacks(const CoinMaturityInfo& coinInfo);
};

} // namespace wallet

#endif // BITCOIN_WALLET_MATURITYTRACKER_H