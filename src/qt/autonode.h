// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_AUTONODE_H
#define BITCOIN_QT_AUTONODE_H

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>

class ClientModel;
enum class BlockSource;

/** Auto-node discovery and network optimization system */
class AutoNodeManager : public QObject
{
    Q_OBJECT

public:
    explicit AutoNodeManager(QObject* parent = nullptr);
    ~AutoNodeManager();

    void setClientModel(ClientModel* clientModel);
    void startAutoDiscovery();
    void stopAutoDiscovery();
    
    bool isAutoDiscoveryEnabled() const { return autoDiscoveryEnabled; }
    void setAutoDiscoveryEnabled(bool enabled);

public Q_SLOTS:
    void performPeerDiscovery();
    void optimizeConnections();
    void checkNetworkHealth();

private Q_SLOTS:
    void onSeedNodesReplyFinished();
    void onPeerDiscoveryTimeout();
    void onNetworkHealthTimeout();

Q_SIGNALS:
    void peerDiscovered(const QString& address, int port);
    void networkHealthChanged(int score);
    void autoSyncProgress(int percentage);

private:
    void loadBuiltinSeedNodes();
    void connectToBestPeers();
    void evaluateNetworkHealth();
    void bootstrapFromMultipleSources();
    void performSelfHealing();
    void optimizePeerConnections();
    void monitorNetworkCapacity();
    
    // Network discovery
    QNetworkAccessManager* networkManager;
    QTimer* discoveryTimer;
    QTimer* healthTimer;
    QTimer* optimizationTimer;
    QTimer* selfHealingTimer;
    
    // Configuration
    bool autoDiscoveryEnabled;
    int maxConnections;
    int targetConnections;
    int discoveryInterval;
    int healthCheckInterval;
    
    // Peer management
    QStringList knownSeedNodes;
    QStringList discoveredPeers;
    QMap<QString, int> peerScores;
    
    // Network health metrics
    int networkHealthScore;
    int connectionCount;
    int syncProgress;
    int failedConnectionAttempts;
    int64_t lastSuccessfulConnection;
    QMap<QString, int64_t> peerLastSeen;
    QMap<QString, int> peerFailureCount;
    
    ClientModel* clientModel;
    
    // Built-in seed nodes for BabaChain network
    static const QStringList BUILTIN_SEED_NODES;
    static constexpr int DEFAULT_DISCOVERY_INTERVAL = 300000; // 5 minutes
    static constexpr int DEFAULT_HEALTH_CHECK_INTERVAL = 60000; // 1 minute
    static constexpr int DEFAULT_TARGET_CONNECTIONS = 8;
    static constexpr int DEFAULT_MAX_CONNECTIONS = 16;
};

#endif // BITCOIN_QT_AUTONODE_H