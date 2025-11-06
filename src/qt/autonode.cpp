// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/autonode.h>

#include <qt/clientmodel.h>
#include <interfaces/node.h>
#include <util/system.h>
#include <sync.h>
#include <validation.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QHostAddress>
#include <QRandomGenerator>
#include <algorithm>
#include <random>

// Built-in seed nodes for BabaChain network
const QStringList AutoNodeManager::BUILTIN_SEED_NODES = {
    "seed1.babachain.org:9999",
    "seed2.babachain.org:9999", 
    "seed3.babachain.org:9999",
    "seed4.babachain.org:9999",
    "node1.babachain.network:9999",
    "node2.babachain.network:9999",
    "bootstrap.babachain.io:9999"
};

AutoNodeManager::AutoNodeManager(QObject* parent)
    : QObject(parent),
      networkManager(nullptr),
      discoveryTimer(nullptr),
      healthTimer(nullptr),
      optimizationTimer(nullptr),
      autoDiscoveryEnabled(true),
      maxConnections(DEFAULT_MAX_CONNECTIONS),
      targetConnections(DEFAULT_TARGET_CONNECTIONS),
      discoveryInterval(DEFAULT_DISCOVERY_INTERVAL),
      healthCheckInterval(DEFAULT_HEALTH_CHECK_INTERVAL),
      networkHealthScore(0),
      connectionCount(0),
      syncProgress(0),
      failedConnectionAttempts(0),
      lastSuccessfulConnection(0),
      clientModel(nullptr)
{
    networkManager = new QNetworkAccessManager(this);
    
    // Initialize timers
    discoveryTimer = new QTimer(this);
    discoveryTimer->setSingleShot(false);
    connect(discoveryTimer, &QTimer::timeout, this, &AutoNodeManager::performPeerDiscovery);
    
    healthTimer = new QTimer(this);
    healthTimer->setSingleShot(false);
    connect(healthTimer, &QTimer::timeout, this, &AutoNodeManager::checkNetworkHealth);
    
    optimizationTimer = new QTimer(this);
    optimizationTimer->setSingleShot(false);
    connect(optimizationTimer, &QTimer::timeout, this, &AutoNodeManager::optimizeConnections);
    
    selfHealingTimer = new QTimer(this);
    selfHealingTimer->setSingleShot(false);
    connect(selfHealingTimer, &QTimer::timeout, this, &AutoNodeManager::performSelfHealing);
    
    // Load built-in seed nodes
    loadBuiltinSeedNodes();
}

AutoNodeManager::~AutoNodeManager()
{
    stopAutoDiscovery();
}

void AutoNodeManager::setClientModel(ClientModel* clientModel)
{
    this->clientModel = clientModel;
    
    if (clientModel) {
        // Connect to ClientModel signals for comprehensive network monitoring
        connect(clientModel, &ClientModel::numBlocksChanged,
                this, [this, clientModel](int count, const QDateTime& blockDate, const QString& blockHash, 
                             double nVerificationProgress, bool header, SynchronizationState sync_state) {
            Q_UNUSED(count)
            Q_UNUSED(blockDate)
            Q_UNUSED(blockHash)
            Q_UNUSED(sync_state)
            if (!header) {
                syncProgress = static_cast<int>(nVerificationProgress * 100);
                Q_EMIT autoSyncProgress(syncProgress);
                
                // Adjust discovery behavior based on sync state
                if (clientModel->node().isInitialBlockDownload()) {
                    // During IBD, be more aggressive with peer discovery
                    if (connectionCount < targetConnections) {
                        performPeerDiscovery();
                    }
                }
            }
        });
        
        // Monitor connection count changes for network health
        connect(clientModel, &ClientModel::numConnectionsChanged,
                this, [this](int count) {
            int previousCount = connectionCount;
            connectionCount = count;
            
            // Update last successful connection time if connections increased
            if (count > previousCount) {
                lastSuccessfulConnection = QDateTime::currentSecsSinceEpoch();
                failedConnectionAttempts = 0;
            }
            
            // Trigger self-healing if connections dropped significantly
            if (count < targetConnections / 2 && previousCount >= targetConnections / 2) {
                performSelfHealing();
            }
            
            // Update network health based on connection changes
            checkNetworkHealth();
        });
        
        // Monitor network activity for connectivity issues
        connect(clientModel, &ClientModel::networkActiveChanged,
                this, [this](bool networkActive) {
            if (!networkActive) {
                // Network became inactive - trigger self-healing
                performSelfHealing();
            } else {
                // Network reactivated - resume normal discovery
                if (autoDiscoveryEnabled) {
                    performPeerDiscovery();
                }
            }
            checkNetworkHealth();
        });
        
        // Monitor additional sync progress for masternode/governance data
        connect(clientModel, &ClientModel::additionalDataSyncProgressChanged,
                this, [this](double nSyncProgress) {
            // Additional data sync affects network health
            Q_UNUSED(nSyncProgress)
            checkNetworkHealth();
        });
    }
}

void AutoNodeManager::startAutoDiscovery()
{
    if (!autoDiscoveryEnabled) {
        return;
    }
    
    // Start periodic peer discovery
    discoveryTimer->start(discoveryInterval);
    
    // Start network health monitoring
    healthTimer->start(healthCheckInterval);
    
    // Start connection optimization (less frequent)
    optimizationTimer->start(discoveryInterval * 2);
    
    // Start self-healing monitoring
    selfHealingTimer->start(healthCheckInterval * 3);
    
    // Perform initial discovery
    performPeerDiscovery();
    checkNetworkHealth();
}

void AutoNodeManager::stopAutoDiscovery()
{
    if (discoveryTimer) {
        discoveryTimer->stop();
    }
    if (healthTimer) {
        healthTimer->stop();
    }
    if (optimizationTimer) {
        optimizationTimer->stop();
    }
    if (selfHealingTimer) {
        selfHealingTimer->stop();
    }
}

void AutoNodeManager::setAutoDiscoveryEnabled(bool enabled)
{
    autoDiscoveryEnabled = enabled;
    
    if (enabled) {
        startAutoDiscovery();
    } else {
        stopAutoDiscovery();
    }
}

void AutoNodeManager::performPeerDiscovery()
{
    if (!clientModel || !autoDiscoveryEnabled) {
        return;
    }
    
    // Try to discover peers from multiple sources
    bootstrapFromMultipleSources();
    
    // Connect to best available peers
    connectToBestPeers();
}

void AutoNodeManager::optimizeConnections()
{
    if (!clientModel) {
        return;
    }
    
    // Get current connection count using ClientModel API
    connectionCount = clientModel->getNumConnections();
    
    // If we have too few connections, try to connect to more peers
    if (connectionCount < targetConnections) {
        connectToBestPeers();
    }
    
    // Monitor network capacity and adjust accordingly
    monitorNetworkCapacity();
    
    // Evaluate and score current peers
    evaluateNetworkHealth();
}

void AutoNodeManager::checkNetworkHealth()
{
    if (!clientModel) {
        return;
    }
    
    // Calculate network health score based on various metrics using ClientModel APIs
    int healthScore = 0;
    
    // Connection count (0-30 points) - use ClientModel API
    connectionCount = clientModel->getNumConnections();
    if (connectionCount >= targetConnections) {
        healthScore += 30;
    } else {
        healthScore += (connectionCount * 30) / targetConnections;
    }
    
    // Sync progress (0-40 points) - use cached sync progress from signals
    if (clientModel->node().isInitialBlockDownload()) {
        // syncProgress is updated via signal connections, use cached value
        healthScore += (syncProgress * 40) / 100;
    } else {
        healthScore += 40; // Fully synced
        syncProgress = 100;
    }
    
    // Network activity (0-30 points) - check block source and connections
    BlockSource blockSource = clientModel->getBlockSource();
    if (blockSource == BlockSource::NETWORK && connectionCount > 0) {
        healthScore += 30;
    } else if (blockSource == BlockSource::DISK && connectionCount > 0) {
        healthScore += 20; // Loading from disk but have connections
    } else if (connectionCount > 0) {
        healthScore += 10; // Have connections but no clear block source
    }
    
    // Adjust score based on recent connection failures
    if (failedConnectionAttempts > 5) {
        healthScore = qMax(0, healthScore - (failedConnectionAttempts * 2));
    }
    
    networkHealthScore = healthScore;
    
    Q_EMIT networkHealthChanged(networkHealthScore);
    Q_EMIT autoSyncProgress(syncProgress);
}

void AutoNodeManager::onSeedNodesReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QJsonArray nodes = obj["nodes"].toArray();
            
            for (const QJsonValue value : nodes) {
                QJsonObject node = value.toObject();
                QString address = node["address"].toString();
                int port = node["port"].toInt();
                
                if (!address.isEmpty() && port > 0) {
                    QString peerAddress = QString("%1:%2").arg(address).arg(port);
                    if (!discoveredPeers.contains(peerAddress)) {
                        discoveredPeers.append(peerAddress);
                        Q_EMIT peerDiscovered(address, port);
                    }
                }
            }
        }
    }
    
    reply->deleteLater();
}

void AutoNodeManager::onPeerDiscoveryTimeout()
{
    // Handle discovery timeout - try alternative methods
    loadBuiltinSeedNodes();
}

void AutoNodeManager::onNetworkHealthTimeout()
{
    // Handle health check timeout
    networkHealthScore = qMax(0, networkHealthScore - 10);
    Q_EMIT networkHealthChanged(networkHealthScore);
}

void AutoNodeManager::loadBuiltinSeedNodes()
{
    knownSeedNodes.clear();
    knownSeedNodes.append(BUILTIN_SEED_NODES);
    
    // Shuffle the list for better distribution using Qt 6 compatible method
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(knownSeedNodes.begin(), knownSeedNodes.end(), g);
}

void AutoNodeManager::connectToBestPeers()
{
    if (!clientModel) {
        return;
    }
    
    // Combine known seed nodes and discovered peers
    QStringList allPeers = knownSeedNodes + discoveredPeers;
    
    // Remove duplicates
    allPeers.removeDuplicates();
    
    // Try to connect to the best peers
    int connectionsNeeded = targetConnections - connectionCount;
    int connected = 0;
    
    for (const QString& peer : allPeers) {
        if (connected >= connectionsNeeded) {
            break;
        }
        
        QStringList parts = peer.split(':');
        if (parts.size() == 2) {
            QString address = parts[0];
            bool ok;
            int port = parts[1].toInt(&ok);
            
            if (ok && port > 0) {
                // Try to connect to this peer
                // Note: This would need to be implemented in the client model
                // For now, we just emit the signal
                Q_EMIT peerDiscovered(address, port);
                connected++;
            }
        }
    }
}

void AutoNodeManager::evaluateNetworkHealth()
{
    // This would evaluate the quality of current connections
    // and potentially disconnect from poor performing peers
    
    // For now, just update the health score
    checkNetworkHealth();
}

void AutoNodeManager::bootstrapFromMultipleSources()
{
    // Try to get peer lists from multiple sources
    
    // 1. Try DNS seeds
    QStringList dnsSeeds = {
        "dnsseed.babachain.org",
        "seed.babachain.network", 
        "nodes.babachain.io"
    };
    
    for (const QString& dnsSeed : dnsSeeds) {
        Q_UNUSED(dnsSeed)
        // This would perform DNS lookup for peer addresses
        // For now, we use the built-in seed nodes
    }
    
    // 2. Try web-based peer discovery
    QStringList webSources = {
        "https://api.babachain.org/peers",
        "https://nodes.babachain.network/api/peers",
        "https://explorer.babachain.io/api/nodes"
    };
    
    for (const QString& source : webSources) {
        QNetworkRequest request;
        request.setUrl(QUrl(source));
        request.setRawHeader("User-Agent", "BabaChain-Qt/1.0");
        
        QNetworkReply* reply = networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, &AutoNodeManager::onSeedNodesReplyFinished);
        
        // Set timeout for the request
        QTimer::singleShot(10000, reply, &QNetworkReply::abort);
    }
    
    // 3. Use built-in seed nodes as fallback
    if (discoveredPeers.isEmpty()) {
        loadBuiltinSeedNodes();
    }
}

void AutoNodeManager::performSelfHealing()
{
    if (!clientModel || !autoDiscoveryEnabled) {
        return;
    }
    
    // Check if we need self-healing using ClientModel APIs
    bool needsHealing = false;
    
    // Check connection count using ClientModel API
    connectionCount = clientModel->getNumConnections();
    if (connectionCount < targetConnections / 2) {
        needsHealing = true;
    }
    
    // Check if we haven't had a successful connection in a while
    int64_t currentTime = QDateTime::currentSecsSinceEpoch();
    if (lastSuccessfulConnection > 0 && 
        (currentTime - lastSuccessfulConnection) > 300) { // 5 minutes
        needsHealing = true;
    }
    
    // Check if too many connection attempts have failed
    if (failedConnectionAttempts > 10) {
        needsHealing = true;
    }
    
    // Check if we're in IBD but have very poor connectivity
    if (clientModel->node().isInitialBlockDownload() && connectionCount == 0) {
        needsHealing = true;
    }
    
    // Check block source - if we can't get blocks from network, we need healing
    if (clientModel->getBlockSource() == BlockSource::NONE && connectionCount > 0) {
        needsHealing = true;
    }
    
    if (needsHealing) {
        // Perform self-healing actions
        
        // 1. Reset failed connection counter
        failedConnectionAttempts = 0;
        
        // 2. Clear poor performing peers
        QStringList peersToRemove;
        for (auto it = peerFailureCount.begin(); it != peerFailureCount.end(); ++it) {
            if (it.value() > 5) {
                peersToRemove.append(it.key());
            }
        }
        
        for (const QString& peer : peersToRemove) {
            discoveredPeers.removeAll(peer);
            peerFailureCount.remove(peer);
            peerScores.remove(peer);
        }
        
        // 3. Reload seed nodes
        loadBuiltinSeedNodes();
        
        // 4. Try alternative discovery methods
        bootstrapFromMultipleSources();
        
        // 5. Attempt to reconnect to best peers
        connectToBestPeers();
        
        // 6. Update network health
        checkNetworkHealth();
    }
}

void AutoNodeManager::optimizePeerConnections()
{
    if (!clientModel) {
        return;
    }
    
    // Get current peer statistics and optimize connections using ClientModel APIs
    connectionCount = clientModel->getNumConnections();
    BlockSource blockSource = clientModel->getBlockSource();
    
    // If we have too many connections but poor block source, prioritize quality over quantity
    if (connectionCount > maxConnections || 
        (connectionCount > targetConnections && blockSource != BlockSource::NETWORK)) {
        // Mark poor performing peers for potential removal
        // This would need to be implemented in the client model
        // to disconnect from specific peers based on performance
        
        // For now, we'll just reduce our target to encourage better peer selection
        targetConnections = qMax(DEFAULT_TARGET_CONNECTIONS / 2, targetConnections - 2);
    }
    
    // Update peer scores based on performance and network conditions
    int64_t currentTime = QDateTime::currentSecsSinceEpoch();
    for (auto it = peerLastSeen.begin(); it != peerLastSeen.end(); ++it) {
        QString peer = it.key();
        int64_t lastSeen = it.value();
        
        // Decrease score for peers not seen recently
        if ((currentTime - lastSeen) > 600) { // 10 minutes
            peerScores[peer] = qMax(-100, peerScores[peer] - 10);
        }
        
        // Bonus points for peers that help with network sync
        if (blockSource == BlockSource::NETWORK && (currentTime - lastSeen) < 60) {
            peerScores[peer] = qMin(100, peerScores[peer] + 5);
        }
    }
    
    // Remove peers with very low scores
    QStringList peersToRemove;
    for (auto it = peerScores.begin(); it != peerScores.end(); ++it) {
        if (it.value() < -50) {
            peersToRemove.append(it.key());
        }
    }
    
    for (const QString& peer : peersToRemove) {
        discoveredPeers.removeAll(peer);
        peerScores.remove(peer);
        peerLastSeen.remove(peer);
        peerFailureCount.remove(peer);
    }
    
    // Monitor network capacity after optimization
    monitorNetworkCapacity();
}

void AutoNodeManager::monitorNetworkCapacity()
{
    if (!clientModel) {
        return;
    }
    
    // Monitor network capacity and adjust target connections accordingly using ClientModel APIs
    
    // Get current network statistics using ClientModel APIs
    connectionCount = clientModel->getNumConnections();
    bool isIBD = clientModel->node().isInitialBlockDownload();
    BlockSource blockSource = clientModel->getBlockSource();
    
    // Adjust target connections based on network conditions
    if (isIBD) {
        // During initial block download, we want more connections for faster sync
        targetConnections = qMin(maxConnections, DEFAULT_TARGET_CONNECTIONS * 2);
    } else {
        // Normal operation
        targetConnections = DEFAULT_TARGET_CONNECTIONS;
    }
    
    // Further adjust based on block source
    if (blockSource == BlockSource::NONE && connectionCount > 0) {
        // We have connections but no block source - increase target to find better peers
        targetConnections = qMin(maxConnections, targetConnections + 2);
    } else if (blockSource == BlockSource::NETWORK && connectionCount >= targetConnections) {
        // Good network sync - can reduce target slightly for efficiency
        targetConnections = qMax(DEFAULT_TARGET_CONNECTIONS, targetConnections - 1);
    }
    
    // Adjust discovery interval based on connection health
    if (connectionCount < targetConnections / 2) {
        // Poor connectivity - discover more frequently
        discoveryInterval = DEFAULT_DISCOVERY_INTERVAL / 2;
    } else if (connectionCount >= targetConnections && blockSource == BlockSource::NETWORK) {
        // Good connectivity and network sync - discover less frequently
        discoveryInterval = DEFAULT_DISCOVERY_INTERVAL * 2;
    } else {
        // Normal discovery interval
        discoveryInterval = DEFAULT_DISCOVERY_INTERVAL;
    }
    
    // Update timer intervals if they're active
    if (discoveryTimer && discoveryTimer->isActive()) {
        discoveryTimer->setInterval(discoveryInterval);
    }
    if (healthTimer && healthTimer->isActive()) {
        // Adjust health check frequency based on network conditions
        int healthInterval = (connectionCount < targetConnections / 2) ? 
                           healthCheckInterval / 2 : healthCheckInterval;
        healthTimer->setInterval(healthInterval);
    }
}