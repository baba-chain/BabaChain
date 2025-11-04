// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_NETWORKSCALER_H
#define BITCOIN_NET_NETWORKSCALER_H

#include <net.h>
#include <netaddress.h>
#include <sync.h>
#include <uint256.h>

#include <atomic>
#include <map>
#include <memory>
#include <vector>
#include <functional>

class CConnman;
class CNode;

/**
 * Information about a network node for load balancing
 */
struct NodeLoadInfo {
    CNetAddr address;               // Node network address
    int64_t lastSeen;              // Last time node was seen
    int connectionCount;           // Current number of connections
    double cpuLoad;                // CPU load percentage (0-100)
    double memoryUsage;            // Memory usage percentage (0-100)
    int64_t bandwidth;             // Available bandwidth (bytes/sec)
    bool isFullNode;               // Whether this is a full node
    bool isStakingNode;            // Whether this node is staking
    double stakingWeight;          // Staking weight/power
    int64_t uptime;                // Node uptime in seconds
    double reliability;            // Reliability score (0-1)
    
    NodeLoadInfo() : lastSeen(0), connectionCount(0), cpuLoad(0.0), memoryUsage(0.0),
                    bandwidth(0), isFullNode(false), isStakingNode(false), 
                    stakingWeight(0.0), uptime(0), reliability(1.0) {}
};

/**
 * Geographic distribution information
 */
struct GeographicInfo {
    std::string country;           // Country code
    std::string region;            // Region/state
    std::string city;              // City name
    double latitude;               // Latitude coordinate
    double longitude;              // Longitude coordinate
    int nodeCount;                 // Number of nodes in this location
    
    GeographicInfo() : latitude(0.0), longitude(0.0), nodeCount(0) {}
};

/**
 * Network scaling metrics
 */
struct NetworkMetrics {
    int totalNodes;                // Total number of nodes
    int activeNodes;               // Currently active nodes
    int fullNodes;                 // Number of full nodes
    int stakingNodes;              // Number of staking nodes
    double averageLoad;            // Average network load
    double totalBandwidth;         // Total network bandwidth
    int64_t totalTransactions;     // Total transactions processed
    double networkHealth;          // Overall network health score (0-1)
    
    NetworkMetrics() : totalNodes(0), activeNodes(0), fullNodes(0), stakingNodes(0),
                      averageLoad(0.0), totalBandwidth(0.0), totalTransactions(0), networkHealth(1.0) {}
};

/**
 * Incentive configuration for running full nodes
 */
struct NodeIncentiveConfig {
    bool enableIncentives;         // Whether to enable node incentives
    double baseReward;             // Base reward for running a node
    double uptimeMultiplier;       // Multiplier based on uptime
    double bandwidthMultiplier;    // Multiplier based on bandwidth contribution
    double reliabilityMultiplier;  // Multiplier based on reliability
    int64_t minUptimeForReward;    // Minimum uptime to qualify for rewards
    double minReliabilityForReward; // Minimum reliability to qualify for rewards
    
    NodeIncentiveConfig() : enableIncentives(true), baseReward(1.0), uptimeMultiplier(1.5),
                           bandwidthMultiplier(1.2), reliabilityMultiplier(2.0),
                           minUptimeForReward(86400), minReliabilityForReward(0.8) {}
};

/**
 * Network auto-scaling and load balancing system
 */
class CNetworkScaler
{
private:
    mutable RecursiveMutex cs_scaler;
    
    // Node tracking
    std::map<CNetAddr, NodeLoadInfo> nodeLoadMap GUARDED_BY(cs_scaler);
    std::map<CNetAddr, GeographicInfo> nodeGeoMap GUARDED_BY(cs_scaler);
    
    // Metrics and configuration
    NetworkMetrics currentMetrics GUARDED_BY(cs_scaler);
    NodeIncentiveConfig incentiveConfig GUARDED_BY(cs_scaler);
    
    // Auto-scaling parameters
    std::atomic<bool> autoScalingEnabled{true};
    std::atomic<int> targetNodeCount{100};
    std::atomic<double> maxLoadThreshold{80.0};
    std::atomic<double> minLoadThreshold{20.0};
    std::atomic<int> maxConnectionsPerNode{125};
    
    // Geographic distribution
    std::atomic<bool> geoDistributionEnabled{true};
    std::atomic<int> maxNodesPerRegion{20};
    
    // Callbacks
    std::vector<std::function<void(const NetworkMetrics&)>> metricsCallbacks;
    std::vector<std::function<void(const CNetAddr&, double)>> incentiveCallbacks;
    
    CConnman* connman;
    
public:
    explicit CNetworkScaler(CConnman* connman);
    ~CNetworkScaler();
    
    /** Update node load information */
    void UpdateNodeLoad(const CNetAddr& addr, const NodeLoadInfo& loadInfo);
    
    /** Update geographic information for a node */
    void UpdateNodeGeography(const CNetAddr& addr, const GeographicInfo& geoInfo);
    
    /** Perform automatic load balancing */
    void PerformLoadBalancing();
    
    /** Scale network capacity based on demand */
    void ScaleNetworkCapacity();
    
    /** Optimize geographic distribution of nodes */
    void OptimizeGeographicDistribution();
    
    /** Calculate and distribute node incentives */
    void CalculateNodeIncentives();
    
    /** Get current network metrics */
    NetworkMetrics GetNetworkMetrics() const;
    
    /** Get load information for a specific node */
    NodeLoadInfo GetNodeLoadInfo(const CNetAddr& addr) const;
    
    /** Get geographic distribution statistics */
    std::map<std::string, int> GetGeographicDistribution() const;
    
    /** Get list of nodes sorted by load */
    std::vector<std::pair<CNetAddr, NodeLoadInfo>> GetNodesByLoad(bool ascending = true) const;
    
    /** Get recommended nodes for new connections */
    std::vector<CNetAddr> GetRecommendedNodes(int count = 10) const;
    
    /** Check if a node qualifies for incentives */
    bool NodeQualifiesForIncentives(const CNetAddr& addr) const;
    
    /** Calculate incentive reward for a node */
    double CalculateNodeReward(const CNetAddr& addr) const;
    
    /** Register callback for metrics updates */
    void RegisterMetricsCallback(const std::function<void(const NetworkMetrics&)>& callback);
    
    /** Register callback for incentive distribution */
    void RegisterIncentiveCallback(const std::function<void(const CNetAddr&, double)>& callback);
    
    /** Configuration methods */
    void SetAutoScalingEnabled(bool enabled);
    void SetTargetNodeCount(int count);
    void SetLoadThresholds(double minLoad, double maxLoad);
    void SetMaxConnectionsPerNode(int maxConnections);
    void SetGeographicDistributionEnabled(bool enabled);
    void SetMaxNodesPerRegion(int maxNodes);
    void SetIncentiveConfig(const NodeIncentiveConfig& config);
    
    /** Get configuration */
    bool IsAutoScalingEnabled() const { return autoScalingEnabled.load(); }
    int GetTargetNodeCount() const { return targetNodeCount.load(); }
    double GetMaxLoadThreshold() const { return maxLoadThreshold.load(); }
    double GetMinLoadThreshold() const { return minLoadThreshold.load(); }
    bool IsGeographicDistributionEnabled() const { return geoDistributionEnabled.load(); }
    NodeIncentiveConfig GetIncentiveConfig() const;
    
    /** Remove inactive nodes from tracking */
    void CleanupInactiveNodes();
    
    /** Get network health score */
    double CalculateNetworkHealth() const;
    
    /** Get bandwidth utilization statistics */
    std::map<std::string, double> GetBandwidthStats() const;
    
    /** Predict future network capacity needs */
    int PredictCapacityNeeds(int64_t timeHorizon) const;
    
private:
    /** Update network metrics */
    void UpdateNetworkMetrics();
    
    /** Find nodes that need load balancing */
    std::vector<CNetAddr> FindOverloadedNodes() const;
    
    /** Find nodes that can accept more load */
    std::vector<CNetAddr> FindUnderloadedNodes() const;
    
    /** Calculate geographic diversity score */
    double CalculateGeographicDiversity() const;
    
    /** Find regions that need more nodes */
    std::vector<std::string> FindUnderservedRegions() const;
    
    /** Redistribute connections between nodes */
    void RedistributeConnections();
    
    /** Trigger callbacks */
    void TriggerMetricsCallbacks();
    void TriggerIncentiveCallbacks(const CNetAddr& addr, double reward);
    
    /** Estimate node geographic location from IP */
    GeographicInfo EstimateGeographicLocation(const CNetAddr& addr) const;
    
    /** Calculate distance between two geographic points */
    double CalculateDistance(const GeographicInfo& geo1, const GeographicInfo& geo2) const;
};

#endif // BITCOIN_NET_NETWORKSCALER_H