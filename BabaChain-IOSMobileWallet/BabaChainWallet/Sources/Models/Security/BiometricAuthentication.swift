//
//  BiometricAuthentication.swift
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
import LocalAuthentication
import Security

/// Manages Face ID/Touch ID authentication for BabaChain wallet security
@objc class BiometricAuthentication: NSObject {
    
    // MARK: - Properties
    
    @objc static let shared = BiometricAuthentication()
    
    private let context = LAContext()
    private let keychainService = "org.babachaincore.babachainwallet.biometric"
    
    // MARK: - Public Methods
    
    @objc func isBiometricAuthenticationAvailable() -> Bool {
        var error: NSError?
        return context.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: &error)
    }
    
    @objc func getBiometricType() -> BiometricType {
        guard isBiometricAuthenticationAvailable() else { return .none }
        
        switch context.biometryType {
        case .faceID:
            return .faceID
        case .touchID:
            return .touchID
        case .opticID:
            return .opticID
        default:
            return .none
        }
    }
    
    @objc func isBiometricAuthenticationEnabled() -> Bool {
        return UserDefaults.standard.bool(forKey: "BiometricAuthenticationEnabled")
    }
    
    @objc func enableBiometricAuthentication(completion: @escaping (Bool, Error?) -> Void) {
        guard isBiometricAuthenticationAvailable() else {
            completion(false, BiometricError.notAvailable)
            return
        }
        
        let reason = "Enable \(getBiometricType().displayName) to secure your BabaChain wallet"
        
        context.evaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, localizedReason: reason) { [weak self] success, error in
            DispatchQueue.main.async {
                if success {
                    UserDefaults.standard.set(true, forKey: "BiometricAuthenticationEnabled")
                    self?.storeBiometricKey()
                    completion(true, nil)
                } else {
                    completion(false, error)
                }
            }
        }
    }
    
    @objc func disableBiometricAuthentication() {
        UserDefaults.standard.set(false, forKey: "BiometricAuthenticationEnabled")
        removeBiometricKey()
    }
    
    @objc func authenticateWithBiometrics(reason: String, completion: @escaping (Bool, Error?) -> Void) {
        guard isBiometricAuthenticationEnabled() else {
            completion(false, BiometricError.notEnabled)
            return
        }
        
        let context = LAContext()
        context.localizedFallbackTitle = "Use Passcode"
        
        context.evaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, localizedReason: reason) { success, error in
            DispatchQueue.main.async {
                completion(success, error)
            }
        }
    }
    
    @objc func authenticateForWalletAccess(completion: @escaping (Bool, Error?) -> Void) {
        let reason = "Authenticate to access your BabaChain wallet"
        authenticateWithBiometrics(reason: reason, completion: completion)
    }
    
    @objc func authenticateForTransaction(amount: UInt64, completion: @escaping (Bool, Error?) -> Void) {
        let babaAmount = Double(amount) / Double(kOneBabaChain)
        let formattedAmount = NumberFormatter.babachainFormatter.string(from: NSNumber(value: babaAmount)) ?? "0"
        let reason = "Authenticate to send \(formattedAmount) BABA"
        
        authenticateWithBiometrics(reason: reason, completion: completion)
    }
    
    @objc func authenticateForStaking(completion: @escaping (Bool, Error?) -> Void) {
        let reason = "Authenticate to enable staking"
        authenticateWithBiometrics(reason: reason, completion: completion)
    }
    
    @objc func authenticateForSeedPhraseAccess(completion: @escaping (Bool, Error?) -> Void) {
        let reason = "Authenticate to view your recovery phrase"
        authenticateWithBiometrics(reason: reason, completion: completion)
    }
    
    @objc func authenticateForPrivateKeyAccess(completion: @escaping (Bool, Error?) -> Void) {
        let reason = "Authenticate to access private keys"
        authenticateWithBiometrics(reason: reason, completion: completion)
    }
    
    // MARK: - Secure Storage
    
    func storeSecureData(_ data: Data, forKey key: String, completion: @escaping (Bool, Error?) -> Void) {
        guard isBiometricAuthenticationEnabled() else {
            completion(false, BiometricError.notEnabled)
            return
        }
        
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: keychainService,
            kSecAttrAccount as String: key,
            kSecValueData as String: data,
            kSecAttrAccessControl as String: createAccessControl()
        ]
        
        // Delete existing item first
        SecItemDelete(query as CFDictionary)
        
        let status = SecItemAdd(query as CFDictionary, nil)
        
        if status == errSecSuccess {
            completion(true, nil)
        } else {
            completion(false, BiometricError.keychainError(status))
        }
    }
    
    func retrieveSecureData(forKey key: String, completion: @escaping (Data?, Error?) -> Void) {
        guard isBiometricAuthenticationEnabled() else {
            completion(nil, BiometricError.notEnabled)
            return
        }
        
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: keychainService,
            kSecAttrAccount as String: key,
            kSecReturnData as String: true,
            kSecMatchLimit as String: kSecMatchLimitOne
        ]
        
        var result: AnyObject?
        let status = SecItemCopyMatching(query as CFDictionary, &result)
        
        if status == errSecSuccess {
            completion(result as? Data, nil)
        } else {
            completion(nil, BiometricError.keychainError(status))
        }
    }
    
    // MARK: - Private Methods
    
    private func createAccessControl() -> SecAccessControl {
        var error: Unmanaged<CFError>?
        
        let accessControl = SecAccessControlCreateWithFlags(
            kCFAllocatorDefault,
            kSecAttrAccessibleWhenUnlockedThisDeviceOnly,
            .biometryAny,
            &error
        )
        
        return accessControl!
    }
    
    private func storeBiometricKey() {
        // Generate and store a unique key for biometric authentication
        let keyData = generateRandomKey()
        
        storeSecureData(keyData, forKey: "BiometricAuthKey") { success, error in
            if !success {
                print("Failed to store biometric key: \(error?.localizedDescription ?? "Unknown error")")
            }
        }
    }
    
    private func removeBiometricKey() {
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: keychainService,
            kSecAttrAccount as String: "BiometricAuthKey"
        ]
        
        SecItemDelete(query as CFDictionary)
    }
    
    private func generateRandomKey() -> Data {
        var keyData = Data(count: 32)
        let result = keyData.withUnsafeMutableBytes {
            SecRandomCopyBytes(kSecRandomDefault, 32, $0.bindMemory(to: UInt8.self).baseAddress!)
        }
        
        if result == errSecSuccess {
            return keyData
        } else {
            // Fallback to UUID-based key
            return UUID().uuidString.data(using: .utf8) ?? Data()
        }
    }
}

// MARK: - Biometric Type Enum

@objc enum BiometricType: Int, CaseIterable {
    case none = 0
    case touchID = 1
    case faceID = 2
    case opticID = 3
    
    var displayName: String {
        switch self {
        case .none:
            return "None"
        case .touchID:
            return "Touch ID"
        case .faceID:
            return "Face ID"
        case .opticID:
            return "Optic ID"
        }
    }
    
    var icon: String {
        switch self {
        case .none:
            return "lock"
        case .touchID:
            return "touchid"
        case .faceID:
            return "faceid"
        case .opticID:
            return "opticid"
        }
    }
}

// MARK: - Biometric Errors

enum BiometricError: LocalizedError {
    case notAvailable
    case notEnabled
    case keychainError(OSStatus)
    
    var errorDescription: String? {
        switch self {
        case .notAvailable:
            return "Biometric authentication is not available on this device"
        case .notEnabled:
            return "Biometric authentication is not enabled"
        case .keychainError(let status):
            return "Keychain error: \(status)"
        }
    }
}

// MARK: - Secure Wallet Operations

extension BiometricAuthentication {
    
    @objc func securelyStoreSeedPhrase(_ seedPhrase: String, completion: @escaping (Bool, Error?) -> Void) {
        guard let data = seedPhrase.data(using: .utf8) else {
            completion(false, BiometricError.keychainError(errSecParam))
            return
        }
        
        storeSecureData(data, forKey: "WalletSeedPhrase", completion: completion)
    }
    
    @objc func securelyRetrieveSeedPhrase(completion: @escaping (String?, Error?) -> Void) {
        retrieveSecureData(forKey: "WalletSeedPhrase") { data, error in
            if let data = data {
                let seedPhrase = String(data: data, encoding: .utf8)
                completion(seedPhrase, nil)
            } else {
                completion(nil, error)
            }
        }
    }
    
    @objc func securelyStorePrivateKey(_ privateKey: String, forAddress address: String, completion: @escaping (Bool, Error?) -> Void) {
        guard let data = privateKey.data(using: .utf8) else {
            completion(false, BiometricError.keychainError(errSecParam))
            return
        }
        
        storeSecureData(data, forKey: "PrivateKey_\(address)", completion: completion)
    }
    
    @objc func securelyRetrievePrivateKey(forAddress address: String, completion: @escaping (String?, Error?) -> Void) {
        retrieveSecureData(forKey: "PrivateKey_\(address)") { data, error in
            if let data = data {
                let privateKey = String(data: data, encoding: .utf8)
                completion(privateKey, nil)
            } else {
                completion(nil, error)
            }
        }
    }
}

// MARK: - Security Settings Manager

@objc class SecuritySettingsManager: NSObject {
    
    @objc static let shared = SecuritySettingsManager()
    
    private let biometricAuth = BiometricAuthentication.shared
    
    @objc func getSecuritySettings() -> SecuritySettings {
        return SecuritySettings(
            isBiometricEnabled: biometricAuth.isBiometricAuthenticationEnabled(),
            biometricType: biometricAuth.getBiometricType(),
            isAutoLockEnabled: UserDefaults.standard.bool(forKey: "AutoLockEnabled"),
            autoLockTimeout: UserDefaults.standard.integer(forKey: "AutoLockTimeout"),
            requireAuthForTransactions: UserDefaults.standard.bool(forKey: "RequireAuthForTransactions"),
            requireAuthForStaking: UserDefaults.standard.bool(forKey: "RequireAuthForStaking")
        )
    }
    
    @objc func updateSecuritySettings(_ settings: SecuritySettings) {
        UserDefaults.standard.set(settings.isAutoLockEnabled, forKey: "AutoLockEnabled")
        UserDefaults.standard.set(settings.autoLockTimeout, forKey: "AutoLockTimeout")
        UserDefaults.standard.set(settings.requireAuthForTransactions, forKey: "RequireAuthForTransactions")
        UserDefaults.standard.set(settings.requireAuthForStaking, forKey: "RequireAuthForStaking")
    }
}

// MARK: - Security Settings Model

@objc class SecuritySettings: NSObject {
    @objc let isBiometricEnabled: Bool
    @objc let biometricType: BiometricType
    @objc let isAutoLockEnabled: Bool
    @objc let autoLockTimeout: Int // in seconds
    @objc let requireAuthForTransactions: Bool
    @objc let requireAuthForStaking: Bool
    
    init(isBiometricEnabled: Bool, biometricType: BiometricType, isAutoLockEnabled: Bool, autoLockTimeout: Int, requireAuthForTransactions: Bool, requireAuthForStaking: Bool) {
        self.isBiometricEnabled = isBiometricEnabled
        self.biometricType = biometricType
        self.isAutoLockEnabled = isAutoLockEnabled
        self.autoLockTimeout = autoLockTimeout
        self.requireAuthForTransactions = requireAuthForTransactions
        self.requireAuthForStaking = requireAuthForStaking
        super.init()
    }
}