//
//  SPVLightNode.swift
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
import Network
import Combine

/// SPV Light Node for BabaChain network with PoS consensus support
@objc class SPVLightNode: NSObject {
    
    // MARK: - Properties
    
    @objc static let shared = SPVLightNode()
    
    private var isRunning = false
    private var connectedPeers: Set<SPVPeer> = []
    private var blockHeaders: [String: BlockHeader] = [:]
    private var stakingInfo: StakingInfo?
    
    private let networkMonitor = NWPathMonitor()
    private let networkQueue = DispatchQueue(label: "SPVNetworkQueue")
    
    // Publishers for reactive updates
    @Published var syncProgress: Double = 0.0
    @Published var isConnected: Bool = false
    @Published var stakingStatus: StakingStatus = .inactive
    
    // MARK: - Initialization
    
    private override init() {
        super.init()
        setupNetworkMonitoring()
    }
    
    // MARK: - Public Methods
    
    @objc func startNode() {
        guard !isRunning else { return }
        
        isRunning = true
        connectToPeers()
        startStakingService()
        
        NotificationCenter.default.post(name: .spvNodeDidStart, object: nil)
    }
    
    @objc func stopNode() {
        guard isRunning else { return }
        
        isRunning = false
        disconnectFromPeers()
        stopStakingService()
        
        NotificationCenter.default.post(name: .spvNodeDidStop, object: nil)
    }
    
    @objc func getCurrentStakingInfo() -> StakingInfo? {
        return stakingInfo
    }
    
    @objc func enableStaking(with privateKey: String) -> Bool {
        guard isRunning else { return false }
        
        // Validate private key and enable staking
        let validator = StakingValidator(privateKey: privateKey)
        
        if validator.isValid {
            stakingStatus = .active
            stakingInfo = StakingInfo(
                isStaking: true,
                balance: getCurrentBalance(),
                expectedReward: calculateExpectedReward(),
                validatorAddress: validator.address
            )
            
            // Schedule background staking tasks
            scheduleBackgroundStaking()
            
            NotificationCenter.default.post(name: .stakingDidStart, object: stakingInfo)
            return true
        }
        
        return false
    }
    
    @objc func disableStaking() {
        stakingStatus = .inactive
        stakingInfo?.isStaking = false
        
        // Cancel background tasks
        cancelBackgroundStaking()
        
        NotificationCenter.default.post(name: .stakingDidStop, object: nil)
    }
    
    // MARK: - Private Methods
    
    private func setupNetworkMonitoring() {
        networkMonitor.pathUpdateHandler = { [weak self] path in
            DispatchQueue.main.async {
                self?.isConnected = path.status == .satisfied
                
                if path.status == .satisfied && self?.isRunning == true {
                    self?.reconnectToPeers()
                }
            }
        }
        networkMonitor.start(queue: networkQueue)
    }
    
    private func connectToPeers() {
        let seedNodes = BabaChainNetwork.seedNodes
        
        for seedNode in seedNodes {
            let peer = SPVPeer(host: seedNode.host, port: seedNode.port)
            peer.delegate = self
            peer.connect()
            connectedPeers.insert(peer)
        }
    }
    
    private func disconnectFromPeers() {
        for peer in connectedPeers {
            peer.disconnect()
        }
        connectedPeers.removeAll()
    }
    
    private func reconnectToPeers() {
        disconnectFromPeers()
        connectToPeers()
    }
    
    private func startStakingService() {
        // Initialize staking service for PoS consensus
        StakingService.shared.start()
    }
    
    private func stopStakingService() {
        StakingService.shared.stop()
    }
    
    private func getCurrentBalance() -> UInt64 {
        // Get current wallet balance
        // This would integrate with the existing wallet balance system
        return 0 // Placeholder
    }
    
    private func calculateExpectedReward() -> UInt64 {
        // Calculate expected staking rewards based on current stake
        // Using BabaChain's gradual bonus system: 1% daily base + gradual bonuses
        let balance = getCurrentBalance()
        let baseDailyRate: Double = 1.0 // 1% daily base rate
        let gradualBonus = calculateGradualBonus(for: balance)
        let totalDailyRate = baseDailyRate + gradualBonus
        
        return UInt64(Double(balance) * totalDailyRate / 100.0)
    }
    
    private func calculateGradualBonus(for balance: UInt64) -> Double {
        // Gradual bonus system: smooth progression based on stake size
        let babaAmount = Double(balance) / Double(kOneBabaChain)
        
        if babaAmount <= 10000 {
            // 0% to 5% bonus for 1-10,000 BABA
            return (babaAmount / 10000.0) * 5.0
        } else if babaAmount <= 100000 {
            // 5% to 20% bonus for 10,000-100,000 BABA
            let progress = (babaAmount - 10000) / 90000.0
            return 5.0 + (progress * 15.0)
        } else {
            // Maximum 20% bonus for 100,000+ BABA
            return 20.0
        }
    }
    
    private func scheduleBackgroundStaking() {
        // Schedule background app refresh for continuous staking
        BackgroundStakingManager.shared.scheduleStakingTasks()
    }
    
    private func cancelBackgroundStaking() {
        BackgroundStakingManager.shared.cancelStakingTasks()
    }
}

// MARK: - SPVPeerDelegate

extension SPVLightNode: SPVPeerDelegate {
    func peer(_ peer: SPVPeer, didReceiveBlockHeader header: BlockHeader) {
        blockHeaders[header.hash] = header
        updateSyncProgress()
    }
    
    func peer(_ peer: SPVPeer, didConnect: Bool) {
        if didConnect {
            // Request block headers for SPV sync
            peer.requestBlockHeaders()
        }
    }
    
    func peer(_ peer: SPVPeer, didDisconnect error: Error?) {
        connectedPeers.remove(peer)
        
        // Attempt to reconnect if network is available
        if isConnected && isRunning {
            DispatchQueue.main.asyncAfter(deadline: .now() + 5.0) {
                self.connectToPeers()
            }
        }
    }
    
    private func updateSyncProgress() {
        // Calculate sync progress based on block headers received
        let currentHeight = blockHeaders.count
        let targetHeight = BabaChainNetwork.currentBlockHeight
        
        if targetHeight > 0 {
            syncProgress = Double(currentHeight) / Double(targetHeight)
        }
    }
}

// MARK: - Supporting Types

struct StakingInfo {
    var isStaking: Bool
    var balance: UInt64
    var expectedReward: UInt64
    var validatorAddress: String
    var lastRewardTime: Date?
    
    init(isStaking: Bool, balance: UInt64, expectedReward: UInt64, validatorAddress: String) {
        self.isStaking = isStaking
        self.balance = balance
        self.expectedReward = expectedReward
        self.validatorAddress = validatorAddress
        self.lastRewardTime = nil
    }
}

enum StakingStatus {
    case inactive
    case active
    case syncing
    case error(String)
}

struct BlockHeader {
    let hash: String
    let previousHash: String
    let merkleRoot: String
    let timestamp: UInt32
    let height: UInt32
    let stakingData: StakingData?
}

struct StakingData {
    let validatorAddress: String
    let stakeAmount: UInt64
    let rewardAmount: UInt64
}

// MARK: - Notifications

extension Notification.Name {
    static let spvNodeDidStart = Notification.Name("SPVNodeDidStart")
    static let spvNodeDidStop = Notification.Name("SPVNodeDidStop")
    static let stakingDidStart = Notification.Name("StakingDidStart")
    static let stakingDidStop = Notification.Name("StakingDidStop")
    static let stakingRewardReceived = Notification.Name("StakingRewardReceived")
}