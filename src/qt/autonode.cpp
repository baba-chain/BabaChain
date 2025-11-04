// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/autonode.h>

#include <qt/clientmodel.h>
#include <interfaces/node.h>
#include <util/system.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QHostAddress>
#include <QRandomGenerator>

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
    
    // Get current connection count
    connectionCount = clientModel->getNumConnections();
    
    // If we have too few connections, try to connect to more peers
    if (connectionCount < targetConnections) {
        connectToBestPeers();
    }
    
    // Evaluate and score current peers
    evaluateNetworkHealth();
}

void AutoNodeManager::checkNetworkHealth()
{
    if (!clientModel) {
        return;
    }
    
    // Calculate network health score based on various metrics
    int healthScore = 0;
    
    // Connection count (0-30 points)
    connectionCount = clientModel->getNumConnections();
    if (connectionCount >= targetConnections) {
        healthScore += 30;
    } else {
        healthScore += (connectionCount * 30) / targetConnections;
    }
    
    // Sync progress (0-40 points)
    if (clientModel->node().isInitialBlockDownload()) {
        syncProgress = static_cast<int>(clientModel->getVerificationProgress() * 100);
        healthScore += (syncProgress * 40) / 100;
    } else {
        healthScore += 40; // Fully synced
        syncProgress = 100;
    }
    
    // Network activity (0-30 points)
    if (clientModel->getNetworkActive()) {
        healthScore += 30;
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
            
            for (const QJsonValue& value : nodes) {
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
    
    // Shuffle the list for better distribution
    std::random_shuffle(knownSeedNodes.begin(), knownSeedNodes.end());
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
        QNetworkRequest request(QUrl(source));
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
    
    // Check if we need self-healing
    bool needsHealing = false;
    
    // Check connection count
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
    
    // Get current peer statistics and optimize connections
    connectionCount = clientModel->getNumConnections();
    
    // If we have too many connections, disconnect from worst performers
    if (connectionCount > maxConnections) {
        // This would need to be implemented in the client model
        // to disconnect from specific peers based on performance
    }
    
    // Update peer scores based on performance
    int64_t currentTime = QDateTime::currentSecsSinceEpoch();
    for (auto it = peerLastSeen.begin(); it != peerLastSeen.end(); ++it) {
        QString peer = it.key();
        int64_t lastSeen = it.value();
        
        // Decrease score for peers not seen recently
        if ((currentTime - lastSeen) > 600) { // 10 minutes
            peerScores[peer] = qMax(0, peerScores[peer] - 10);
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
}

void AutoNodeManager::monitorNetworkCapacity()
{
    if (!clientModel) {
        return;
    }
    
    // Monitor network capacity and adjust target connections accordingly
    
    // Get current network statistics
    connectionCount = clientModel->getNumConnections();
    bool isIBD = clientModel->node().isInitialBlockDownload();
    
    // Adjust target connections based on network conditions
    if (isIBD) {
        // During initial block download, we want more connections for faster sync
        targetConnections = qMin(maxConnections, DEFAULT_TARGET_CONNECTIONS * 2);
    } else {
        // Normal operation
        targetConnections = DEFAULT_TARGET_CONNECTIONS;
    }
    
    // Adjust discovery interval based on connection health
    if (connectionCount < targetConnections / 2) {
        // Poor connectivity - discover more frequently
        discoveryInterval = DEFAULT_DISCOVERY_INTERVAL / 2;
    } else if (connectionCount >= targetConnections) {
        // Good connectivity - discover less frequently
        discoveryInterval = DEFAULT_DISCOVERY_INTERVAL * 2;
    } else {
        // Normal discovery interval
        discoveryInterval = DEFAULT_DISCOVERY_INTERVAL;
    }
    
    // Update timer intervals
    if (discoveryTimer && discoveryTimer->isActive()) {
        discoveryTimer->setInterval(discoveryInterval);
    }
}