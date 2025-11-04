// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/autobootstrap.h>

#include <qt/clientmodel.h>
#include <interfaces/node.h>
#include <util/system.h>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

// Built-in bootstrap sources for BabaChain network
const QStringList AutoBootstrapManager::BOOTSTRAP_SOURCES = {
    "https://bootstrap.babachain.org/latest/blockchain.tar.gz",
    "https://cdn.babachain.network/bootstrap/blockchain.tar.gz",
    "https://mirror1.babachain.io/bootstrap/blockchain.tar.gz",
    "https://mirror2.babachain.io/bootstrap/blockchain.tar.gz",
    "https://files.babachain.org/bootstrap/blockchain.tar.gz"
};

AutoBootstrapManager::AutoBootstrapManager(QObject* parent)
    : QObject(parent),
      networkManager(nullptr),
      currentDownload(nullptr),
      statusTimer(nullptr),
      isBootstrapActive(false),
      autoBootstrapEnabled(true),
      bootstrapProgress(0),
      clientModel(nullptr)
{
    networkManager = new QNetworkAccessManager(this);
    
    statusTimer = new QTimer(this);
    statusTimer->setSingleShot(false);
    connect(statusTimer, &QTimer::timeout, this, &AutoBootstrapManager::checkBootstrapStatus);
    
    // Initialize bootstrap sources
    bootstrapSources = BOOTSTRAP_SOURCES;
    
    // Initialize source reliability scores
    for (const QString& source : bootstrapSources) {
        sourceReliability[source] = 100; // Start with full reliability
    }
}

AutoBootstrapManager::~AutoBootstrapManager()
{
    stopBootstrap();
    cleanupBootstrapFiles();
}

void AutoBootstrapManager::setClientModel(ClientModel* clientModel)
{
    this->clientModel = clientModel;
}

void AutoBootstrapManager::startBootstrap()
{
    if (!clientModel || !autoBootstrapEnabled || isBootstrapActive) {
        return;
    }
    
    // Check if bootstrap is needed
    if (!clientModel->node().isInitialBlockDownload()) {
        return; // Already synced
    }
    
    // Check available disk space
    QString dataDir = QString::fromStdString(gArgs.GetDataDirNet().string());
    QDir dir(dataDir);
    if (!dir.exists()) {
        Q_EMIT bootstrapError(tr("Data directory does not exist"));
        return;
    }
    
    // Start bootstrap process
    isBootstrapActive = true;
    bootstrapProgress = 0;
    
    Q_EMIT bootstrapStarted();
    
    // Select best bootstrap source
    selectBestBootstrapSource();
    
    // Start status monitoring
    statusTimer->start(BOOTSTRAP_CHECK_INTERVAL);
    
    // Begin download
    downloadBootstrapData();
}

void AutoBootstrapManager::stopBootstrap()
{
    if (statusTimer) {
        statusTimer->stop();
    }
    
    if (currentDownload) {
        currentDownload->abort();
        currentDownload->deleteLater();
        currentDownload = nullptr;
    }
    
    isBootstrapActive = false;
    bootstrapProgress = 0;
}

void AutoBootstrapManager::checkBootstrapStatus()
{
    if (!clientModel || !isBootstrapActive) {
        return;
    }
    
    // Check if node is still in IBD
    if (!clientModel->node().isInitialBlockDownload()) {
        // Bootstrap completed successfully
        isBootstrapActive = false;
        statusTimer->stop();
        cleanupBootstrapFiles();
        Q_EMIT bootstrapFinished(true);
        return;
    }
    
    // Update progress based on sync status
    double syncProgress = clientModel->getVerificationProgress();
    bootstrapProgress = static_cast<int>(syncProgress * 100);
    Q_EMIT bootstrapProgress(bootstrapProgress);
}

void AutoBootstrapManager::downloadBootstrapData()
{
    if (currentBootstrapSource.isEmpty()) {
        Q_EMIT bootstrapError(tr("No bootstrap source available"));
        return;
    }
    
    // Prepare download path
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    bootstrapFilePath = tempDir + "/babachain_bootstrap.tar.gz";
    
    // Start download
    QNetworkRequest request(QUrl(currentBootstrapSource));
    request.setRawHeader("User-Agent", "BabaChain-Qt/1.0");
    
    currentDownload = networkManager->get(request);
    
    connect(currentDownload, &QNetworkReply::downloadProgress,
            this, &AutoBootstrapManager::onBootstrapDownloadProgress);
    connect(currentDownload, &QNetworkReply::finished,
            this, &AutoBootstrapManager::onBootstrapDownloadFinished);
}

void AutoBootstrapManager::verifyBootstrapData()
{
    if (bootstrapFilePath.isEmpty() || !QFile::exists(bootstrapFilePath)) {
        Q_EMIT bootstrapError(tr("Bootstrap file not found"));
        return;
    }
    
    // Verify file integrity
    if (!verifyBootstrapIntegrity(bootstrapFilePath)) {
        Q_EMIT bootstrapError(tr("Bootstrap file verification failed"));
        return;
    }
    
    // Apply bootstrap data
    applyBootstrapData(bootstrapFilePath);
}

void AutoBootstrapManager::onBootstrapDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int progress = static_cast<int>((bytesReceived * 50) / bytesTotal); // 50% for download
        bootstrapProgress = progress;
        Q_EMIT bootstrapProgress(bootstrapProgress);
    }
}

void AutoBootstrapManager::onBootstrapDownloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() == QNetworkReply::NoError) {
        // Save downloaded data
        QFile file(bootstrapFilePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            
            // Update source reliability (successful download)
            sourceReliability[currentBootstrapSource] = qMin(100, 
                sourceReliability[currentBootstrapSource] + 10);
            
            // Proceed to verification
            verifyBootstrapData();
        } else {
            Q_EMIT bootstrapError(tr("Failed to save bootstrap file"));
        }
    } else {
        // Download failed, update source reliability
        sourceReliability[currentBootstrapSource] = qMax(0, 
            sourceReliability[currentBootstrapSource] - 20);
        
        // Try next source
        selectBestBootstrapSource();
        if (!currentBootstrapSource.isEmpty()) {
            downloadBootstrapData();
        } else {
            Q_EMIT bootstrapError(tr("All bootstrap sources failed"));
        }
    }
    
    reply->deleteLater();
    currentDownload = nullptr;
}

void AutoBootstrapManager::onBootstrapVerificationFinished()
{
    // This would be called after verification is complete
    bootstrapProgress = 100;
    Q_EMIT bootstrapProgress(bootstrapProgress);
    Q_EMIT bootstrapFinished(true);
}

void AutoBootstrapManager::selectBestBootstrapSource()
{
    currentBootstrapSource.clear();
    int bestReliability = -1;
    
    // Select source with highest reliability score
    for (auto it = sourceReliability.begin(); it != sourceReliability.end(); ++it) {
        if (it.value() > bestReliability) {
            bestReliability = it.value();
            currentBootstrapSource = it.key();
        }
    }
    
    // If no reliable source found, reset all scores and try again
    if (bestReliability < 10) {
        for (auto it = sourceReliability.begin(); it != sourceReliability.end(); ++it) {
            it.value() = 50; // Reset to moderate reliability
        }
        selectBestBootstrapSource();
    }
}

void AutoBootstrapManager::downloadFromSource(const QString& source)
{
    currentBootstrapSource = source;
    downloadBootstrapData();
}

bool AutoBootstrapManager::verifyBootstrapIntegrity(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    // Check file size
    qint64 fileSize = file.size();
    if (fileSize > MAX_BOOTSTRAP_SIZE || fileSize < 1024) {
        return false;
    }
    
    // Basic integrity check - in a real implementation, you would:
    // 1. Verify cryptographic signature
    // 2. Check SHA256 hash against known good hash
    // 3. Verify the archive structure
    
    // For now, just check if it's a valid archive header
    QByteArray header = file.read(10);
    file.close();
    
    // Check for gzip magic number
    if (header.size() >= 2 && 
        static_cast<unsigned char>(header[0]) == 0x1f && 
        static_cast<unsigned char>(header[1]) == 0x8b) {
        return true;
    }
    
    return false;
}

void AutoBootstrapManager::applyBootstrapData(const QString& filePath)
{
    // In a real implementation, this would:
    // 1. Stop the node safely
    // 2. Extract the bootstrap archive to the blockchain data directory
    // 3. Restart the node
    // 4. Monitor the sync process
    
    // For now, we'll just simulate the process
    bootstrapProgress = 75;
    Q_EMIT bootstrapProgress(bootstrapProgress);
    
    // Simulate processing time
    QTimer::singleShot(5000, this, &AutoBootstrapManager::onBootstrapVerificationFinished);
}

void AutoBootstrapManager::cleanupBootstrapFiles()
{
    if (!bootstrapFilePath.isEmpty() && QFile::exists(bootstrapFilePath)) {
        QFile::remove(bootstrapFilePath);
        bootstrapFilePath.clear();
    }
}