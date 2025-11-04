// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_STAKINGPAGE_H
#define BITCOIN_QT_STAKINGPAGE_H

#include <interfaces/wallet.h>
#include <qt/bitcoinunits.h>

#include <QWidget>
#include <QTimer>
#include <memory>

class ClientModel;
class WalletModel;
class QLabel;
class QPushButton;
class QProgressBar;
class QTableWidget;
class QVBoxLayout;
class QHBoxLayout;
class QGroupBox;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Staking page widget with auto-staking functionality */
class StakingPage : public QWidget
{
    Q_OBJECT

public:
    explicit StakingPage(QWidget* parent = nullptr);
    ~StakingPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);

public Q_SLOTS:
    void updateStakingInfo();
    void updateBalance(const interfaces::WalletBalances& balances);
    void setPrivacy(bool privacy);

private Q_SLOTS:
    void toggleAutoStaking();
    void startManualStaking();
    void stopStaking();
    void refreshValidators();
    void onStakingReward(const QString& amount, const QString& txid);

Q_SIGNALS:
    void stakingStatusChanged(bool enabled);
    void stakingRewardReceived(const QString& amount, const QString& txid);

private:
    void setupUI();
    void updateStakingStatus();
    void updateEarningsDisplay();
    void updateValidatorsList();
    void startAutoStaking();
    bool checkStakingEligibility();
    void showStakingError(const QString& error);
    void showStakingSuccess(const QString& message);

    // UI Components
    QVBoxLayout* mainLayout;
    QGroupBox* stakingControlGroup;
    QGroupBox* stakingStatsGroup;
    QGroupBox* earningsGroup;
    QGroupBox* validatorsGroup;

    // Staking Controls
    QPushButton* autoStakingButton;
    QPushButton* manualStakingButton;
    QPushButton* stopStakingButton;
    QPushButton* refreshButton;
    QLabel* stakingStatusLabel;
    QProgressBar* stakingProgress;

    // Staking Statistics
    QLabel* totalStakedLabel;
    QLabel* stakingRewardLabel;
    QLabel* networkStakeLabel;
    QLabel* stakingWeightLabel;
    QLabel* expectedTimeLabel;
    QLabel* stakingDifficultyLabel;

    // Earnings Display
    QLabel* totalEarningsLabel;
    QLabel* dailyEarningsLabel;
    QLabel* weeklyEarningsLabel;
    QLabel* monthlyEarningsLabel;
    QLabel* yearlyProjectionLabel;
    QLabel* roiLabel;

    // Validators List
    QTableWidget* validatorsTable;

    // Models and Data
    ClientModel* clientModel;
    WalletModel* walletModel;
    QTimer* updateTimer;
    QTimer* autoStakingTimer;

    // Staking State
    bool isAutoStakingEnabled;
    bool isCurrentlyStaking;
    bool m_privacy;
    interfaces::WalletBalances m_balances;
    BitcoinUnit m_display_unit;

    // Earnings Tracking
    CAmount totalEarnings;
    CAmount dailyEarnings;
    CAmount weeklyEarnings;
    CAmount monthlyEarnings;
    double currentROI;

    // Auto-staking Configuration
    static constexpr int AUTO_STAKING_CHECK_INTERVAL = 60000; // 1 minute
    static constexpr int STAKING_UPDATE_INTERVAL = 30000;     // 30 seconds
    static constexpr CAmount MIN_STAKING_AMOUNT = 1000 * COIN; // 1000 BabaChain minimum
    static constexpr int64_t COIN_MATURITY_TIME = 8 * 60 * 60; // 8 hours
};

#endif // BITCOIN_QT_STAKINGPAGE_H