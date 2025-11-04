// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/stakingsetupwizard.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>

#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <util/system.h>
#include <consensus/amount.h>

#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

// Staking reward rate (365% APY as mentioned in the tasks)
static const double STAKING_APY = 3.65; // 365% APY

StakingSetupWizard::StakingSetupWizard(WalletModel* walletModel, QWidget* parent) :
    QWizard(parent),
    walletModel(walletModel),
    selectedStakeAmount(0),
    autoStakingEnabled(true),
    notificationsEnabled(true),
    emailNotifications(false),
    pushNotifications(true),
    notificationEmail("")
{
    setWindowTitle(tr("BabaChain Staking Setup Wizard"));
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::HaveHelpButton, false);
    setOption(QWizard::NoBackButtonOnStartPage, true);
    setOption(QWizard::NoBackButtonOnLastPage, true);
    
    // Add wizard pages
    setPage(Page_Welcome, createWelcomePage());
    setPage(Page_StakeAmount, createStakeAmountPage());
    setPage(Page_AutoStaking, createAutoStakingPage());
    setPage(Page_Notifications, createNotificationsPage());
    setPage(Page_Complete, createCompletePage());
    
    setStartId(Page_Welcome);
    
    // Connect signals
    connect(this, &QWizard::finished, this, &StakingSetupWizard::completeSetup);
    
    resize(600, 500);
}

StakingSetupWizard::~StakingSetupWizard()
{
}QWiz
ardPage* StakingSetupWizard::createWelcomePage()
{
    return new WelcomePage(walletModel, this);
}

QWizardPage* StakingSetupWizard::createStakeAmountPage()
{
    return new StakeAmountPage(walletModel, this);
}

QWizardPage* StakingSetupWizard::createAutoStakingPage()
{
    return new AutoStakingPage(this);
}

QWizardPage* StakingSetupWizard::createNotificationsPage()
{
    return new NotificationsPage(this);
}

QWizardPage* StakingSetupWizard::createCompletePage()
{
    return new CompletePage(this);
}

void StakingSetupWizard::onStakeAmountChanged()
{
    StakeAmountPage* page = qobject_cast<StakeAmountPage*>(currentPage());
    if (page) {
        selectedStakeAmount = page->getStakeAmount();
        page->updateEarningsProjection();
    }
}

void StakingSetupWizard::onAutoStakingToggled(bool enabled)
{
    autoStakingEnabled = enabled;
}

void StakingSetupWizard::onNotificationSettingsChanged()
{
    NotificationsPage* page = qobject_cast<NotificationsPage*>(currentPage());
    if (page) {
        notificationsEnabled = page->areNotificationsEnabled();
        emailNotifications = page->areEmailNotificationsEnabled();
        pushNotifications = page->arePushNotificationsEnabled();
        notificationEmail = page->getNotificationEmail();
    }
}

void StakingSetupWizard::completeSetup()
{
    if (result() == QDialog::Accepted) {
        applyStakingSettings();
    }
}

void StakingSetupWizard::updateEarningsProjection()
{
    calculateEarningsProjection(selectedStakeAmount);
}

void StakingSetupWizard::calculateEarningsProjection(qint64 stakeAmount)
{
    // Calculate earnings based on 365% APY
    double dailyRate = STAKING_APY / 365.0;
    double weeklyRate = dailyRate * 7.0;
    double monthlyRate = dailyRate * 30.0;
    double yearlyRate = STAKING_APY;
    
    // Update earnings labels if on stake amount page
    StakeAmountPage* page = qobject_cast<StakeAmountPage*>(currentPage());
    if (page) {
        page->updateEarningsProjection();
    }
}

bool StakingSetupWizard::validateStakeAmount()
{
    if (!walletModel) return false;
    
    interfaces::WalletBalances balances = walletModel->wallet().getBalances();
    CAmount availableBalance = balances.balance;
    
    return selectedStakeAmount > 0 && selectedStakeAmount <= availableBalance;
}

void StakingSetupWizard::applyStakingSettings()
{
    if (!walletModel) return;
    
    try {
        // Enable staking in the wallet
        walletModel->wallet().setStakingEnabled(autoStakingEnabled);
        
        // Store notification preferences (would be implemented in settings)
        // This would typically be saved to QSettings or wallet database
        
        QMessageBox::information(this, tr("Setup Complete"), 
            tr("Staking has been successfully configured!\n\n"
               "Your coins will start earning rewards once they mature (after 100 confirmations).\n"
               "Expected daily earnings: %1 BABACHAIN").arg(
                   BitcoinUnits::formatWithUnit(BitcoinUnit::BABACHAIN, 
                       selectedStakeAmount * STAKING_APY / 365.0 / 100)));
                       
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Setup Error"), 
            tr("Failed to configure staking: %1").arg(QString::fromStdString(e.what())));
    }
}// Welco
mePage implementation
WelcomePage::WelcomePage(WalletModel* walletModel, QWidget* parent) :
    QWizardPage(parent),
    walletModel(walletModel)
{
    setTitle(tr("Welcome to BabaChain Staking"));
    setSubTitle(tr("Earn up to 365% APY by staking your BABACHAIN coins"));
    
    QVBoxLayout* layout = new QVBoxLayout;
    
    welcomeLabel = new QLabel(tr(
        "<h3>Start Earning Passive Income Today!</h3>"
        "<p>BabaChain's Proof-of-Stake system allows you to earn rewards "
        "simply by holding and staking your coins. This wizard will help you "
        "set up automatic staking in just a few simple steps.</p>"
    ));
    welcomeLabel->setWordWrap(true);
    layout->addWidget(welcomeLabel);
    
    benefitsLabel = new QLabel(tr(
        "<h4>Benefits of Staking:</h4>"
        "<ul>"
        "<li>🎯 <b>Up to 365% Annual Percentage Yield (APY)</b></li>"
        "<li>💰 <b>Daily rewards</b> - earn coins every day</li>"
        "<li>🔒 <b>Secure network</b> - help secure the BabaChain network</li>"
        "<li>⚡ <b>Automatic</b> - set it and forget it</li>"
        "<li>🌱 <b>Compound growth</b> - reinvest rewards automatically</li>"
        "</ul>"
    ));
    benefitsLabel->setWordWrap(true);
    layout->addWidget(benefitsLabel);
    
    // Show current balance
    if (walletModel) {
        interfaces::WalletBalances balances = walletModel->wallet().getBalances();
        balanceLabel = new QLabel(tr(
            "<p><b>Your current balance:</b> %1</p>"
        ).arg(BitcoinUnits::formatWithUnit(BitcoinUnit::BABACHAIN, balances.balance)));
        balanceLabel->setStyleSheet("QLabel { background-color: #f0f8ff; padding: 10px; border-radius: 5px; }");
        layout->addWidget(balanceLabel);
    }
    
    layout->addStretch();
    setLayout(layout);
}