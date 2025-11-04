// Copyright (c) 2025 The BabaChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sync/walletsync.h>

#include <wallet/wallet.h>
#include <logging.h>
#include <util/time.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <crypto/sha256.h>
#include <random.h>

#include <algorithm>
#include <sstream>

CWalletSync::CWalletSync(CWallet* wallet) : wallet(wallet)
{
    // Generate unique device ID for this instance
    currentDeviceId = GenerateDeviceId();
    
    // Initialize current device info
    currentDevice.deviceId = currentDeviceId;
    currentDevice.deviceName = gArgs.GetArg("-devicename", "BabaChain Wallet");
    currentDevice.platform = "desktop"; // Could be detected dynamically
    currentDevice.version = "1.0.0";
    currentDevice.lastSeen = GetTime();
    currentDevice.isActive = true;
    currentDevice.isTrusted = true;
    
    // Register this device
    registeredDevices[currentDeviceId] = currentDevice;
    
    LogPrint(BCLog::WALLET, "CWalletSync: Initialized wallet sync for device %s\n", currentDeviceId.c_str());
}

CWalletSync::~CWalletSync()
{
}

bool CWalletSync::RegisterDevice(const DeviceInfo& device)
{
    LOCK(cs_sync);
    
    // Check if device already exists
    if (registeredDevices.find(device.deviceId) != registeredDevices.end()) {
        return false;
    }
    
    registeredDevices[device.deviceId] = device;
    
    LogPrint(BCLog::WALLET, "CWalletSync::RegisterDevice: Registered device %s (%s)\n",
             device.deviceId.c_str(), device.deviceName.c_str());
    
    TriggerDeviceConnectedCallbacks(device.deviceId);
    
    return true;
}

bool CWalletSync::UnregisterDevice(const std::string& deviceId)
{
    LOCK(cs_sync);
    
    auto it = registeredDevices.find(deviceId);
    if (it == registeredDevices.end()) {
        return false;
    }
    
    // Don't allow unregistering current device
    if (deviceId == currentDeviceId) {
        return false;
    }
    
    registeredDevices.erase(it);
    deviceStates.erase(deviceId);
    
    LogPrint(BCLog::WALLET, "CWalletSync::UnregisterDevice: Unregistered device %s\n", deviceId.c_str());
    
    TriggerDeviceDisconnectedCallbacks(deviceId);
    
    return true;
}

std::vector<DeviceInfo> CWalletSync::GetRegisteredDevices() const
{
    LOCK(cs_sync);
    
    std::vector<DeviceInfo> devices;
    for (const auto& [deviceId, device] : registeredDevices) {
        devices.push_back(device);
    }
    
    return devices;
}

DeviceInfo CWalletSync::GetDeviceInfo(const std::string& deviceId) const
{
    LOCK(cs_sync);
    
    auto it = registeredDevices.find(deviceId);
    if (it != registeredDevices.end()) {
        return it->second;
    }
    
    return DeviceInfo(); // Return empty info if not found
}

bool CWalletSync::IsDeviceTrusted(const std::string& deviceId) const
{
    LOCK(cs_sync);
    
    auto it = registeredDevices.find(deviceId);
    return (it != registeredDevices.end()) && it->second.isTrusted;
}

void CWalletSync::SetDeviceTrusted(const std::string& deviceId, bool trusted)
{
    LOCK(cs_sync);
    
    auto it = registeredDevices.find(deviceId);
    if (it != registeredDevices.end()) {
        it->second.isTrusted = trusted;
        
        LogPrint(BCLog::WALLET, "CWalletSync::SetDeviceTrusted: Device %s trust set to %s\n",
                 deviceId.c_str(), trusted ? "true" : "false");
    }
}

bool CWalletSync::SyncWithDevice(const std::string& deviceId)
{
    if (!syncEnabled.load()) {
        return false;
    }
    
    LOCK(cs_sync);
    
    if (!IsDeviceTrusted(deviceId)) {
        LogPrint(BCLog::WALLET, "CWalletSync::SyncWithDevice: Device %s is not trusted\n", deviceId.c_str());
        return false;
    }
    
    // Update local state before syncing
    UpdateLocalState();
    
    // Create sync data
    std::vector<unsigned char> walletData = SerializeWalletState();
    SyncData syncData = CreateSyncData("wallet", walletData);
    
    // Send sync data to device
    bool success = SendSyncData(deviceId, syncData);
    
    if (success) {
        LogPrint(BCLog::WALLET, "CWalletSync::SyncWithDevice: Successfully synced with device %s\n", deviceId.c_str());
        TriggerSyncCompletedCallbacks(syncData);
    } else {
        LogPrint(BCLog::WALLET, "CWalletSync::SyncWithDevice: Failed to sync with device %s\n", deviceId.c_str());
    }
    
    return success;
}

bool CWalletSync::SyncWithAllDevices()
{
    if (!syncEnabled.load()) {
        return false;
    }
    
    LOCK(cs_sync);
    
    bool allSuccess = true;
    
    for (const auto& [deviceId, device] : registeredDevices) {
        if (deviceId != currentDeviceId && device.isTrusted && device.isActive) {
            if (!SyncWithDevice(deviceId)) {
                allSuccess = false;
            }
        }
    }
    
    return allSuccess;
}

bool CWalletSync::ProcessIncomingSyncData(const SyncData& syncData)
{
    if (!syncEnabled.load()) {
        return false;
    }
    
    LOCK(cs_sync);
    
    // Verify data integrity
    if (!VerifyDataIntegrity(syncData)) {
        LogPrint(BCLog::WALLET, "CWalletSync::ProcessIncomingSyncData: Data integrity check failed\n");
        return false;
    }
    
    // Check if source device is trusted
    if (!IsDeviceTrusted(syncData.deviceId)) {
        LogPrint(BCLog::WALLET, "CWalletSync::ProcessIncomingSyncData: Source device %s is not trusted\n",
                 syncData.deviceId.c_str());
        return false;
    }
    
    // Decrypt data
    std::vector<unsigned char> decryptedData = DecryptSyncData(syncData.encryptedData);
    if (decryptedData.empty()) {
        LogPrint(BCLog::WALLET, "CWalletSync::ProcessIncomingSyncData: Failed to decrypt sync data\n");
        return false;
    }
    
    // Process based on data type
    bool success = false;
    if (syncData.dataType == "wallet") {
        success = DeserializeWalletState(decryptedData);
    } else if (syncData.dataType == "settings") {
        success = DeserializeSettings(decryptedData);
    }
    
    if (success) {
        LogPrint(BCLog::WALLET, "CWalletSync::ProcessIncomingSyncData: Successfully processed %s data from device %s\n",
                 syncData.dataType.c_str(), syncData.deviceId.c_str());
        
        TriggerSyncCompletedCallbacks(syncData);
    }
    
    return success;
}

void CWalletSync::UpdateLocalState()
{
    LOCK(cs_sync);
    
    if (!wallet) {
        return;
    }
    
    LOCK(wallet->cs_wallet);
    
    // Update wallet state
    localState.lastSyncTime = GetTime();
    localState.transactionCount = wallet->mapWallet.size();
    
    // Get current balance
    interfaces::WalletBalances balances = wallet->GetBalances();
    localState.balance = balances.balance;
    
    // Get recent transactions
    localState.recentTxs.clear();
    int count = 0;
    for (const auto& [txid, wtx] : wallet->mapWallet) {
        if (count >= 10) break; // Keep last 10 transactions
        localState.recentTxs.push_back(txid);
        count++;
    }
    
    // Calculate wallet hash
    localState.walletHash = CalculateStateHash(localState);
    
    LogPrint(BCLog::WALLET, "CWalletSync::UpdateLocalState: Updated local state (balance: %s, txs: %d)\n",
             FormatMoney(localState.balance), localState.transactionCount);
}

WalletSyncState CWalletSync::GetLocalState() const
{
    LOCK(cs_sync);
    return localState;
}

WalletSyncState CWalletSync::GetDeviceState(const std::string& deviceId) const
{
    LOCK(cs_sync);
    
    auto it = deviceStates.find(deviceId);
    if (it != deviceStates.end()) {
        return it->second;
    }
    
    return WalletSyncState(); // Return empty state if not found
}

bool CWalletSync::IsWalletInSync() const
{
    LOCK(cs_sync);
    
    for (const auto& [deviceId, device] : registeredDevices) {
        if (deviceId != currentDeviceId && device.isTrusted && device.isActive) {
            auto stateIt = deviceStates.find(deviceId);
            if (stateIt != deviceStates.end()) {
                if (stateIt->second.walletHash != localState.walletHash) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

std::vector<std::string> CWalletSync::GetOutOfSyncDevices() const
{
    LOCK(cs_sync);
    
    std::vector<std::string> outOfSync;
    
    for (const auto& [deviceId, device] : registeredDevices) {
        if (deviceId != currentDeviceId && device.isTrusted && device.isActive) {
            auto stateIt = deviceStates.find(deviceId);
            if (stateIt != deviceStates.end()) {
                if (stateIt->second.walletHash != localState.walletHash) {
                    outOfSync.push_back(deviceId);
                }
            } else {
                outOfSync.push_back(deviceId); // No state info = out of sync
            }
        }
    }
    
    return outOfSync;
}

bool CWalletSync::CreateCloudBackup()
{
    if (!backupConfig.enabled) {
        return false;
    }
    
    LOCK(cs_sync);
    
    try {
        // Create backup data
        std::string backupData = CreateBackupData();
        
        // Generate backup version
        std::string version = GenerateBackupVersion();
        
        // Encrypt backup data
        std::vector<unsigned char> dataBytes(backupData.begin(), backupData.end());
        std::vector<unsigned char> encryptedData = EncryptSyncData(dataBytes);
        
        // Store backup (this would typically upload to cloud storage)
        // For now, we just add it to our local backup versions list
        backupVersions.push_back(version);
        
        // Keep only the configured number of backup versions
        while (backupVersions.size() > static_cast<size_t>(backupConfig.maxBackupVersions)) {
            backupVersions.erase(backupVersions.begin());
        }
        
        backupConfig.lastBackupTime = GetTime();
        
        LogPrint(BCLog::WALLET, "CWalletSync::CreateCloudBackup: Created backup version %s\n", version.c_str());
        
        TriggerBackupCompletedCallbacks(version);
        
        return true;
        
    } catch (const std::exception& e) {
        LogPrintf("CWalletSync::CreateCloudBackup: Exception: %s\n", e.what());
        return false;
    }
}

bool CWalletSync::RestoreFromCloudBackup(const std::string& backupVersion)
{
    LOCK(cs_sync);
    
    // If no version specified, use the latest
    std::string version = backupVersion;
    if (version.empty() && !backupVersions.empty()) {
        version = backupVersions.back();
    }
    
    if (version.empty()) {
        LogPrint(BCLog::WALLET, "CWalletSync::RestoreFromCloudBackup: No backup version available\n");
        return false;
    }
    
    try {
        // This would typically download from cloud storage
        // For now, we simulate having the backup data
        
        LogPrint(BCLog::WALLET, "CWalletSync::RestoreFromCloudBackup: Restored from backup version %s\n", version.c_str());
        
        return true;
        
    } catch (const std::exception& e) {
        LogPrintf("CWalletSync::RestoreFromCloudBackup: Exception: %s\n", e.what());
        return false;
    }
}

std::vector<std::string> CWalletSync::GetAvailableBackups() const
{
    LOCK(cs_sync);
    return backupVersions;
}

bool CWalletSync::CoordinateStaking()
{
    LOCK(cs_sync);
    
    // Update staking coordination info
    stakingCoord.lastCoordinationUpdate = GetTime();
    
    if (wallet) {
        LOCK(wallet->cs_wallet);
        stakingCoord.stakingEnabled = wallet->IsStakingEnabled();
        stakingCoord.totalStaked = wallet->GetStakedBalance();
    }
    
    // Set this device as primary if none is set
    if (stakingCoord.primaryDevice.empty()) {
        stakingCoord.primaryDevice = currentDeviceId;
    }
    
    // Update device staking status
    stakingCoord.deviceStakingStatus[currentDeviceId] = stakingCoord.stakingEnabled;
    
    LogPrint(BCLog::WALLET, "CWalletSync::CoordinateStaking: Coordinated staking (primary: %s, enabled: %s)\n",
             stakingCoord.primaryDevice.c_str(), stakingCoord.stakingEnabled ? "true" : "false");
    
    return true;
}

bool CWalletSync::SetPrimaryStakingDevice(const std::string& deviceId)
{
    LOCK(cs_sync);
    
    if (registeredDevices.find(deviceId) == registeredDevices.end()) {
        return false;
    }
    
    stakingCoord.primaryDevice = deviceId;
    stakingCoord.lastCoordinationUpdate = GetTime();
    
    LogPrint(BCLog::WALLET, "CWalletSync::SetPrimaryStakingDevice: Set primary staking device to %s\n", deviceId.c_str());
    
    return true;
}

std::string CWalletSync::GetPrimaryStakingDevice() const
{
    LOCK(cs_sync);
    return stakingCoord.primaryDevice;
}

bool CWalletSync::SwitchToDevice(const std::string& deviceId)
{
    LOCK(cs_sync);
    
    if (!IsDeviceTrusted(deviceId)) {
        return false;
    }
    
    // Prepare current device for switch
    if (!PrepareForDeviceSwitch()) {
        return false;
    }
    
    // Sync with target device
    if (!SyncWithDevice(deviceId)) {
        return false;
    }
    
    LogPrint(BCLog::WALLET, "CWalletSync::SwitchToDevice: Switched to device %s\n", deviceId.c_str());
    
    return true;
}

bool CWalletSync::PrepareForDeviceSwitch()
{
    LOCK(cs_sync);
    
    // Update local state
    UpdateLocalState();
    
    // Create backup
    if (backupConfig.enabled) {
        CreateCloudBackup();
    }
    
    // Sync with all devices
    SyncWithAllDevices();
    
    LogPrint(BCLog::WALLET, "CWalletSync::PrepareForDeviceSwitch: Prepared for device switch\n");
    
    return true;
}

void CWalletSync::SetSyncEnabled(bool enabled)
{
    syncEnabled.store(enabled);
    LogPrint(BCLog::WALLET, "CWalletSync: Sync %s\n", enabled ? "enabled" : "disabled");
}

void CWalletSync::SetAutoSyncEnabled(bool enabled)
{
    autoSyncEnabled.store(enabled);
    LogPrint(BCLog::WALLET, "CWalletSync: Auto-sync %s\n", enabled ? "enabled" : "disabled");
}

void CWalletSync::SetSyncInterval(int64_t interval)
{
    syncInterval.store(std::max(int64_t(60), interval)); // Minimum 1 minute
    LogPrint(BCLog::WALLET, "CWalletSync: Sync interval set to %d seconds\n", interval);
}

std::map<std::string, int64_t> CWalletSync::GetSyncStatistics() const
{
    LOCK(cs_sync);
    
    std::map<std::string, int64_t> stats;
    
    stats["registered_devices"] = registeredDevices.size();
    stats["trusted_devices"] = 0;
    stats["active_devices"] = 0;
    stats["last_sync_time"] = localState.lastSyncTime;
    stats["pending_sync_data"] = pendingSyncData.size();
    stats["backup_versions"] = backupVersions.size();
    
    for (const auto& [deviceId, device] : registeredDevices) {
        if (device.isTrusted) stats["trusted_devices"]++;
        if (device.isActive) stats["active_devices"]++;
    }
    
    return stats;
}

std::string CWalletSync::GetSyncStatus() const
{
    LOCK(cs_sync);
    
    if (!syncEnabled.load()) {
        return "disabled";
    }
    
    if (IsWalletInSync()) {
        return "synced";
    }
    
    std::vector<std::string> outOfSync = GetOutOfSyncDevices();
    if (!outOfSync.empty()) {
        return "out_of_sync";
    }
    
    return "syncing";
}

// Private helper methods implementation

std::string CWalletSync::GenerateDeviceId() const
{
    // Generate a unique device ID based on system characteristics
    std::string seed = gArgs.GetArg("-deviceid", "");
    if (seed.empty()) {
        // Use system time and random data
        seed = std::to_string(GetTime()) + std::to_string(GetRand(UINT64_MAX));
    }
    
    CSHA256 hasher;
    hasher.Write((const unsigned char*)seed.data(), seed.size());
    uint256 hash;
    hasher.Finalize(hash.begin());
    
    return hash.GetHex().substr(0, 16); // Use first 16 characters
}

SyncData CWalletSync::CreateSyncData(const std::string& dataType, const std::vector<unsigned char>& data) const
{
    SyncData syncData;
    syncData.dataType = dataType;
    syncData.deviceId = currentDeviceId;
    syncData.timestamp = GetTime();
    
    // Encrypt data if encryption is enabled
    if (encryptionEnabled.load()) {
        syncData.encryptedData = EncryptSyncData(data);
    } else {
        syncData.encryptedData = data;
    }
    
    // Calculate data hash
    CSHA256 hasher;
    hasher.Write(syncData.encryptedData.data(), syncData.encryptedData.size());
    hasher.Finalize(syncData.dataHash.begin());
    
    // Sign data
    syncData.signature = SignSyncData(syncData);
    
    return syncData;
}

bool CWalletSync::SendSyncData(const std::string& deviceId, const SyncData& syncData)
{
    // This would implement actual network communication
    // For now, we simulate successful sending
    
    LogPrint(BCLog::WALLET, "CWalletSync::SendSyncData: Sent %s data to device %s\n",
             syncData.dataType.c_str(), deviceId.c_str());
    
    return true;
}

std::vector<unsigned char> CWalletSync::SerializeWalletState() const
{
    // Serialize the current wallet state
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << localState;
    
    return std::vector<unsigned char>(ss.begin(), ss.end());
}

bool CWalletSync::DeserializeWalletState(const std::vector<unsigned char>& data)
{
    try {
        CDataStream ss(data, SER_NETWORK, PROTOCOL_VERSION);
        WalletSyncState newState;
        ss >> newState;
        
        // Merge with local state if newer
        if (IsStateNewer(newState, localState)) {
            localState = MergeStates(localState, newState);
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        LogPrintf("CWalletSync::DeserializeWalletState: Exception: %s\n", e.what());
        return false;
    }
}

std::vector<unsigned char> CWalletSync::EncryptSyncData(const std::vector<unsigned char>& data) const
{
    // This would implement actual encryption
    // For now, we just return the data as-is
    return data;
}

std::vector<unsigned char> CWalletSync::DecryptSyncData(const std::vector<unsigned char>& encryptedData) const
{
    // This would implement actual decryption
    // For now, we just return the data as-is
    return encryptedData;
}

bool CWalletSync::VerifyDataIntegrity(const SyncData& syncData) const
{
    // Calculate hash of the data
    CSHA256 hasher;
    hasher.Write(syncData.encryptedData.data(), syncData.encryptedData.size());
    uint256 calculatedHash;
    hasher.Finalize(calculatedHash.begin());
    
    return calculatedHash == syncData.dataHash;
}

std::string CWalletSync::SignSyncData(const SyncData& syncData) const
{
    // This would implement actual digital signing
    // For now, we return a placeholder signature
    return "signature_placeholder";
}

uint256 CWalletSync::CalculateStateHash(const WalletSyncState& state) const
{
    CSHA256 hasher;
    
    // Hash key components of the state
    hasher.Write((const unsigned char*)&state.lastSyncTime, sizeof(state.lastSyncTime));
    hasher.Write((const unsigned char*)&state.transactionCount, sizeof(state.transactionCount));
    hasher.Write((const unsigned char*)&state.balance, sizeof(state.balance));
    
    for (const auto& txid : state.recentTxs) {
        hasher.Write(txid.begin(), txid.size());
    }
    
    uint256 hash;
    hasher.Finalize(hash.begin());
    
    return hash;
}

WalletSyncState CWalletSync::MergeStates(const WalletSyncState& state1, const WalletSyncState& state2) const
{
    // Simple merge strategy - use the newer state
    return IsStateNewer(state1, state2) ? state1 : state2;
}

bool CWalletSync::IsStateNewer(const WalletSyncState& state1, const WalletSyncState& state2) const
{
    return state1.lastSyncTime > state2.lastSyncTime;
}

std::string CWalletSync::CreateBackupData() const
{
    // Create a comprehensive backup of wallet data
    std::ostringstream backup;
    
    backup << "BabaChain Wallet Backup v1.0\n";
    backup << "Timestamp: " << GetTime() << "\n";
    backup << "Device: " << currentDeviceId << "\n";
    backup << "Balance: " << localState.balance << "\n";
    backup << "Transactions: " << localState.transactionCount << "\n";
    
    // Add more wallet data as needed
    
    return backup.str();
}

std::string CWalletSync::GenerateBackupVersion() const
{
    return std::to_string(GetTime()) + "_" + currentDeviceId.substr(0, 8);
}

void CWalletSync::TriggerDeviceConnectedCallbacks(const std::string& deviceId)
{
    for (const auto& callback : deviceConnectedCallbacks) {
        try {
            callback(deviceId);
        } catch (const std::exception& e) {
            LogPrintf("CWalletSync: Exception in device connected callback: %s\n", e.what());
        }
    }
}

void CWalletSync::TriggerSyncCompletedCallbacks(const SyncData& syncData)
{
    for (const auto& callback : syncCompletedCallbacks) {
        try {
            callback(syncData);
        } catch (const std::exception& e) {
            LogPrintf("CWalletSync: Exception in sync completed callback: %s\n", e.what());
        }
    }
}

void CWalletSync::TriggerBackupCompletedCallbacks(const std::string& backupVersion)
{
    for (const auto& callback : backupCompletedCallbacks) {
        try {
            callback(backupVersion);
        } catch (const std::exception& e) {
            LogPrintf("CWalletSync: Exception in backup completed callback: %s\n", e.what());
        }
    }
}