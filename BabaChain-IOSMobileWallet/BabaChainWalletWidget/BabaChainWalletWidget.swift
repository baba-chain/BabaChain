//
//  BabaChainWalletWidget.swift
//  BabaChainWalletWidget
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

import WidgetKit
import SwiftUI

// MARK: - Widget Entry

struct BabaChainWidgetEntry: TimelineEntry {
    let date: Date
    let balance: UInt64
    let fiatValue: Double
    let currency: String
    let stakingReward: UInt64
    let isStaking: Bool
    let syncProgress: Double
}

// MARK: - Widget Provider

struct BabaChainWidgetProvider: TimelineProvider {
    
    func placeholder(in context: Context) -> BabaChainWidgetEntry {
        BabaChainWidgetEntry(
            date: Date(),
            balance: 1000000000, // 10 BABA
            fiatValue: 5.0,
            currency: "USD",
            stakingReward: 50000000, // 0.5 BABA
            isStaking: true,
            syncProgress: 1.0
        )
    }
    
    func getSnapshot(in context: Context, completion: @escaping (BabaChainWidgetEntry) -> ()) {
        let entry = createEntry()
        completion(entry)
    }
    
    func getTimeline(in context: Context, completion: @escaping (Timeline<BabaChainWidgetEntry>) -> ()) {
        let currentDate = Date()
        let entry = createEntry()
        
        // Update every 5 minutes
        let nextUpdate = Calendar.current.date(byAdding: .minute, value: 5, to: currentDate)!
        let timeline = Timeline(entries: [entry], policy: .after(nextUpdate))
        
        completion(timeline)
    }
    
    private func createEntry() -> BabaChainWidgetEntry {
        let widgetData = loadWidgetData()
        
        return BabaChainWidgetEntry(
            date: Date(),
            balance: widgetData.balance,
            fiatValue: widgetData.fiatValue,
            currency: widgetData.currency,
            stakingReward: widgetData.stakingReward,
            isStaking: widgetData.isStaking,
            syncProgress: widgetData.syncProgress
        )
    }
    
    private func loadWidgetData() -> WidgetData {
        let userDefaults = UserDefaults(suiteName: "group.org.babachaincore.babachainwallet")
        
        // Load balance data
        var balance: UInt64 = 0
        var fiatValue: Double = 0
        var currency = "USD"
        
        if let balanceData = userDefaults?.data(forKey: "BalanceWidgetData"),
           let balanceInfo = try? JSONDecoder().decode(BalanceWidgetData.self, from: balanceData) {
            balance = balanceInfo.balance
            fiatValue = balanceInfo.fiatValue
            currency = balanceInfo.currency
        }
        
        // Load staking data
        var stakingReward: UInt64 = 0
        var isStaking = false
        
        if let stakingData = userDefaults?.data(forKey: "StakingWidgetData"),
           let stakingInfo = try? JSONDecoder().decode(StakingWidgetData.self, from: stakingData) {
            stakingReward = stakingInfo.currentReward
            isStaking = stakingInfo.isStaking
        }
        
        // Load sync progress
        let syncProgress = userDefaults?.double(forKey: "SyncProgress") ?? 1.0
        
        return WidgetData(
            balance: balance,
            fiatValue: fiatValue,
            currency: currency,
            stakingReward: stakingReward,
            isStaking: isStaking,
            syncProgress: syncProgress
        )
    }
}

// MARK: - Widget Views

struct BabaChainWidgetSmallView: View {
    let entry: BabaChainWidgetEntry
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Image("babachain_logo")
                    .resizable()
                    .frame(width: 20, height: 20)
                Spacer()
                if entry.isStaking {
                    Image(systemName: "bolt.fill")
                        .foregroundColor(.green)
                        .font(.caption)
                }
            }
            
            Text(formatBabaChainAmount(entry.balance))
                .font(.headline)
                .fontWeight(.bold)
                .foregroundColor(.primary)
            
            Text("\(formatFiatAmount(entry.fiatValue)) \(entry.currency)")
                .font(.caption)
                .foregroundColor(.secondary)
            
            if entry.isStaking {
                HStack {
                    Text("Earning:")
                        .font(.caption2)
                        .foregroundColor(.secondary)
                    Text(formatBabaChainAmount(entry.stakingReward))
                        .font(.caption2)
                        .fontWeight(.medium)
                        .foregroundColor(.green)
                }
            }
        }
        .padding()
        .background(Color(.systemBackground))
    }
}

struct BabaChainWidgetMediumView: View {
    let entry: BabaChainWidgetEntry
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Image("babachain_logo")
                        .resizable()
                        .frame(width: 24, height: 24)
                    Text("BabaChain")
                        .font(.headline)
                        .fontWeight(.bold)
                    Spacer()
                    if entry.isStaking {
                        HStack(spacing: 2) {
                            Image(systemName: "bolt.fill")
                                .foregroundColor(.green)
                            Text("Staking")
                                .font(.caption)
                                .foregroundColor(.green)
                        }
                    }
                }
                
                VStack(alignment: .leading, spacing: 4) {
                    Text("Balance")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    Text(formatBabaChainAmount(entry.balance))
                        .font(.title2)
                        .fontWeight(.bold)
                    Text("\(formatFiatAmount(entry.fiatValue)) \(entry.currency)")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                
                if entry.syncProgress < 1.0 {
                    VStack(alignment: .leading, spacing: 2) {
                        Text("Syncing...")
                            .font(.caption2)
                            .foregroundColor(.orange)
                        ProgressView(value: entry.syncProgress)
                            .progressViewStyle(LinearProgressViewStyle())
                    }
                }
            }
            
            Spacer()
            
            if entry.isStaking {
                VStack(alignment: .trailing, spacing: 8) {
                    Text("Today's Rewards")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    Text(formatBabaChainAmount(entry.stakingReward))
                        .font(.title3)
                        .fontWeight(.semibold)
                        .foregroundColor(.green)
                    
                    Text("365%+ APR")
                        .font(.caption2)
                        .fontWeight(.medium)
                        .foregroundColor(.green)
                        .padding(.horizontal, 6)
                        .padding(.vertical, 2)
                        .background(Color.green.opacity(0.1))
                        .cornerRadius(4)
                }
            }
        }
        .padding()
        .background(Color(.systemBackground))
    }
}

struct BabaChainWidgetLargeView: View {
    let entry: BabaChainWidgetEntry
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            // Header
            HStack {
                Image("babachain_logo")
                    .resizable()
                    .frame(width: 28, height: 28)
                Text("BabaChain Wallet")
                    .font(.title3)
                    .fontWeight(.bold)
                Spacer()
                Text(formatTime(entry.date))
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            // Balance Section
            VStack(alignment: .leading, spacing: 6) {
                Text("Total Balance")
                    .font(.subheadline)
                    .foregroundColor(.secondary)
                
                HStack(alignment: .bottom) {
                    Text(formatBabaChainAmount(entry.balance))
                        .font(.largeTitle)
                        .fontWeight(.bold)
                    Text("BABA")
                        .font(.title3)
                        .foregroundColor(.secondary)
                        .padding(.bottom, 4)
                }
                
                Text("\(formatFiatAmount(entry.fiatValue)) \(entry.currency)")
                    .font(.subheadline)
                    .foregroundColor(.secondary)
            }
            
            Divider()
            
            // Staking Section
            if entry.isStaking {
                HStack {
                    VStack(alignment: .leading, spacing: 4) {
                        HStack {
                            Image(systemName: "bolt.fill")
                                .foregroundColor(.green)
                            Text("Staking Active")
                                .font(.subheadline)
                                .fontWeight(.medium)
                                .foregroundColor(.green)
                        }
                        
                        Text("Today's Rewards")
                            .font(.caption)
                            .foregroundColor(.secondary)
                        
                        Text(formatBabaChainAmount(entry.stakingReward))
                            .font(.title2)
                            .fontWeight(.semibold)
                            .foregroundColor(.green)
                    }
                    
                    Spacer()
                    
                    VStack(alignment: .trailing, spacing: 4) {
                        Text("365%+ APR")
                            .font(.headline)
                            .fontWeight(.bold)
                            .foregroundColor(.green)
                        
                        Text("Auto-staking enabled")
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                }
            } else {
                HStack {
                    Image(systemName: "pause.circle")
                        .foregroundColor(.orange)
                    Text("Staking Inactive")
                        .font(.subheadline)
                        .foregroundColor(.orange)
                    Spacer()
                    Text("Tap to enable")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
            
            // Sync Status
            if entry.syncProgress < 1.0 {
                HStack {
                    Image(systemName: "arrow.triangle.2.circlepath")
                        .foregroundColor(.blue)
                    VStack(alignment: .leading, spacing: 2) {
                        Text("Syncing blockchain...")
                            .font(.caption)
                            .foregroundColor(.blue)
                        ProgressView(value: entry.syncProgress)
                            .progressViewStyle(LinearProgressViewStyle())
                    }
                }
            }
        }
        .padding()
        .background(Color(.systemBackground))
    }
}

// MARK: - Widget Configuration

struct BabaChainWidget: Widget {
    let kind: String = "BabaChainWidget"
    
    var body: some WidgetConfiguration {
        StaticConfiguration(kind: kind, provider: BabaChainWidgetProvider()) { entry in
            BabaChainWidgetEntryView(entry: entry)
        }
        .configurationDisplayName("BabaChain Wallet")
        .description("Keep track of your BabaChain balance and staking rewards.")
        .supportedFamilies([.systemSmall, .systemMedium, .systemLarge])
    }
}

struct BabaChainWidgetEntryView: View {
    var entry: BabaChainWidgetProvider.Entry
    @Environment(\.widgetFamily) var family
    
    var body: some View {
        switch family {
        case .systemSmall:
            BabaChainWidgetSmallView(entry: entry)
        case .systemMedium:
            BabaChainWidgetMediumView(entry: entry)
        case .systemLarge:
            BabaChainWidgetLargeView(entry: entry)
        default:
            BabaChainWidgetMediumView(entry: entry)
        }
    }
}

// MARK: - Helper Functions

private func formatBabaChainAmount(_ amount: UInt64) -> String {
    let babaAmount = Double(amount) / Double(kOneBabaChain)
    let formatter = NumberFormatter()
    formatter.numberStyle = .decimal
    formatter.minimumFractionDigits = 2
    formatter.maximumFractionDigits = 4
    return formatter.string(from: NSNumber(value: babaAmount)) ?? "0"
}

private func formatFiatAmount(_ amount: Double) -> String {
    let formatter = NumberFormatter()
    formatter.numberStyle = .currency
    formatter.currencyCode = "USD"
    formatter.minimumFractionDigits = 2
    formatter.maximumFractionDigits = 2
    return formatter.string(from: NSNumber(value: amount)) ?? "$0.00"
}

private func formatTime(_ date: Date) -> String {
    let formatter = DateFormatter()
    formatter.timeStyle = .short
    return formatter.string(from: date)
}

// MARK: - Supporting Types

struct WidgetData {
    let balance: UInt64
    let fiatValue: Double
    let currency: String
    let stakingReward: UInt64
    let isStaking: Bool
    let syncProgress: Double
}

// MARK: - Widget Preview

struct BabaChainWidget_Previews: PreviewProvider {
    static var previews: some View {
        let entry = BabaChainWidgetEntry(
            date: Date(),
            balance: 1000000000, // 10 BABA
            fiatValue: 5.0,
            currency: "USD",
            stakingReward: 50000000, // 0.5 BABA
            isStaking: true,
            syncProgress: 1.0
        )
        
        Group {
            BabaChainWidgetEntryView(entry: entry)
                .previewContext(WidgetPreviewContext(family: .systemSmall))
            
            BabaChainWidgetEntryView(entry: entry)
                .previewContext(WidgetPreviewContext(family: .systemMedium))
            
            BabaChainWidgetEntryView(entry: entry)
                .previewContext(WidgetPreviewContext(family: .systemLarge))
        }
    }
}