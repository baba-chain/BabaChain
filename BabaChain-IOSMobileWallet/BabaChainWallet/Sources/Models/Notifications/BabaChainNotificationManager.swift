//
//  BabaChainNotificationManager.swift
//  BabaChainWallet
//
//  Created by BabaChain Core Group
//  Copyright © 2024 BabaChain Core Group. All rights reserved.
//
//  Licensed under the MIT License (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  https://opensource.org/licenses/MIT
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

import Foundation
import UserNotifications
import WidgetKit

/// Manages iOS-specific notifications and widget updates for BabaChain
@objc class BabaChainNotificationManager: NSObject {
    
    // MARK: - Properties
    
    @objc static let shared = BabaChainNotificationManager()
    
    private let notificationCenter = UNUserNotificationCenter.current()
    
    // Notification categories
    private let stakingCategory = "STAKING_CATEGORY"
    private let transactionCategory = "TRANSACTION_CATEGORY"
    private let networkCategory = "NETWORK_CATEGORY"
    
    // MARK: - Initialization
    
    private override init() {
        super.init()
        setupNotificationCategories()
        notificationCenter.delegate = self
    }
    
    // MARK: - Public Methods
    
    @objc func requestNotificationPermissions() {
        notificationCenter.requestAuthorization(options: [.alert, .badge, .sound, .provisional]) { granted, error in
            DispatchQueue.main.async {
                if granted {
                    print("Notification permissions granted")
                    self.registerForRemoteNotifications()
                } else {
                    print("Notification permissions denied: \(error?.localizedDescription ?? "Unknown error")")
                }
            }
        }
    }
    
    @objc func sendStakingRewardNotification(reward: UInt64, totalEarnings: UInt64) {
        let content = UNMutableNotificationContent()
        content.title = "🎉 Daily Staking Reward!"
        content.body = "You earned \(formatBabaChainAmount(reward)) BABA today! Total: \(formatBabaChainAmount(totalEarnings)) BABA (1%+ daily)"
        content.sound = .default
        content.badge = NSNumber(value: UIApplication.shared.applicationIconBadgeNumber + 1)
        content.categoryIdentifier = stakingCategory
        
        // Add rich content
        content.userInfo = [
            "type": "staking_reward",
            "reward": reward,
            "totalEarnings": totalEarnings,
            "timestamp": Date().timeIntervalSince1970
        ]
        
        // Add action buttons
        let viewAction = UNNotificationAction(
            identifier: "VIEW_STAKING",
            title: "View Staking",
            options: [.foreground]
        )
        
        let shareAction = UNNotificationAction(
            identifier: "SHARE_REWARD",
            title: "Share Achievement",
            options: []
        )
        
        let category = UNNotificationCategory(
            identifier: stakingCategory,
            actions: [viewAction, shareAction],
            intentIdentifiers: [],
            options: []
        )
        
        notificationCenter.setNotificationCategories([category])
        
        let request = UNNotificationRequest(
            identifier: "staking_reward_\(Date().timeIntervalSince1970)",
            content: content,
            trigger: nil
        )
        
        notificationCenter.add(request) { error in
            if let error = error {
                print("Failed to send staking notification: \(error)")
            } else {
                // Update widget with new staking data
                self.updateStakingWidget(reward: reward, totalEarnings: totalEarnings)
            }
        }
    }
    
    @objc func sendTransactionNotification(amount: UInt64, isReceived: Bool, txHash: String) {
        let content = UNMutableNotificationContent()
        
        if isReceived {
            content.title = "💰 Payment Received"
            content.body = "You received \(formatBabaChainAmount(amount)) BABA"
        } else {
            content.title = "📤 Payment Sent"
            content.body = "You sent \(formatBabaChainAmount(amount)) BABA"
        }
        
        content.sound = .default
        content.categoryIdentifier = transactionCategory
        content.userInfo = [
            "type": "transaction",
            "amount": amount,
            "isReceived": isReceived,
            "txHash": txHash,
            "timestamp": Date().timeIntervalSince1970
        ]
        
        let request = UNNotificationRequest(
            identifier: "transaction_\(txHash)",
            content: content,
            trigger: nil
        )
        
        notificationCenter.add(request) { error in
            if let error = error {
                print("Failed to send transaction notification: \(error)")
            } else {
                // Update balance widget
                self.updateBalanceWidget()
            }
        }
    }
    
    @objc func sendNetworkStatusNotification(isConnected: Bool, peerCount: Int) {
        let content = UNMutableNotificationContent()
        
        if isConnected {
            content.title = "🌐 Network Connected"
            content.body = "Connected to \(peerCount) peers. Staking is active."
        } else {
            content.title = "⚠️ Network Disconnected"
            content.body = "Lost connection to BabaChain network. Staking paused."
        }
        
        content.sound = isConnected ? .default : .critical
        content.categoryIdentifier = networkCategory
        content.userInfo = [
            "type": "network_status",
            "isConnected": isConnected,
            "peerCount": peerCount,
            "timestamp": Date().timeIntervalSince1970
        ]
        
        let request = UNNotificationRequest(
            identifier: "network_status_\(Date().timeIntervalSince1970)",
            content: content,
            trigger: nil
        )
        
        notificationCenter.add(request) { error in
            if let error = error {
                print("Failed to send network notification: \(error)")
            }
        }
    }
    
    @objc func scheduleStakingMilestoneNotification(milestone: UInt64, estimatedTime: TimeInterval) {
        let content = UNMutableNotificationContent()
        content.title = "🎯 Staking Milestone Alert"
        content.body = "You're close to earning \(formatBabaChainAmount(milestone)) BABA in total rewards!"
        content.sound = .default
        content.categoryIdentifier = stakingCategory
        
        let trigger = UNTimeIntervalNotificationTrigger(timeInterval: estimatedTime, repeats: false)
        
        let request = UNNotificationRequest(
            identifier: "milestone_\(milestone)",
            content: content,
            trigger: trigger
        )
        
        notificationCenter.add(request) { error in
            if let error = error {
                print("Failed to schedule milestone notification: \(error)")
            }
        }
    }
    
    // MARK: - Widget Updates
    
    private func updateStakingWidget(reward: UInt64, totalEarnings: UInt64) {
        let stakingData = StakingWidgetData(
            currentReward: reward,
            totalEarnings: totalEarnings,
            isStaking: true,
            lastUpdate: Date()
        )
        
        // Save widget data for widget extension
        saveWidgetData(stakingData, forKey: "StakingWidgetData")
        
        // Reload widget timeline
        WidgetCenter.shared.reloadTimelines(ofKind: "BabaChainStakingWidget")
    }
    
    private func updateBalanceWidget() {
        // Get current balance from wallet
        let balance = getCurrentWalletBalance()
        
        let balanceData = BalanceWidgetData(
            balance: balance,
            fiatValue: calculateFiatValue(balance),
            currency: App.fiatCurrency,
            lastUpdate: Date()
        )
        
        saveWidgetData(balanceData, forKey: "BalanceWidgetData")
        WidgetCenter.shared.reloadTimelines(ofKind: "BabaChainBalanceWidget")
    }
    
    // MARK: - Private Methods
    
    private func setupNotificationCategories() {
        // Staking category with actions
        let viewStakingAction = UNNotificationAction(
            identifier: "VIEW_STAKING",
            title: "View Staking",
            options: [.foreground]
        )
        
        let shareRewardAction = UNNotificationAction(
            identifier: "SHARE_REWARD",
            title: "Share",
            options: []
        )
        
        let stakingCategory = UNNotificationCategory(
            identifier: self.stakingCategory,
            actions: [viewStakingAction, shareRewardAction],
            intentIdentifiers: [],
            options: []
        )
        
        // Transaction category with actions
        let viewTransactionAction = UNNotificationAction(
            identifier: "VIEW_TRANSACTION",
            title: "View Details",
            options: [.foreground]
        )
        
        let transactionCategory = UNNotificationCategory(
            identifier: self.transactionCategory,
            actions: [viewTransactionAction],
            intentIdentifiers: [],
            options: []
        )
        
        // Network category
        let networkCategory = UNNotificationCategory(
            identifier: self.networkCategory,
            actions: [],
            intentIdentifiers: [],
            options: []
        )
        
        notificationCenter.setNotificationCategories([stakingCategory, transactionCategory, networkCategory])
    }
    
    private func registerForRemoteNotifications() {
        DispatchQueue.main.async {
            UIApplication.shared.registerForRemoteNotifications()
        }
    }
    
    private func formatBabaChainAmount(_ amount: UInt64) -> String {
        let babaAmount = Double(amount) / Double(kOneBabaChain)
        return NumberFormatter.babachainFormatter.string(from: NSNumber(value: babaAmount)) ?? "0"
    }
    
    private func getCurrentWalletBalance() -> UInt64 {
        // Get current wallet balance - this would integrate with existing wallet system
        return 0 // Placeholder
    }
    
    private func calculateFiatValue(_ babaAmount: UInt64) -> Double {
        // Calculate fiat value based on current exchange rate
        let babaValue = Double(babaAmount) / Double(kOneBabaChain)
        let exchangeRate = 1.0 // Placeholder - would get from exchange rate service
        return babaValue * exchangeRate
    }
    
    private func saveWidgetData<T: Codable>(_ data: T, forKey key: String) {
        if let encoded = try? JSONEncoder().encode(data) {
            let userDefaults = UserDefaults(suiteName: "group.org.babachaincore.babachainwallet")
            userDefaults?.set(encoded, forKey: key)
        }
    }
}

// MARK: - UNUserNotificationCenterDelegate

extension BabaChainNotificationManager: UNUserNotificationCenterDelegate {
    
    func userNotificationCenter(_ center: UNUserNotificationCenter, willPresent notification: UNNotification, withCompletionHandler completionHandler: @escaping (UNNotificationPresentationOptions) -> Void) {
        // Show notification even when app is in foreground
        completionHandler([.banner, .sound, .badge])
    }
    
    func userNotificationCenter(_ center: UNUserNotificationCenter, didReceive response: UNNotificationResponse, withCompletionHandler completionHandler: @escaping () -> Void) {
        
        let userInfo = response.notification.request.content.userInfo
        
        switch response.actionIdentifier {
        case "VIEW_STAKING":
            // Navigate to staking screen
            NotificationCenter.default.post(name: .navigateToStaking, object: nil)
            
        case "SHARE_REWARD":
            if let reward = userInfo["reward"] as? UInt64 {
                shareStakingAchievement(reward: reward)
            }
            
        case "VIEW_TRANSACTION":
            if let txHash = userInfo["txHash"] as? String {
                // Navigate to transaction details
                NotificationCenter.default.post(name: .navigateToTransaction, object: txHash)
            }
            
        default:
            // Handle default tap
            if let type = userInfo["type"] as? String {
                handleNotificationTap(type: type, userInfo: userInfo)
            }
        }
        
        completionHandler()
    }
    
    private func shareStakingAchievement(reward: UInt64) {
        let message = "I just earned \(formatBabaChainAmount(reward)) BABA from staking! 🎉 #BabaChain #Staking"
        
        NotificationCenter.default.post(
            name: .shareContent,
            object: nil,
            userInfo: ["message": message]
        )
    }
    
    private func handleNotificationTap(type: String, userInfo: [AnyHashable: Any]) {
        switch type {
        case "staking_reward":
            NotificationCenter.default.post(name: .navigateToStaking, object: nil)
        case "transaction":
            if let txHash = userInfo["txHash"] as? String {
                NotificationCenter.default.post(name: .navigateToTransaction, object: txHash)
            }
        case "network_status":
            NotificationCenter.default.post(name: .navigateToNetworkStatus, object: nil)
        default:
            break
        }
    }
}

// MARK: - Widget Data Models

struct StakingWidgetData: Codable {
    let currentReward: UInt64
    let totalEarnings: UInt64
    let isStaking: Bool
    let lastUpdate: Date
}

struct BalanceWidgetData: Codable {
    let balance: UInt64
    let fiatValue: Double
    let currency: String
    let lastUpdate: Date
}

// MARK: - Navigation Notifications

extension Notification.Name {
    static let navigateToStaking = Notification.Name("NavigateToStaking")
    static let navigateToTransaction = Notification.Name("NavigateToTransaction")
    static let navigateToNetworkStatus = Notification.Name("NavigateToNetworkStatus")
    static let shareContent = Notification.Name("ShareContent")
}