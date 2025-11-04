// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_STAKINGSETUPWIZARD_H
#define BITCOIN_QT_STAKINGSETUPWIZARD_H

#include <interfaces/wallet.h>
#include <qt/bitcoinunits.h>

#include <QWizard>
#include <QWizardPage>
#include <QTimer>
#include <memory>

class ClientModel;
class WalletModel;
class QLabel;
class QPushButton;
class QProgressBar;
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QTextEdit;
class QVBoxLayout;
class QHBoxLayout;
class QGroupBox;
class QSlider;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** One-click staking setup wizard */
class StakingSetupWizard : public QWizard
{
    Q_OBJECT

public:
    explicit StakingSetupWizard(WalletModel* walletModel, QWidget* parent = nullptr);
    ~StakingSetupWizard();

    enum PageId {
        Page_Welcome,
        Page_StakeAmount,
        Page_AutoStaking,
        Page_Notifications,
        Page_Complete
    };

private Q_SLOTS:
    void onStakeAmountChanged();
    void onAutoStakingToggled(bool enabled);
    void onNotificationSettingsChanged();
    void completeSetup();
    void updateEarningsProjection();

private:
    WalletModel* walletModel;
    
    // Wizard pages
    QWizardPage* createWelcomePage();
    QWizardPage* createStakeAmountPage();
    QWizardPage* createAutoStakingPage();
    QWizardPage* createNotificationsPage();
    QWizardPage* createCompletePage();
    
    // Helper functions
    void calculateEarningsProjection(qint64 stakeAmount);
    bool validateStakeAmount();
    void applyStakingSettings();
    
    // Settings storage
    qint64 selectedStakeAmount;
    bool autoStakingEnabled;
    bool notificationsEnabled;
    bool emailNotifications;
    bool pushNotifications;
    QString notificationEmail;
};

/** Welcome page for staking setup */
class WelcomePage : public QWizardPage
{
    Q_OBJECT

public:
    WelcomePage(WalletModel* walletModel, QWidget* parent = nullptr);

private:
    WalletModel* walletModel;
    QLabel* welcomeLabel;
    QLabel* benefitsLabel;
    QLabel* balanceLabel;
};

/** Stake amount selection page */
class StakeAmountPage : public QWizardPage
{
    Q_OBJECT

public:
    StakeAmountPage(WalletModel* walletModel, QWidget* parent = nullptr);
    
    qint64 getStakeAmount() const;
    void updateEarningsProjection();

private Q_SLOTS:
    void onAmountChanged();
    void onSliderChanged(int value);
    void onPresetClicked();

private:
    WalletModel* walletModel;
    
    // Amount selection controls
    QDoubleSpinBox* amountSpinBox;
    QSlider* amountSlider;
    QPushButton* preset25Button;
    QPushButton* preset50Button;
    QPushButton* preset75Button;
    QPushButton* preset100Button;
    
    // Earnings projection display
    QLabel* dailyEarningsLabel;
    QLabel* weeklyEarningsLabel;
    QLabel* monthlyEarningsLabel;
    QLabel* yearlyEarningsLabel;
    QLabel* roiLabel;
    
    // Balance info
    QLabel* availableBalanceLabel;
    QLabel* afterStakeBalanceLabel;
    
    qint64 availableBalance;
    qint64 currentStakeAmount;
};

/** Auto-staking configuration page */
class AutoStakingPage : public QWizardPage
{
    Q_OBJECT

public:
    AutoStakingPage(QWidget* parent = nullptr);
    
    bool isAutoStakingEnabled() const;
    int getMaturityThreshold() const;
    bool isCompoundingEnabled() const;

private:
    QCheckBox* enableAutoStakingBox;
    QSpinBox* maturityThresholdSpinBox;
    QCheckBox* enableCompoundingBox;
    QLabel* explanationLabel;
};

/** Notifications configuration page */
class NotificationsPage : public QWizardPage
{
    Q_OBJECT

public:
    NotificationsPage(QWidget* parent = nullptr);
    
    bool areNotificationsEnabled() const;
    bool areEmailNotificationsEnabled() const;
    bool arePushNotificationsEnabled() const;
    QString getNotificationEmail() const;

private Q_SLOTS:
    void onNotificationsToggled(bool enabled);

private:
    QCheckBox* enableNotificationsBox;
    QCheckBox* emailNotificationsBox;
    QCheckBox* pushNotificationsBox;
    QLineEdit* emailLineEdit;
    QLabel* emailLabel;
};

/** Setup completion page */
class CompletePage : public QWizardPage
{
    Q_OBJECT

public:
    CompletePage(QWidget* parent = nullptr);
    
    void updateSummary(qint64 stakeAmount, bool autoStaking, bool notifications);

private:
    QLabel* summaryLabel;
    QLabel* nextStepsLabel;
    QPushButton* startStakingButton;
};

#endif // BITCOIN_QT_STAKINGSETUPWIZARD_H