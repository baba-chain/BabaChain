// Copyright (c) 2024 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/stakingpage.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>

#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <util/system.h>

#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

StakingPage::StakingPage(QWidget* parent) :
    QWidget(parent),
    mainLayout(nullptr),
    stakingControlGroup(nullptr),
    stakingStatsGroup(nullptr),
    earningsGroup(nullptr),
    validatorsGroup(nullptr),
    autoStakingButton(nullptr),
    manualStakingButton(nullptr),
    stopStakingButton(nullptr),
    refreshButton(nullptr),
    stakingStatusLabel(nullptr),
    stakingProgress(nullptr),
    totalStakedLabel(nullptr),
    stakingRewardLabel(nullptr),
    networkStakeLabel(nullptr),
    stakingWeightLabel(nullptr),
    expectedTimeLabel(nullptr),
    stakingDifficultyLabel(nullptr),
    totalEarningsLabel(nullptr),
    dailyEarningsLabel(nullptr),
    weeklyEarningsLabel(nullptr),
    monthlyEarningsLabel(nullptr),
    yearlyProjectionLabel(nullptr),
    roiLabel(nullptr),
    validatorsTable(nullptr),
    clientModel(nullptr),
    walletModel(nullptr),
    updateTimer(nullptr),
    autoStakingTimer(nullptr),
    isAutoStakingEnabled(false),
    isCurrentlyStaking(false),
    m_privacy(false),
    m_display_unit(BitcoinUnit::BABACHAIN),
    totalEarnings(0),
    dailyEarnings(0),
    weeklyEarnings(0),
    monthlyEarnings(0),
    currentROI(0.0)
{
    setupUI();
    
    // Initialize timers
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &StakingPage::updateStakingInfo);
    updateTimer->start(STAKING_UPDATE_INTERVAL);
    
    autoStakingTimer = new QTimer(this);
    connect(autoStakingTimer, &QTimer::timeout, this, &StakingPage::startAutoStaking);
}

StakingPage::~StakingPage()
{
    if (updateTimer) {
        updateTimer->stop();
    }
    if (autoStakingTimer) {
        autoStakingTimer->stop();
    }
}

void StakingPage::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // Staking Control Group
    stakingControlGroup = new QGroupBox(tr("Staking Controls"));
    QVBoxLayout* controlLayout = new QVBoxLayout(stakingControlGroup);
    
    // Status display
    stakingStatusLabel = new QLabel(tr("Staking Status: Inactive"));
    stakingStatusLabel->setStyleSheet("QLabel { font-weight: bold; color: #ff6b6b; }");
    controlLayout->addWidget(stakingStatusLabel);
    
    stakingProgress = new QProgressBar();
    stakingProgress->setVisible(false);
    controlLayout->addWidget(stakingProgress);
    
    // Control buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    autoStakingButton = new QPushButton(tr("Enable Auto-Staking"));
    autoStakingButton->setToolTip(tr("Automatically start staking when coins mature (8+ hours old)"));
    connect(autoStakingButton, &QPushButton::clicked, this, &StakingPage::toggleAutoStaking);
    buttonLayout->addWidget(autoStakingButton);
    
    manualStakingButton = new QPushButton(tr("Start Staking"));
    manualStakingButton->setToolTip(tr("Manually start staking with current mature coins"));
    connect(manualStakingButton, &QPushButton::clicked, this, &StakingPage::startManualStaking);
    buttonLayout->addWidget(manualStakingButton);
    
    stopStakingButton = new QPushButton(tr("Stop Staking"));
    stopStakingButton->setEnabled(false);
    connect(stopStakingButton, &QPushButton::clicked, this, &StakingPage::stopStaking);
    buttonLayout->addWidget(stopStakingButton);
    
    refreshButton = new QPushButton(tr("Refresh"));
    connect(refreshButton, &QPushButton::clicked, this, &StakingPage::refreshValidators);
    buttonLayout->addWidget(refreshButton);
    
    controlLayout->addLayout(buttonLayout);
    mainLayout->addWidget(stakingControlGroup);

    // Staking Statistics Group
    stakingStatsGroup = new QGroupBox(tr("Staking Statistics"));
    QVBoxLayout* statsLayout = new QVBoxLayout(stakingStatsGroup);
    
    QHBoxLayout* statsRow1 = new QHBoxLayout();
    totalStakedLabel = new QLabel(tr("Total Staked: 0 BABACHAIN"));
    stakingRewardLabel = new QLabel(tr("Next Reward: ~0 BABACHAIN"));
    statsRow1->addWidget(totalStakedLabel);
    statsRow1->addWidget(stakingRewardLabel);
    statsLayout->addLayout(statsRow1);
    
    QHBoxLayout* statsRow2 = new QHBoxLayout();
    networkStakeLabel = new QLabel(tr("Network Stake: 0 BABACHAIN"));
    stakingWeightLabel = new QLabel(tr("Staking Weight: 0%"));
    statsRow2->addWidget(networkStakeLabel);
    statsRow2->addWidget(stakingWeightLabel);
    statsLayout->addLayout(statsRow2);
    
    QHBoxLayout* statsRow3 = new QHBoxLayout();
    expectedTimeLabel = new QLabel(tr("Expected Time: Unknown"));
    stakingDifficultyLabel = new QLabel(tr("Staking Difficulty: 0"));
    statsRow3->addWidget(expectedTimeLabel);
    statsRow3->addWidget(stakingDifficultyLabel);
    statsLayout->addLayout(statsRow3);
    
    mainLayout->addWidget(stakingStatsGroup);

    // Earnings Group
    earningsGroup = new QGroupBox(tr("Staking Earnings & Projections"));
    QVBoxLayout* earningsLayout = new QVBoxLayout(earningsGroup);
    
    QHBoxLayout* earningsRow1 = new QHBoxLayout();
    totalEarningsLabel = new QLabel(tr("Total Earnings: 0 BABACHAIN"));
    totalEarningsLabel->setStyleSheet("QLabel { font-weight: bold; color: #4ecdc4; }");
    roiLabel = new QLabel(tr("Current ROI: 0.00%"));
    roiLabel->setStyleSheet("QLabel { font-weight: bold; color: #45b7d1; }");
    earningsRow1->addWidget(totalEarningsLabel);
    earningsRow1->addWidget(roiLabel);
    earningsLayout->addLayout(earningsRow1);
    
    QHBoxLayout* earningsRow2 = new QHBoxLayout();
    dailyEarningsLabel = new QLabel(tr("Daily: 0 BABACHAIN"));
    weeklyEarningsLabel = new QLabel(tr("Weekly: 0 BABACHAIN"));
    earningsRow2->addWidget(dailyEarningsLabel);
    earningsRow2->addWidget(weeklyEarningsLabel);
    earningsLayout->addLayout(earningsRow2);
    
    QHBoxLayout* earningsRow3 = new QHBoxLayout();
    monthlyEarningsLabel = new QLabel(tr("Monthly: 0 BABACHAIN"));
    yearlyProjectionLabel = new QLabel(tr("Yearly Projection: 0 BABACHAIN"));
    yearlyProjectionLabel->setStyleSheet("QLabel { font-weight: bold; color: #f39c12; }");
    earningsRow3->addWidget(monthlyEarningsLabel);
    earningsRow3->addWidget(yearlyProjectionLabel);
    earningsLayout->addLayout(earningsRow3);
    
    mainLayout->addWidget(earningsGroup);

    // Validators Group
    validatorsGroup = new QGroupBox(tr("Active Validators"));
    QVBoxLayout* validatorsLayout = new QVBoxLayout(validatorsGroup);
    
    validatorsTable = new QTableWidget(0, 4);
    QStringList headers;
    headers << tr("Validator") << tr("Stake Amount") << tr("Status") << tr("Last Active");
    validatorsTable->setHorizontalHeaderLabels(headers);
    validatorsTable->horizontalHeader()->setStretchLastSection(true);
    validatorsTable->setAlternatingRowColors(true);
    validatorsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    validatorsTable->setMaximumHeight(200);
    
    validatorsLayout->addWidget(validatorsTable);
    mainLayout->addWidget(validatorsGroup);

    setLayout(mainLayout);
}

void StakingPage::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if (clientModel) {
        updateStakingInfo();
    }
}

void StakingPage::setWalletModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
    if (walletModel) {
        connect(walletModel, &WalletModel::balanceChanged, this, &StakingPage::updateBalance);
        updateStakingInfo();
    }
}

void StakingPage::updateBalance(const interfaces::WalletBalances& balances)
{
    m_balances = balances;
    updateStakingStatus();
    updateEarningsDisplay();
}

void StakingPage::setPrivacy(bool privacy)
{
    m_privacy = privacy;
    updateStakingInfo();
}

void StakingPage::updateStakingInfo()
{
    if (!clientModel || !walletModel) {
        return;
    }

    // Update staking status and statistics
    updateStakingStatus();
    updateEarningsDisplay();
    updateValidatorsList();
}

void StakingPage::updateStakingStatus()
{
    if (!clientModel || !walletModel) {
        return;
    }

    try {
        // Get staking info from RPC
        interfaces::Node& node = clientModel->node();
        UniValue stakingInfo;
        
        if (node.tryGetStakingInfo(stakingInfo)) {
            bool stakingEnabled = stakingInfo["enabled"].get_bool();
            CAmount totalStaked = stakingInfo["totalstaked"].get_int64();
            CAmount networkStake = stakingInfo["networkstake"].get_int64();
            double stakingWeight = stakingInfo["weight"].get_real();
            int64_t expectedTime = stakingInfo["expectedtime"].get_int64();
            double difficulty = stakingInfo["difficulty"].get_real();
            
            // Update status label and styling
            if (stakingEnabled && isCurrentlyStaking) {
                stakingStatusLabel->setText(tr("Staking Status: Active"));
                stakingStatusLabel->setStyleSheet("QLabel { font-weight: bold; color: #4ecdc4; }");
                stakingProgress->setVisible(true);
                stakingProgress->setRange(0, 0); // Indeterminate progress
            } else if (isAutoStakingEnabled) {
                stakingStatusLabel->setText(tr("Staking Status: Auto-Staking Enabled"));
                stakingStatusLabel->setStyleSheet("QLabel { font-weight: bold; color: #f39c12; }");
                stakingProgress->setVisible(false);
            } else {
                stakingStatusLabel->setText(tr("Staking Status: Inactive"));
                stakingStatusLabel->setStyleSheet("QLabel { font-weight: bold; color: #ff6b6b; }");
                stakingProgress->setVisible(false);
            }
            
            // Update statistics
            QString totalStakedStr = m_privacy ? tr("Hidden") : 
                BitcoinUnits::formatWithUnit(m_display_unit, totalStaked);
            totalStakedLabel->setText(tr("Total Staked: %1").arg(totalStakedStr));
            
            QString networkStakeStr = BitcoinUnits::formatWithUnit(m_display_unit, networkStake);
            networkStakeLabel->setText(tr("Network Stake: %1").arg(networkStakeStr));
            
            stakingWeightLabel->setText(tr("Staking Weight: %1%").arg(QString::number(stakingWeight, 'f', 2)));
            
            if (expectedTime > 0) {
                QString timeStr = GUIUtil::formatDurationStr(expectedTime);
                expectedTimeLabel->setText(tr("Expected Time: %1").arg(timeStr));
            } else {
                expectedTimeLabel->setText(tr("Expected Time: Unknown"));
            }
            
            stakingDifficultyLabel->setText(tr("Staking Difficulty: %1").arg(QString::number(difficulty, 'f', 4)));
            
            // Calculate next reward estimate
            CAmount nextReward = totalStaked * 0.001; // Rough estimate: 0.1% of stake
            QString nextRewardStr = m_privacy ? tr("Hidden") : 
                BitcoinUnits::formatWithUnit(m_display_unit, nextReward);
            stakingRewardLabel->setText(tr("Next Reward: ~%1").arg(nextRewardStr));
            
            // Update button states
            bool hasEligibleCoins = checkStakingEligibility();
            manualStakingButton->setEnabled(!isCurrentlyStaking && hasEligibleCoins);
            stopStakingButton->setEnabled(isCurrentlyStaking);
            
            isCurrentlyStaking = stakingEnabled;
        }
    } catch (const std::exception& e) {
        // Handle RPC errors gracefully
        stakingStatusLabel->setText(tr("Staking Status: Error"));
        stakingStatusLabel->setStyleSheet("QLabel { font-weight: bold; color: #ff6b6b; }");
    }
}

void StakingPage::updateEarningsDisplay()
{
    if (!walletModel || m_privacy) {
        totalEarningsLabel->setText(tr("Total Earnings: Hidden"));
        dailyEarningsLabel->setText(tr("Daily: Hidden"));
        weeklyEarningsLabel->setText(tr("Weekly: Hidden"));
        monthlyEarningsLabel->setText(tr("Monthly: Hidden"));
        yearlyProjectionLabel->setText(tr("Yearly Projection: Hidden"));
        roiLabel->setText(tr("Current ROI: Hidden"));
        return;
    }

    // Calculate earnings based on balance changes and staking rewards
    // This is a simplified calculation - in a real implementation, 
    // you'd track actual staking rewards from transaction history
    
    CAmount currentBalance = m_balances.balance;
    if (currentBalance > 0) {
        // Estimate daily earnings at 1% annual rate (365% / 365 days)
        dailyEarnings = currentBalance / 365;
        weeklyEarnings = dailyEarnings * 7;
        monthlyEarnings = dailyEarnings * 30;
        CAmount yearlyProjection = dailyEarnings * 365;
        
        // Calculate ROI (simplified)
        currentROI = 365.0; // 365% annual ROI as per BabaChain design
        
        // Update labels
        totalEarningsLabel->setText(tr("Total Earnings: %1")
            .arg(BitcoinUnits::formatWithUnit(m_display_unit, totalEarnings)));
        
        dailyEarningsLabel->setText(tr("Daily: %1")
            .arg(BitcoinUnits::formatWithUnit(m_display_unit, dailyEarnings)));
        
        weeklyEarningsLabel->setText(tr("Weekly: %1")
            .arg(BitcoinUnits::formatWithUnit(m_display_unit, weeklyEarnings)));
        
        monthlyEarningsLabel->setText(tr("Monthly: %1")
            .arg(BitcoinUnits::formatWithUnit(m_display_unit, monthlyEarnings)));
        
        yearlyProjectionLabel->setText(tr("Yearly Projection: %1")
            .arg(BitcoinUnits::formatWithUnit(m_display_unit, yearlyProjection)));
        
        roiLabel->setText(tr("Current ROI: %1%").arg(QString::number(currentROI, 'f', 2)));
    }
}

void StakingPage::updateValidatorsList()
{
    if (!clientModel) {
        return;
    }

    try {
        // Get validators list from RPC
        interfaces::Node& node = clientModel->node();
        UniValue validators;
        
        if (node.tryGetValidatorsList(validators)) {
            validatorsTable->setRowCount(0);
            
            for (size_t i = 0; i < validators.size(); ++i) {
                const UniValue& validator = validators[i];
                
                validatorsTable->insertRow(i);
                
                QString pubkey = QString::fromStdString(validator["pubkey"].get_str()).left(16) + "...";
                CAmount stakeAmount = validator["stake"].get_int64();
                QString status = QString::fromStdString(validator["status"].get_str());
                int64_t lastActive = validator["lastactive"].get_int64();
                
                validatorsTable->setItem(i, 0, new QTableWidgetItem(pubkey));
                validatorsTable->setItem(i, 1, new QTableWidgetItem(
                    BitcoinUnits::formatWithUnit(m_display_unit, stakeAmount)));
                validatorsTable->setItem(i, 2, new QTableWidgetItem(status));
                validatorsTable->setItem(i, 3, new QTableWidgetItem(
                    QDateTime::fromSecsSinceEpoch(lastActive).toString()));
            }
        }
    } catch (const std::exception& e) {
        // Handle RPC errors gracefully
    }
}

void StakingPage::toggleAutoStaking()
{
    isAutoStakingEnabled = !isAutoStakingEnabled;
    
    if (isAutoStakingEnabled) {
        autoStakingButton->setText(tr("Disable Auto-Staking"));
        autoStakingTimer->start(AUTO_STAKING_CHECK_INTERVAL);
        showStakingSuccess(tr("Auto-staking enabled. Will automatically start staking when coins mature."));
    } else {
        autoStakingButton->setText(tr("Enable Auto-Staking"));
        autoStakingTimer->stop();
        showStakingSuccess(tr("Auto-staking disabled."));
    }
    
    updateStakingStatus();
    Q_EMIT stakingStatusChanged(isAutoStakingEnabled);
}

void StakingPage::startManualStaking()
{
    if (!walletModel || !clientModel) {
        showStakingError(tr("Wallet not available"));
        return;
    }

    if (!checkStakingEligibility()) {
        showStakingError(tr("No eligible coins for staking. Coins must be at least 8 hours old and minimum 1000 BABACHAIN."));
        return;
    }

    try {
        // Start staking via RPC
        interfaces::Node& node = clientModel->node();
        CAmount stakeAmount = m_balances.balance; // Stake all available balance
        
        if (node.tryStartStaking(stakeAmount)) {
            isCurrentlyStaking = true;
            showStakingSuccess(tr("Staking started successfully!"));
            updateStakingStatus();
        } else {
            showStakingError(tr("Failed to start staking. Please check your wallet and try again."));
        }
    } catch (const std::exception& e) {
        showStakingError(tr("Error starting staking: %1").arg(QString::fromStdString(e.what())));
    }
}

void StakingPage::stopStaking()
{
    if (!clientModel) {
        showStakingError(tr("Client not available"));
        return;
    }

    try {
        // Stop staking via RPC
        interfaces::Node& node = clientModel->node();
        
        if (node.tryStopStaking()) {
            isCurrentlyStaking = false;
            showStakingSuccess(tr("Staking stopped successfully."));
            updateStakingStatus();
        } else {
            showStakingError(tr("Failed to stop staking."));
        }
    } catch (const std::exception& e) {
        showStakingError(tr("Error stopping staking: %1").arg(QString::fromStdString(e.what())));
    }
}

void StakingPage::refreshValidators()
{
    updateValidatorsList();
    showStakingSuccess(tr("Validators list refreshed."));
}

void StakingPage::startAutoStaking()
{
    if (!isAutoStakingEnabled || isCurrentlyStaking) {
        return;
    }

    if (checkStakingEligibility()) {
        startManualStaking();
    }
}

bool StakingPage::checkStakingEligibility()
{
    if (!walletModel) {
        return false;
    }

    // Check if we have enough mature coins
    CAmount matureBalance = m_balances.balance; // Simplified - should check coin age
    return matureBalance >= MIN_STAKING_AMOUNT;
}

void StakingPage::showStakingError(const QString& error)
{
    QMessageBox::warning(this, tr("Staking Error"), error);
}

void StakingPage::showStakingSuccess(const QString& message)
{
    // Could use a status bar or notification instead of message box
    // For now, just update the status label briefly
    QString originalText = stakingStatusLabel->text();
    QString originalStyle = stakingStatusLabel->styleSheet();
    
    stakingStatusLabel->setText(message);
    stakingStatusLabel->setStyleSheet("QLabel { font-weight: bold; color: #4ecdc4; }");
    
    QTimer::singleShot(3000, [this, originalText, originalStyle]() {
        stakingStatusLabel->setText(originalText);
        stakingStatusLabel->setStyleSheet(originalStyle);
    });
}

void StakingPage::onStakingReward(const QString& amount, const QString& txid)
{
    // Handle staking reward notification
    totalEarnings += BitcoinUnits::parse(m_display_unit, amount);
    updateEarningsDisplay();
    
    Q_EMIT stakingRewardReceived(amount, txid);
    
    showStakingSuccess(tr("Staking reward received: %1").arg(amount));
}