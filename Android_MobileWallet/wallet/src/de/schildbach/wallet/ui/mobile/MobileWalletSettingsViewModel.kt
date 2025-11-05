/*
 * Copyright 2024 BabaChain Core Group.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package de.schildbach.wallet.ui.mobile

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import org.slf4j.LoggerFactory
import javax.inject.Inject

/**
 * ViewModel for Mobile Wallet Settings
 */
@HiltViewModel
class MobileWalletSettingsViewModel @Inject constructor(
    private val mobileWalletController: MobileWalletController
) : ViewModel() {
    
    companion object {
        private val log = LoggerFactory.getLogger(MobileWalletSettingsViewModel::class.java)
    }
    
    private val _stakingStatus = MutableLiveData<MobileWalletController.StakingStatus>()
    val stakingStatus: LiveData<MobileWalletController.StakingStatus> = _stakingStatus
    
    private val _lightNodeStatus = MutableLiveData<String>()
    val lightNodeStatus: LiveData<String> = _lightNodeStatus
    
    private val _biometricStatus = MutableLiveData<BiometricStatus>()
    val biometricStatus: LiveData<BiometricStatus> = _biometricStatus
    
    init {
        refreshStatus()
        startPeriodicUpdates()
    }
    
    fun refreshStatus() {
        viewModelScope.launch {
            updateStakingStatus()
            updateLightNodeStatus()
            updateBiometricStatus()
        }
    }
    
    private fun startPeriodicUpdates() {
        viewModelScope.launch {
            while (true) {
                delay(30000) // Update every 30 seconds
                updateStakingStatus()
                updateLightNodeStatus()
            }
        }
    }
    
    private fun updateStakingStatus() {
        try {
            val status = mobileWalletController.getStakingStatus()
            _stakingStatus.postValue(status)
        } catch (e: Exception) {
            log.error("Error updating staking status", e)
        }
    }
    
    private fun updateLightNodeStatus() {
        try {
            // In a real implementation, this would check the actual service status
            val status = if (isLightNodeRunning()) {
                "Connected and syncing"
            } else {
                "Disconnected"
            }
            _lightNodeStatus.postValue(status)
        } catch (e: Exception) {
            log.error("Error updating light node status", e)
        }
    }
    
    private fun updateBiometricStatus() {
        try {
            val isAvailable = mobileWalletController.isBiometricAvailable()
            val isEnabled = mobileWalletController.isBiometricEnabled()
            val types = mobileWalletController.getAvailableBiometricTypes()
            
            _biometricStatus.postValue(
                BiometricStatus(
                    isAvailable = isAvailable,
                    isEnabled = isEnabled,
                    availableTypes = types
                )
            )
        } catch (e: Exception) {
            log.error("Error updating biometric status", e)
        }
    }
    
    private fun isLightNodeRunning(): Boolean {
        // In a real implementation, this would check if the LightNodeService is running
        // For now, we'll assume it's always running in mobile wallet mode
        return true
    }
    
    data class BiometricStatus(
        val isAvailable: Boolean,
        val isEnabled: Boolean,
        val availableTypes: List<String>
    )
}