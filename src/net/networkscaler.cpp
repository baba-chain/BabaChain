// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net/networkscaler.h>

#include <net.h>
#include <logging.h>
#include <util/time.h>
#include <util/system.h>

#include <algorithm>
#include <cmath>
#include <random>

CNetworkScaler::CNetworkScaler(CConnman* connman) : connman(connman)
{
    LogPrint(BCLog::NET, "CNetworkScaler: Initialized network auto-scaler\n");
}

CNetworkScaler::~CNetworkScaler()
{
}

void CNetworkScaler::UpdateNodeLoad(const CNetAddr& addr, const NodeLoadInfo& loadInfo)
{
    LOCK(cs_scaler);
    
    nodeLoadMap[addr] = loadInfo;
    nodeLoadMap[addr].lastSeen = GetTime();
    
    // Update network metrics
    UpdateNetworkMetrics();
    
    LogPrint(BCLog::NET, "CNetworkScaler::UpdateNodeLoad: Updated load for %s (CPU: %.1f%%, Mem: %.1f%%, Connections: %d)\n",
             addr.ToString(), loadInfo.cpuLoad, loadInfo.memoryUsage, loadInfo.connectionCount);
}

void CNetworkScaler::UpdateNodeGeography(const CNetAddr& addr, const GeographicInfo& geoInfo)
{
    LOCK(cs_scaler);
    
    nodeGeoMap[addr] = geoInfo;
    
    LogPrint(BCLog::NET, "CNetworkScaler::UpdateNodeGeography: Updated geography for %s (%s, %s)\n",
             addr.ToString(), geoInfo.country.c_str(), geoInfo.city.c_str());
}

void CNetworkScaler::PerformLoadBalancing()
{
    if (!autoScalingEnabled.load()) {
        return;
    }
    
    LOCK(cs_scaler);
    
    // Find overloaded and underloaded nodes
    std::vector<CNetAddr> overloadedNodes = FindOverloadedNodes();
    std::vector<CNetAddr> underloadedNodes = FindUnderloadedNodes();
    
    if (overloadedNodes.empty() || underloadedNodes.empty()) {
        return; // No balancing needed
    }
    
    LogPrint(BCLog::NET, "CNetworkScaler::PerformLoadBalancing: Found %d overloaded and %d underloaded nodes\n",
             overloadedNodes.size(), underloadedNodes.size());
    
    // Redistribute connections
    RedistributeConnections();
    
    // Update metrics after balancing
    UpdateNetworkMetrics();
    TriggerMetricsCallbacks();
}

void CNetworkScaler::ScaleNetworkCapacity()
{
    if (!autoScalingEnabled.load()) {
        return;
    }
    
    LOCK(cs_scaler);
    
    int currentActiveNodes = currentMetrics.activeNodes;
    int targetNodes = targetNodeCount.load();
    double averageLoad = currentMetrics.averageLoad;
    
    // Determine if we need to scale up or down
    bool needScaleUp = (averageLoad > maxLoadThreshold.load()) || (currentActiveNodes < targetNodes * 0.8);
    bool needScaleDown = (averageLoad < minLoadThreshold.load()) && (currentActiveNodes > targetNodes * 1.2);
    
    if (needScaleUp) {
        LogPrint(BCLog::NET, "CNetworkScaler::ScaleNetworkCapacity: Network needs scaling up (load: %.1f%%, nodes: %d/%d)\n",
                 averageLoad, currentActiveNodes, targetNodes);
        
        // Request more nodes (this would typically involve signaling to the network)
        // For now, we just log the recommendation
        
    } else if (needScaleDown) {
        LogPrint(BCLog::NET, "CNetworkScaler::ScaleNetworkCapacity: Network can scale down (load: %.1f%%, nodes: %d/%d)\n",
                 averageLoad, currentActiveNodes, targetNodes);
        
        // Identify nodes that can be safely reduced
        std::vector<CNetAddr> candidatesForReduction = FindUnderloadedNodes();
        
        // Gradually reduce connections to underloaded nodes
        for (const auto& addr : candidatesForReduction) {
            if (nodeLoadMap[addr].connectionCount > 8) { // Keep minimum connections
                LogPrint(BCLog::NET, "CNetworkScaler: Recommending connection reduction for %s\n", addr.ToString());
            }
        }
    }
}

void CNetworkScaler::OptimizeGeographicDistribution()
{
    if (!geoDistributionEnabled.load()) {
        return;
    }
    
    LOCK(cs_scaler);
    
    // Calculate current geographic diversity
    double diversityScore = CalculateGeographicDiversity();
    
    // Find underserved regions
    std::vector<std::string> underservedRegions = FindUnderservedRegions();
    
    if (!underservedRegions.empty()) {
        LogPrint(BCLog::NET, "CNetworkScaler::OptimizeGeographicDistribution: Found %d underserved regions (diversity: %.2f)\n",
                 underservedRegions.size(), diversityScore);
        
        for (const auto& region : underservedRegions) {
            LogPrint(BCLog::NET, "CNetworkScaler: Region %s needs more nodes\n", region.c_str());
        }
    }
    
    // Check for regions with too many nodes
    std::map<std::string, int> regionCounts = GetGeographicDistribution();
    int maxPerRegion = maxNodesPerRegion.load();
    
    for (const auto& [region, count] : regionCounts) {
        if (count > maxPerRegion) {
            LogPrint(BCLog::NET, "CNetworkScaler: Region %s has too many nodes (%d > %d)\n",
                     region.c_str(), count, maxPerRegion);
        }
    }
}

void CNetworkScaler::CalculateNodeIncentives()
{
    if (!incentiveConfig.enableIncentives) {
        return;
    }
    
    LOCK(cs_scaler);
    
    for (const auto& [addr, loadInfo] : nodeLoadMap) {
        if (NodeQualifiesForIncentives(addr)) {
            double reward = CalculateNodeReward(addr);
            
            if (reward > 0.0) {
                LogPrint(BCLog::NET, "CNetworkScaler::CalculateNodeIncentives: Node %s qualifies for reward: %.4f\n",
                         addr.ToString(), reward);
                
                TriggerIncentiveCallbacks(addr, reward);
            }
        }
    }
}

NetworkMetrics CNetworkScaler::GetNetworkMetrics() const
{
    LOCK(cs_scaler);
    return currentMetrics;
}

NodeLoadInfo CNetworkScaler::GetNodeLoadInfo(const CNetAddr& addr) const
{
    LOCK(cs_scaler);
    
    auto it = nodeLoadMap.find(addr);
    if (it != nodeLoadMap.end()) {
        return it->second;
    }
    
    return NodeLoadInfo(); // Return empty info if not found
}

std::map<std::string, int> CNetworkScaler::GetGeographicDistribution() const
{
    LOCK(cs_scaler);
    
    std::map<std::string, int> distribution;
    
    for (const auto& [addr, geoInfo] : nodeGeoMap) {
        distribution[geoInfo.country]++;
    }
    
    return distribution;
}

std::vector<std::pair<CNetAddr, NodeLoadInfo>> CNetworkScaler::GetNodesByLoad(bool ascending) const
{
    LOCK(cs_scaler);
    
    std::vector<std::pair<CNetAddr, NodeLoadInfo>> nodes;
    
    for (const auto& [addr, loadInfo] : nodeLoadMap) {
        nodes.emplace_back(addr, loadInfo);
    }
    
    std::sort(nodes.begin(), nodes.end(),
        [ascending](const auto& a, const auto& b) {
            double loadA = (a.second.cpuLoad + a.second.memoryUsage) / 2.0;
            double loadB = (b.second.cpuLoad + b.second.memoryUsage) / 2.0;
            return ascending ? (loadA < loadB) : (loadA > loadB);
        });
    
    return nodes;
}

std::vector<CNetAddr> CNetworkScaler::GetRecommendedNodes(int count) const
{
    LOCK(cs_scaler);
    
    std::vector<CNetAddr> recommended;
    
    // Get nodes sorted by load (ascending - prefer less loaded nodes)
    auto nodesByLoad = GetNodesByLoad(true);
    
    for (const auto& [addr, loadInfo] : nodesByLoad) {
        if (recommended.size() >= static_cast<size_t>(count)) {
            break;
        }
        
        // Check if node is suitable for new connections
        if (loadInfo.connectionCount < maxConnectionsPerNode.load() &&
            loadInfo.reliability > 0.7 &&
            (loadInfo.cpuLoad + loadInfo.memoryUsage) / 2.0 < 70.0) {
            
            recommended.push_back(addr);
        }
    }
    
    return recommended;
}

bool CNetworkScaler::NodeQualifiesForIncentives(const CNetAddr& addr) const
{
    LOCK(cs_scaler);
    
    auto it = nodeLoadMap.find(addr);
    if (it == nodeLoadMap.end()) {
        return false;
    }
    
    const NodeLoadInfo& info = it->second;
    
    return info.isFullNode &&
           info.uptime >= incentiveConfig.minUptimeForReward &&
           info.reliability >= incentiveConfig.minReliabilityForReward;
}

double CNetworkScaler::CalculateNodeReward(const CNetAddr& addr) const
{
    LOCK(cs_scaler);
    
    auto it = nodeLoadMap.find(addr);
    if (it == nodeLoadMap.end()) {
        return 0.0;
    }
    
    const NodeLoadInfo& info = it->second;
    
    double reward = incentiveConfig.baseReward;
    
    // Apply uptime multiplier
    double uptimeHours = info.uptime / 3600.0;
    if (uptimeHours > 24.0) {
        reward *= incentiveConfig.uptimeMultiplier;
    }
    
    // Apply bandwidth multiplier
    if (info.bandwidth > 1000000) { // > 1 MB/s
        reward *= incentiveConfig.bandwidthMultiplier;
    }
    
    // Apply reliability multiplier
    if (info.reliability > 0.9) {
        reward *= incentiveConfig.reliabilityMultiplier;
    }
    
    // Bonus for staking nodes
    if (info.isStakingNode && info.stakingWeight > 0.0) {
        reward *= (1.0 + info.stakingWeight / 100.0);
    }
    
    return reward;
}

void CNetworkScaler::RegisterMetricsCallback(const std::function<void(const NetworkMetrics&)>& callback)
{
    LOCK(cs_scaler);
    metricsCallbacks.push_back(callback);
}

void CNetworkScaler::RegisterIncentiveCallback(const std::function<void(const CNetAddr&, double)>& callback)
{
    LOCK(cs_scaler);
    incentiveCallbacks.push_back(callback);
}

void CNetworkScaler::SetAutoScalingEnabled(bool enabled)
{
    autoScalingEnabled.store(enabled);
    LogPrint(BCLog::NET, "CNetworkScaler: Auto-scaling %s\n", enabled ? "enabled" : "disabled");
}

void CNetworkScaler::SetTargetNodeCount(int count)
{
    targetNodeCount.store(std::max(10, count));
    LogPrint(BCLog::NET, "CNetworkScaler: Target node count set to %d\n", count);
}

void CNetworkScaler::SetLoadThresholds(double minLoad, double maxLoad)
{
    minLoadThreshold.store(std::max(0.0, std::min(100.0, minLoad)));
    maxLoadThreshold.store(std::max(0.0, std::min(100.0, maxLoad)));
    LogPrint(BCLog::NET, "CNetworkScaler: Load thresholds set to %.1f%% - %.1f%%\n", minLoad, maxLoad);
}

void CNetworkScaler::SetMaxConnectionsPerNode(int maxConnections)
{
    maxConnectionsPerNode.store(std::max(8, maxConnections));
    LogPrint(BCLog::NET, "CNetworkScaler: Max connections per node set to %d\n", maxConnections);
}

void CNetworkScaler::SetGeographicDistributionEnabled(bool enabled)
{
    geoDistributionEnabled.store(enabled);
    LogPrint(BCLog::NET, "CNetworkScaler: Geographic distribution %s\n", enabled ? "enabled" : "disabled");
}

void CNetworkScaler::SetMaxNodesPerRegion(int maxNodes)
{
    maxNodesPerRegion.store(std::max(1, maxNodes));
    LogPrint(BCLog::NET, "CNetworkScaler: Max nodes per region set to %d\n", maxNodes);
}

void CNetworkScaler::SetIncentiveConfig(const NodeIncentiveConfig& config)
{
    LOCK(cs_scaler);
    incentiveConfig = config;
    LogPrint(BCLog::NET, "CNetworkScaler: Updated incentive configuration\n");
}

NodeIncentiveConfig CNetworkScaler::GetIncentiveConfig() const
{
    LOCK(cs_scaler);
    return incentiveConfig;
}

void CNetworkScaler::CleanupInactiveNodes()
{
    LOCK(cs_scaler);
    
    int64_t now = GetTime();
    int64_t timeout = 300; // 5 minutes
    
    auto it = nodeLoadMap.begin();
    while (it != nodeLoadMap.end()) {
        if (now - it->second.lastSeen > timeout) {
            LogPrint(BCLog::NET, "CNetworkScaler::CleanupInactiveNodes: Removing inactive node %s\n",
                     it->first.ToString());
            
            nodeGeoMap.erase(it->first);
            it = nodeLoadMap.erase(it);
        } else {
            ++it;
        }
    }
    
    UpdateNetworkMetrics();
}

double CNetworkScaler::CalculateNetworkHealth() const
{
    LOCK(cs_scaler);
    
    if (nodeLoadMap.empty()) {
        return 0.0;
    }
    
    double totalHealth = 0.0;
    int nodeCount = 0;
    
    for (const auto& [addr, loadInfo] : nodeLoadMap) {
        double nodeHealth = loadInfo.reliability;
        
        // Penalize high load
        double avgLoad = (loadInfo.cpuLoad + loadInfo.memoryUsage) / 2.0;
        if (avgLoad > 80.0) {
            nodeHealth *= (100.0 - avgLoad) / 20.0; // Reduce health for high load
        }
        
        // Bonus for uptime
        if (loadInfo.uptime > 86400) { // > 24 hours
            nodeHealth *= 1.1;
        }
        
        totalHealth += std::min(1.0, nodeHealth);
        nodeCount++;
    }
    
    return totalHealth / nodeCount;
}

std::map<std::string, double> CNetworkScaler::GetBandwidthStats() const
{
    LOCK(cs_scaler);
    
    std::map<std::string, double> stats;
    
    double totalBandwidth = 0.0;
    double usedBandwidth = 0.0;
    int nodeCount = 0;
    
    for (const auto& [addr, loadInfo] : nodeLoadMap) {
        totalBandwidth += loadInfo.bandwidth;
        usedBandwidth += loadInfo.bandwidth * (loadInfo.cpuLoad / 100.0);
        nodeCount++;
    }
    
    stats["total_bandwidth"] = totalBandwidth;
    stats["used_bandwidth"] = usedBandwidth;
    stats["available_bandwidth"] = totalBandwidth - usedBandwidth;
    stats["utilization_percentage"] = totalBandwidth > 0 ? (usedBandwidth / totalBandwidth * 100.0) : 0.0;
    stats["average_per_node"] = nodeCount > 0 ? (totalBandwidth / nodeCount) : 0.0;
    
    return stats;
}

int CNetworkScaler::PredictCapacityNeeds(int64_t timeHorizon) const
{
    LOCK(cs_scaler);
    
    // Simple prediction based on current growth trends
    // In a real implementation, this would use more sophisticated algorithms
    
    int currentNodes = currentMetrics.activeNodes;
    double currentLoad = currentMetrics.averageLoad;
    
    // Assume 10% growth per month
    double monthsAhead = timeHorizon / (30.0 * 24.0 * 3600.0);
    double growthFactor = std::pow(1.1, monthsAhead);
    
    int predictedNodes = static_cast<int>(currentNodes * growthFactor);
    
    // Adjust based on current load
    if (currentLoad > 70.0) {
        predictedNodes = static_cast<int>(predictedNodes * 1.2); // Need more capacity
    } else if (currentLoad < 30.0) {
        predictedNodes = static_cast<int>(predictedNodes * 0.9); // Can reduce capacity
    }
    
    return std::max(currentNodes, predictedNodes);
}

void CNetworkScaler::UpdateNetworkMetrics()
{
    // This method is called with cs_scaler already locked
    
    currentMetrics = NetworkMetrics();
    
    if (nodeLoadMap.empty()) {
        return;
    }
    
    double totalLoad = 0.0;
    double totalBandwidth = 0.0;
    
    for (const auto& [addr, loadInfo] : nodeLoadMap) {
        currentMetrics.totalNodes++;
        
        if (GetTime() - loadInfo.lastSeen < 300) { // Active in last 5 minutes
            currentMetrics.activeNodes++;
        }
        
        if (loadInfo.isFullNode) {
            currentMetrics.fullNodes++;
        }
        
        if (loadInfo.isStakingNode) {
            currentMetrics.stakingNodes++;
        }
        
        totalLoad += (loadInfo.cpuLoad + loadInfo.memoryUsage) / 2.0;
        totalBandwidth += loadInfo.bandwidth;
    }
    
    currentMetrics.averageLoad = totalLoad / currentMetrics.totalNodes;
    currentMetrics.totalBandwidth = totalBandwidth;
    currentMetrics.networkHealth = CalculateNetworkHealth();
}

std::vector<CNetAddr> CNetworkScaler::FindOverloadedNodes() const
{
    std::vector<CNetAddr> overloaded;
    double threshold = maxLoadThreshold.load();
    
    for (const auto& [addr, loadInfo] : nodeLoadMap) {
        double avgLoad = (loadInfo.cpuLoad + loadInfo.memoryUsage) / 2.0;
        if (avgLoad > threshold || loadInfo.connectionCount > maxConnectionsPerNode.load()) {
            overloaded.push_back(addr);
        }
    }
    
    return overloaded;
}

std::vector<CNetAddr> CNetworkScaler::FindUnderloadedNodes() const
{
    std::vector<CNetAddr> underloaded;
    double threshold = minLoadThreshold.load();
    
    for (const auto& [addr, loadInfo] : nodeLoadMap) {
        double avgLoad = (loadInfo.cpuLoad + loadInfo.memoryUsage) / 2.0;
        if (avgLoad < threshold && loadInfo.connectionCount < maxConnectionsPerNode.load() / 2) {
            underloaded.push_back(addr);
        }
    }
    
    return underloaded;
}

double CNetworkScaler::CalculateGeographicDiversity() const
{
    if (nodeGeoMap.empty()) {
        return 0.0;
    }
    
    std::map<std::string, int> countryCounts;
    for (const auto& [addr, geoInfo] : nodeGeoMap) {
        countryCounts[geoInfo.country]++;
    }
    
    // Calculate entropy as a measure of diversity
    double entropy = 0.0;
    int totalNodes = nodeGeoMap.size();
    
    for (const auto& [country, count] : countryCounts) {
        double probability = static_cast<double>(count) / totalNodes;
        if (probability > 0.0) {
            entropy -= probability * std::log2(probability);
        }
    }
    
    // Normalize to 0-1 range
    double maxEntropy = std::log2(countryCounts.size());
    return maxEntropy > 0.0 ? (entropy / maxEntropy) : 0.0;
}

std::vector<std::string> CNetworkScaler::FindUnderservedRegions() const
{
    std::vector<std::string> underserved;
    std::map<std::string, int> regionCounts = GetGeographicDistribution();
    
    int averageNodesPerRegion = nodeGeoMap.size() / std::max(1, static_cast<int>(regionCounts.size()));
    
    for (const auto& [region, count] : regionCounts) {
        if (count < averageNodesPerRegion / 2) {
            underserved.push_back(region);
        }
    }
    
    return underserved;
}

void CNetworkScaler::RedistributeConnections()
{
    // This would implement actual connection redistribution
    // For now, we just log the recommendation
    
    std::vector<CNetAddr> overloaded = FindOverloadedNodes();
    std::vector<CNetAddr> underloaded = FindUnderloadedNodes();
    
    LogPrint(BCLog::NET, "CNetworkScaler::RedistributeConnections: Redistributing connections between %d overloaded and %d underloaded nodes\n",
             overloaded.size(), underloaded.size());
}

void CNetworkScaler::TriggerMetricsCallbacks()
{
    for (const auto& callback : metricsCallbacks) {
        try {
            callback(currentMetrics);
        } catch (const std::exception& e) {
            LogPrintf("CNetworkScaler: Exception in metrics callback: %s\n", e.what());
        }
    }
}

void CNetworkScaler::TriggerIncentiveCallbacks(const CNetAddr& addr, double reward)
{
    for (const auto& callback : incentiveCallbacks) {
        try {
            callback(addr, reward);
        } catch (const std::exception& e) {
            LogPrintf("CNetworkScaler: Exception in incentive callback: %s\n", e.what());
        }
    }
}

GeographicInfo CNetworkScaler::EstimateGeographicLocation(const CNetAddr& addr) const
{
    // This would implement IP geolocation
    // For now, return a placeholder
    GeographicInfo info;
    info.country = "Unknown";
    info.region = "Unknown";
    info.city = "Unknown";
    return info;
}

double CNetworkScaler::CalculateDistance(const GeographicInfo& geo1, const GeographicInfo& geo2) const
{
    // Haversine formula for calculating distance between two points on Earth
    const double R = 6371.0; // Earth's radius in kilometers
    
    double lat1Rad = geo1.latitude * M_PI / 180.0;
    double lat2Rad = geo2.latitude * M_PI / 180.0;
    double deltaLatRad = (geo2.latitude - geo1.latitude) * M_PI / 180.0;
    double deltaLonRad = (geo2.longitude - geo1.longitude) * M_PI / 180.0;
    
    double a = std::sin(deltaLatRad / 2.0) * std::sin(deltaLatRad / 2.0) +
               std::cos(lat1Rad) * std::cos(lat2Rad) *
               std::sin(deltaLonRad / 2.0) * std::sin(deltaLonRad / 2.0);
    
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    
    return R * c;
}