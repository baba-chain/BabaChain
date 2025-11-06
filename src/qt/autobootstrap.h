// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_AUTOBOOTSTRAP_H
#define BITCOIN_QT_AUTOBOOTSTRAP_H

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressBar>
#include <QLabel>
#include <memory>

class ClientModel;
enum class SynchronizationState;

/** Automatic blockchain bootstrap system */
class AutoBootstrapManager : public QObject
{
    Q_OBJECT

public:
    explicit AutoBootstrapManager(QObject* parent = nullptr);
    ~AutoBootstrapManager();

    void setClientModel(ClientModel* clientModel);
    void startBootstrap();
    void stopBootstrap();
    
    bool isBootstrapping() const { return isBootstrapActive; }
    int getBootstrapProgress() const { return m_bootstrapProgress; }

public Q_SLOTS:
    void checkBootstrapStatus();
    void downloadBootstrapData();
    void verifyBootstrapData();

private Q_SLOTS:
    void onBootstrapDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onBootstrapDownloadFinished();
    void onBootstrapVerificationFinished();

Q_SIGNALS:
    void bootstrapStarted();
    void bootstrapProgress(int percentage);
    void bootstrapFinished(bool success);
    void bootstrapError(const QString& error);

private:
    void selectBestBootstrapSource();
    void downloadFromSource(const QString& source);
    bool verifyBootstrapIntegrity(const QString& filePath);
    void applyBootstrapData(const QString& filePath);
    void cleanupBootstrapFiles();
    
    // Network and download management
    QNetworkAccessManager* networkManager;
    QNetworkReply* currentDownload;
    QTimer* statusTimer;
    
    // Bootstrap configuration
    bool isBootstrapActive;
    bool autoBootstrapEnabled;
    int m_bootstrapProgress;  // Renamed to avoid conflict with signal
    QString currentBootstrapSource;
    QString bootstrapFilePath;
    
    // Bootstrap sources (multiple mirrors for redundancy)
    QStringList bootstrapSources;
    QMap<QString, int> sourceReliability;
    
    ClientModel* clientModel;
    
    // Built-in bootstrap sources for BabaChain
    static const QStringList BOOTSTRAP_SOURCES;
    static constexpr int BOOTSTRAP_CHECK_INTERVAL = 30000; // 30 seconds
    static constexpr qint64 MAX_BOOTSTRAP_SIZE = 10LL * 1024 * 1024 * 1024; // 10GB
};

#endif // BITCOIN_QT_AUTOBOOTSTRAP_H