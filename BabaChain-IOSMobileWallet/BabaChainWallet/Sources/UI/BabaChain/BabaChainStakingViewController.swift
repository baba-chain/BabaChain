//
//  BabaChainStakingViewController.swift
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

import UIKit
import Combine

/// View controller for BabaChain staking interface
class BabaChainStakingViewController: UIViewController {
    
    // MARK: - Properties
    
    private let spvNode = SPVLightNode.shared
    private let stakingManager = BackgroundStakingManager.shared
    private let biometricAuth = BiometricAuthentication.shared
    private let notificationManager = BabaChainNotificationManager.shared
    private let applePayIntegration = ApplePayIntegration.shared
    
    private var cancellables = Set<AnyCancellable>()
    
    // MARK: - UI Elements
    
    @IBOutlet weak var balanceLabel: UILabel!
    @IBOutlet weak var stakingStatusLabel: UILabel!
    @IBOutlet weak var stakingToggleButton: UIButton!
    @IBOutlet weak var rewardsLabel: UILabel!
    @IBOutlet weak var aprLabel: UILabel!
    @IBOutlet weak var syncProgressView: UIProgressView!
    @IBOutlet weak var buyBabaChainButton: UIButton!
    @IBOutlet weak var enableBiometricsButton: UIButton!
    
    // MARK: - Lifecycle
    
    override func viewDidLoad() {
        super.viewDidLoad()
        setupUI()
        setupBindings()
        updateUI()
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        updateStakingStatus()
    }
    
    // MARK: - Setup
    
    private func setupUI() {
        title = "BabaChain Staking"
        
        // Configure buttons
        stakingToggleButton.layer.cornerRadius = 8
        buyBabaChainButton.layer.cornerRadius = 8
        enableBiometricsButton.layer.cornerRadius = 8
        
        // Set initial button states
        updateBiometricButton()
        
        // Configure APR label
        aprLabel.text = "365% APR"
        aprLabel.textColor = .systemGreen
    }
    
    private func setupBindings() {
        // Bind to SPV node updates
        spvNode.$syncProgress
            .receive(on: DispatchQueue.main)
            .sink { [weak self] progress in
                self?.syncProgressView.progress = Float(progress)
                self?.syncProgressView.isHidden = progress >= 1.0
            }
            .store(in: &cancellables)
        
        spvNode.$isConnected
            .receive(on: DispatchQueue.main)
            .sink { [weak self] isConnected in
                self?.updateConnectionStatus(isConnected)
            }
            .store(in: &cancellables)
        
        spvNode.$stakingStatus
            .receive(on: DispatchQueue.main)
            .sink { [weak self] status in
                self?.updateStakingStatus(status)
            }
            .store(in: &cancellables)
        
        // Listen for staking rewards
        NotificationCenter.default.publisher(for: .stakingRewardReceived)
            .receive(on: DispatchQueue.main)
            .sink { [weak self] notification in
                self?.handleStakingReward(notification)
            }
            .store(in: &cancellables)
    }
    
    // MARK: - Actions
    
    @IBAction func stakingToggleButtonTapped(_ sender: UIButton) {
        let currentStakingInfo = spvNode.getCurrentStakingInfo()
        
        if currentStakingInfo?.isStaking == true {
            // Disable staking
            spvNode.disableStaking()
            updateStakingToggleButton(isStaking: false)
        } else {
            // Enable staking with biometric authentication
            enableStakingWithAuthentication()
        }
    }
    
    @IBAction func buyBabaChainButtonTapped(_ sender: UIButton) {
        if applePayIntegration.isApplePayAvailable() {
            applePayIntegration.presentQuickBuyOptions(from: self)
        } else {
            showAlert(title: "Apple Pay Not Available", message: "Please set up Apple Pay to purchase BabaChain.")
        }
    }
    
    @IBAction func enableBiometricsButtonTapped(_ sender: UIButton) {
        if biometricAuth.isBiometricAuthenticationEnabled() {
            // Disable biometrics
            biometricAuth.disableBiometricAuthentication()
            updateBiometricButton()
        } else {
            // Enable biometrics
            biometricAuth.enableBiometricAuthentication { [weak self] success, error in
                DispatchQueue.main.async {
                    if success {
                        self?.updateBiometricButton()
                        self?.showAlert(title: "Success", message: "\(self?.biometricAuth.getBiometricType().displayName ?? "Biometric") authentication enabled.")
                    } else {
                        self?.showAlert(title: "Error", message: error?.localizedDescription ?? "Failed to enable biometric authentication.")
                    }
                }
            }
        }
    }
    
    // MARK: - Private Methods
    
    private func enableStakingWithAuthentication() {
        guard biometricAuth.isBiometricAuthenticationEnabled() else {
            showAlert(title: "Biometric Authentication Required", message: "Please enable Face ID or Touch ID to use staking features.")
            return
        }
        
        biometricAuth.authenticateForStaking { [weak self] success, error in
            DispatchQueue.main.async {
                if success {
                    self?.enableStaking()
                } else {
                    self?.showAlert(title: "Authentication Failed", message: error?.localizedDescription ?? "Authentication required to enable staking.")
                }
            }
        }
    }
    
    private func enableStaking() {
        // For demo purposes, use a mock private key
        // In production, this would use the actual wallet private key
        let mockPrivateKey = "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"
        
        let success = spvNode.enableStaking(with: mockPrivateKey)
        
        if success {
            updateStakingToggleButton(isStaking: true)
            showAlert(title: "Staking Enabled", message: "Your BabaChain is now staking and earning rewards!")
        } else {
            showAlert(title: "Error", message: "Failed to enable staking. Please try again.")
        }
    }
    
    private func updateUI() {
        // Update balance (mock data for demo)
        let mockBalance: UInt64 = 1000000000 // 10 BABA
        let babaAmount = Double(mockBalance) / Double(kOneBabaChain)
        balanceLabel.text = String(format: "%.4f BABA", babaAmount)
        
        // Update rewards (mock data for demo)
        let mockRewards: UInt64 = 50000000 // 0.5 BABA
        let rewardAmount = Double(mockRewards) / Double(kOneBabaChain)
        rewardsLabel.text = String(format: "Today: +%.4f BABA", rewardAmount)
    }
    
    private func updateStakingStatus() {
        let stakingInfo = spvNode.getCurrentStakingInfo()
        updateStakingStatus(spvNode.stakingStatus)
        updateStakingToggleButton(isStaking: stakingInfo?.isStaking ?? false)
    }
    
    private func updateStakingStatus(_ status: StakingStatus) {
        switch status {
        case .inactive:
            stakingStatusLabel.text = "Staking Inactive"
            stakingStatusLabel.textColor = .systemRed
        case .active:
            stakingStatusLabel.text = "Staking Active"
            stakingStatusLabel.textColor = .systemGreen
        case .syncing:
            stakingStatusLabel.text = "Syncing..."
            stakingStatusLabel.textColor = .systemOrange
        case .error(let message):
            stakingStatusLabel.text = "Error: \(message)"
            stakingStatusLabel.textColor = .systemRed
        }
    }
    
    private func updateStakingToggleButton(isStaking: Bool) {
        if isStaking {
            stakingToggleButton.setTitle("Stop Staking", for: .normal)
            stakingToggleButton.backgroundColor = .systemRed
        } else {
            stakingToggleButton.setTitle("Start Staking", for: .normal)
            stakingToggleButton.backgroundColor = .systemGreen
        }
    }
    
    private func updateConnectionStatus(_ isConnected: Bool) {
        // Update UI based on connection status
        if !isConnected {
            showAlert(title: "Network Disconnected", message: "Lost connection to BabaChain network. Staking is paused.")
        }
    }
    
    private func updateBiometricButton() {
        let biometricType = biometricAuth.getBiometricType()
        let isEnabled = biometricAuth.isBiometricAuthenticationEnabled()
        
        if biometricType == .none {
            enableBiometricsButton.isHidden = true
        } else {
            enableBiometricsButton.isHidden = false
            
            if isEnabled {
                enableBiometricsButton.setTitle("Disable \(biometricType.displayName)", for: .normal)
                enableBiometricsButton.backgroundColor = .systemRed
            } else {
                enableBiometricsButton.setTitle("Enable \(biometricType.displayName)", for: .normal)
                enableBiometricsButton.backgroundColor = .systemBlue
            }
        }
    }
    
    private func handleStakingReward(_ notification: Notification) {
        guard let userInfo = notification.userInfo,
              let reward = userInfo["reward"] as? UInt64 else { return }
        
        let rewardAmount = Double(reward) / Double(kOneBabaChain)
        
        // Show reward animation or update UI
        showAlert(title: "Staking Reward!", message: String(format: "You earned %.4f BABA!", rewardAmount))
        
        // Update rewards display
        updateUI()
    }
    
    private func showAlert(title: String, message: String) {
        let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: "OK", style: .default))
        present(alert, animated: true)
    }
}

// MARK: - Factory Method

extension BabaChainStakingViewController {
    static func create() -> BabaChainStakingViewController {
        let storyboard = UIStoryboard(name: "BabaChainStaking", bundle: nil)
        return storyboard.instantiateViewController(withIdentifier: "BabaChainStakingViewController") as! BabaChainStakingViewController
    }
}