// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SYNC_WALLETSYNC_H
#define BITCOIN_SYNC_WALLETSYNC_H

#include <consensus/amount.h>
#include <pubkey.h>
#include <uint256.h>
#include <sync.h>
#include <serialize.h>

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <functional>

class CWallet;

/**
 * Device information for synchronization
 */
struct DeviceInfo {
    std::string deviceId;           // Unique device identifier
    std::string deviceName;         // Human-readable device name
    std::string platform;           // Platform (desktop, mobile, web)
    std::string version;            // App version
    int64_t lastSeen;              // Last time device was seen
    bool isActive;                 // Whether device is currently active
    bool isTrusted;                // Whether device is trusted for sync
    CPubKey syncKey;               // Device-specific sync key
    
    DeviceInfo() : lastSeen(0), isActive(false), isTrusted(false) {}
    
    SERIALIZE_METHODS(DeviceInfo, obj) {
        READWRITE(obj.deviceId, obj.deviceName, obj.platform, obj.version, 
                 obj.lastSeen, obj.isActive, obj.isTrusted, obj.syncKey);
    }
};

/**
 * Synchronization data packet
 */
struct SyncData {
    std::string dataType;           // Type of data (wallet, settings, etc.)
    std::string deviceId;           // Source device ID
    int64_t timestamp;              // When data was created
    uint256 dataHash;               // Hash of the data for integrity
    std::vector<unsigned char> encryptedData; // Encrypted sync data
    std::string signature;          // Digital signature
    
    SyncData() : timestamp(0) {}
    
    SERIALIZE_METHODS(SyncData, obj) {
        READWRITE(obj.dataType, obj.deviceId, obj.timestamp, obj.dataHash, 
                 obj.encryptedData, obj.signature);
    }
};

/**
 * Wallet synchronization state
 */
struct WalletSyncState {
    uint256 walletHash;             // Hash of wallet state
    int64_t lastSyncTime;           // Last successful sync
    int transactionCount;           // Number of transactions
    CAmount balance;                // Current balance
    std::map<std::string, std::string> settings; // Wallet settings
    std::vector<uint256> recentTxs; // Recent transaction hashes
    
    WalletSyncState() : lastSyncTime(0), transactionCount(0), balance(0) {}
    
    SERIALIZE_METHODS(WalletSyncState, obj) {
        READWRITE(obj.walletHash, obj.lastSyncTime, obj.transactionCount, 
                 obj.balance, obj.settings, obj.recentTxs);
    }
};

/**
 * Cloud backup configuration
 */
struct CloudBackupConfig {
    bool enabled;                   // Whether cloud backup is enabled
    std::string provider;           // Cloud provider (encrypted, ipfs, etc.)
    std::string encryptionKey;      // Encryption key for backups
    int64_t backupInterval;         // Backup interval in seconds
    int64_t lastBackupTime;         // Last backup timestamp
    int maxBackupVersions;          // Maximum backup versions to keep
    bool autoBackup;                // Whether to backup automatically
    
    CloudBackupConfig() : enabled(false), backupInterval(3600), lastBackupTime(0), 
                         maxBackupVersions(10), autoBackup(true) {}
    
    SERIALIZE_METHODS(CloudBackupConfig, obj) {
        READWRITE(obj.enabled, obj.provider, obj.encryptionKey, obj.backupInterval, 
                 obj.lastBackupTime, obj.maxBackupVersions, obj.autoBackup);
    }
};

/**
 * Staking coordination information
 */
struct StakingCoordination {
    std::string primaryDevice;      // Primary staking device
    bool stakingEnabled;            // Whether staking is enabled
    CAmount totalStaked;            // Total amount staked across devices
    std::map<std::string, bool> deviceStakingStatus; // Per-device staking status
    int64_t lastCoordinationUpdate; // Last coordination update
    
    StakingCoordination() : stakingEnabled(false), totalStaked(0), lastCoordinationUpdate(0) {}
    
    SERIALIZE_METHODS(StakingCoordination, obj) {
        READWRITE(obj.primaryDevice, obj.stakingEnabled, obj.totalStaked, 
                 obj.deviceStakingStatus, obj.lastCoordinationUpdate);
    }
};

/**
 * Cross-platform wallet synchronization system
 */
class CWalletSync
{
private:
    mutable RecursiveMutex cs_sync;
    
    // Device management
    std::map<std::string, DeviceInfo> registeredDevices GUARDED_BY(cs_sync);
    std::string currentDeviceId GUARDED_BY(cs_sync);
    DeviceInfo currentDevice GUARDED_BY(cs_sync);
    
    // Synchronization state
    WalletSyncState localState GUARDED_BY(cs_sync);
    std::map<std::string, WalletSyncState> deviceStates GUARDED_BY(cs_sync);
    std::vector<SyncData> pendingSyncData GUARDED_BY(cs_sync);
    
    // Cloud backup
    CloudBackupConfig backupConfig GUARDED_BY(cs_sync);
    std::vector<std::string> backupVersions GUARDED_BY(cs_sync);
    
    // Staking coordination
    StakingCoordination stakingCoord GUARDED_BY(cs_sync);
    
    // Configuration
    std::atomic<bool> syncEnabled{true};
    std::atomic<bool> autoSyncEnabled{true};
    std::atomic<int64_t> syncInterval{300}; // 5 minutes
    std::atomic<bool> encryptionEnabled{true};
    
    // Callbacks
    std::vector<std::function<void(const std::string&)>> deviceConnectedCallbacks;
    std::vector<std::function<void(const std::string&)>> deviceDisconnectedCallbacks;
    std::vector<std::function<void(const SyncData&)>> syncCompletedCallbacks;
    std::vector<std::function<void(const std::string&)>> backupCompletedCallbacks;
    
    CWallet* wallet;
    
public:
    explicit CWalletSync(CWallet* wallet);
    ~CWalletSync();
    
    /** Device management */
    bool RegisterDevice(const DeviceInfo& device);
    bool UnregisterDevice(const std::string& deviceId);
    std::vector<DeviceInfo> GetRegisteredDevices() const;
    DeviceInfo GetDeviceInfo(const std::string& deviceId) const;
    bool IsDeviceTrusted(const std::string& deviceId) const;
    void SetDeviceTrusted(const std::string& deviceId, bool trusted);
    
    /** Synchronization */
    bool SyncWithDevice(const std::string& deviceId);
    bool SyncWithAllDevices();
    bool ProcessIncomingSyncData(const SyncData& syncData);
    std::vector<SyncData> GetPendingSyncData() const;
    void ClearPendingSyncData();
    
    /** Wallet state management */
    void UpdateLocalState();
    WalletSyncState GetLocalState() const;
    WalletSyncState GetDeviceState(const std::string& deviceId) const;
    bool IsWalletInSync() const;
    std::vector<std::string> GetOutOfSyncDevices() const;
    
    /** Cloud backup */
    bool CreateCloudBackup();
    bool RestoreFromCloudBackup(const std::string& backupVersion = "");
    std::vector<std::string> GetAvailableBackups() const;
    bool DeleteCloudBackup(const std::string& backupVersion);
    void SetCloudBackupConfig(const CloudBackupConfig& config);
    CloudBackupConfig GetCloudBackupConfig() const;
    
    /** Staking coordination */
    bool CoordinateStaking();
    bool SetPrimaryStakingDevice(const std::string& deviceId);
    std::string GetPrimaryStakingDevice() const;
    bool EnableStakingOnDevice(const std::string& deviceId, bool enable);
    StakingCoordination GetStakingCoordination() const;
    
    /** Device switching */
    bool SwitchToDevice(const std::string& deviceId);
    bool PrepareForDeviceSwitch();
    bool CompleteDeviceSwitch(const std::string& fromDeviceId);
    
    /** Conflict resolution */
    bool ResolveConflicts();
    std::vector<std::string> DetectConflicts() const;
    bool MergeWalletStates(const std::vector<WalletSyncState>& states);
    
    /** Encryption and security */
    std::vector<unsigned char> EncryptSyncData(const std::vector<unsigned char>& data) const;
    std::vector<unsigned char> DecryptSyncData(const std::vector<unsigned char>& encryptedData) const;
    bool VerifyDataIntegrity(const SyncData& syncData) const;
    std::string SignSyncData(const SyncData& syncData) const;
    
    /** Configuration */
    void SetSyncEnabled(bool enabled);
    void SetAutoSyncEnabled(bool enabled);
    void SetSyncInterval(int64_t interval);
    void SetEncryptionEnabled(bool enabled);
    
    bool IsSyncEnabled() const { return syncEnabled.load(); }
    bool IsAutoSyncEnabled() const { return autoSyncEnabled.load(); }
    int64_t GetSyncInterval() const { return syncInterval.load(); }
    bool IsEncryptionEnabled() const { return encryptionEnabled.load(); }
    
    /** Callbacks */
    void RegisterDeviceConnectedCallback(const std::function<void(const std::string&)>& callback);
    void RegisterDeviceDisconnectedCallback(const std::function<void(const std::string&)>& callback);
    void RegisterSyncCompletedCallback(const std::function<void(const SyncData&)>& callback);
    void RegisterBackupCompletedCallback(const std::function<void(const std::string&)>& callback);
    
    /** Statistics and monitoring */
    std::map<std::string, int64_t> GetSyncStatistics() const;
    int64_t GetLastSyncTime() const;
    std::string GetSyncStatus() const;
    
    /** Maintenance */
    void CleanupOldSyncData();
    void OptimizeSyncData();
    bool ValidateWalletConsistency() const;
    
private:
    /** Helper methods */
    std::string GenerateDeviceId() const;
    SyncData CreateSyncData(const std::string& dataType, const std::vector<unsigned char>& data) const;
    bool SendSyncData(const std::string& deviceId, const SyncData& syncData);
    bool ReceiveSyncData(const std::string& deviceId, SyncData& syncData);
    
    /** State serialization */
    std::vector<unsigned char> SerializeWalletState() const;
    bool DeserializeWalletState(const std::vector<unsigned char>& data);
    std::vector<unsigned char> SerializeSettings() const;
    bool DeserializeSettings(const std::vector<unsigned char>& data);
    
    /** Backup operations */
    std::string CreateBackupData() const;
    bool RestoreBackupData(const std::string& backupData);
    std::string GenerateBackupVersion() const;
    
    /** Conflict resolution helpers */
    WalletSyncState MergeStates(const WalletSyncState& state1, const WalletSyncState& state2) const;
    bool IsStateNewer(const WalletSyncState& state1, const WalletSyncState& state2) const;
    
    /** Callback triggers */
    void TriggerDeviceConnectedCallbacks(const std::string& deviceId);
    void TriggerDeviceDisconnectedCallbacks(const std::string& deviceId);
    void TriggerSyncCompletedCallbacks(const SyncData& syncData);
    void TriggerBackupCompletedCallbacks(const std::string& backupVersion);
    
    /** Utility methods */
    uint256 CalculateStateHash(const WalletSyncState& state) const;
    bool IsDeviceOnline(const std::string& deviceId) const;
    int64_t GetCurrentTimestamp() const;
};

#endif // BITCOIN_SYNC_WALLETSYNC_H