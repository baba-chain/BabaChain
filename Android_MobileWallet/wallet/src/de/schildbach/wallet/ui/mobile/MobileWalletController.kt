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

import android.content.Context
import android.content.Intent
import androidx.fragment.app.FragmentActivity
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.lifecycleScope
import dagger.hilt.android.qualifiers.ApplicationContext
import de.schildbach.wallet.WalletApplication
import de.schildbach.wallet.security.BiometricAuthenticationManager
import de.schildbach.wallet.service.BackgroundStakingService
import de.schildbach.wallet.service.LightNodeService
import de.schildbach.wallet.ui.notifications.StakingNotificationService
import de.schildbach.wallet.ui.scan.EnhancedQrScannerActivity
import kotlinx.coroutines.launch
import org.bitcoinj.core.Coin
import org.slf4j.LoggerFactory
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Main controller for BabaChain Mobile Wallet features
 * Coordinates light node, background staking, notifications, and biometric security
 */
@Singleton
class MobileWalletController @Inject constructor(
    @ApplicationContext private val context: Context,
    private val application: WalletApplication,
    private val biometricManager: BiometricAuthenticationManager
) {
    
    companion object {
        private val log = LoggerFactory.getLogger(MobileWalletController::class.java)
    }
    
    private var isInitialized = false
    
    /**
     * Initialize the mobile wallet features
     */
    fun initialize() {
        if (isInitialized) {
            log.info("Mobile wallet already initialized")
            return
        }
        
        log.info("Initializing BabaChain Mobile Wallet features...")
        
        // Initialize notification channels
        StakingNotificationService.createNotificationChannels(context)
        
        // Start light node service
        startLightNodeService()
        
        // Check if auto-staking is enabled and start if needed
        if (isAutoStakingEnabled()) {
            startBackgroundStaking()
        }
        
        isInitialized = true
        log.info("Mobile wallet initialization completed")
    }
    
    /**
     * Start the light node service for SPV functionality
     */
    fun startLightNodeService() {
        log.info("Starting light node service...")
        val intent = Intent(context, LightNodeService::class.java).apply {
            action = LightNodeService.ACTION_START_LIGHT_NODE
        }
        context.startForegroundService(intent)
    }
    
    /**
     * Stop the light node service
     */
    fun stopLightNodeService() {
        log.info("Stopping light node service...")
        val intent = Intent(context, LightNodeService::class.java).apply {
            action = LightNodeService.ACTION_STOP_LIGHT_NODE
        }
        context.startService(intent)
    }
    
    /**
     * Start background staking
     */
    fun startBackgroundStaking() {
        log.info("Starting background staking...")
        BackgroundStakingService.startService(context)
        setAutoStakingEnabled(true)
    }
    
    /**
     * Stop background staking
     */
    fun stopBackgroundStaking() {
        log.info("Stopping background staking...")
        BackgroundStakingService.stopService(context)
        setAutoStakingEnabled(false)
    }
    
    /**
     * Check if background staking is enabled
     */
    fun isBackgroundStakingActive(): Boolean {
        // In a real implementation, this would check the service status
        return isAutoStakingEnabled()
    }
    
    /**
     * Launch the enhanced QR scanner
     */
    fun launchQrScanner(activity: FragmentActivity) {
        val intent = EnhancedQrScannerActivity.createIntent(activity)
        activity.startActivity(intent)
    }
    
    /**
     * Setup biometric authentication
     */
    suspend fun setupBiometricAuth(activity: FragmentActivity, pin: String): BiometricAuthenticationManager.BiometricAuthResult {
        return biometricManager.enableBiometricAuth(activity, pin)
    }
    
    /**
     * Authenticate with biometrics
     */
    suspend fun authenticateWithBiometrics(activity: FragmentActivity): BiometricAuthenticationManager.BiometricAuthResult {
        return biometricManager.authenticateWithBiometrics(activity)
    }
    
    /**
     * Check if biometric authentication is available
     */
    fun isBiometricAvailable(): Boolean {
        return biometricManager.isBiometricAvailable()
    }
    
    /**
     * Check if biometric authentication is enabled
     */
    fun isBiometricEnabled(): Boolean {
        return biometricManager.isBiometricEnabled()
    }
    
    /**
     * Disable biometric authentication
     */
    fun disableBiometricAuth() {
        biometricManager.disableBiometricAuth()
    }
    
    /**
     * Get available biometric types
     */
    fun getAvailableBiometricTypes(): List<String> {
        return biometricManager.getAvailableBiometricTypes()
    }
    
    /**
     * Get wallet balance for staking calculations
     */
    fun getWalletBalance(): Coin {
        return application.wallet?.getBalance(org.bitcoinj.wallet.Wallet.BalanceType.AVAILABLE) ?: Coin.ZERO
    }
    
    /**
     * Check if wallet has sufficient balance for staking
     */
    fun hasMinimumStakeBalance(): Boolean {
        val balance = getWalletBalance()
        val minimumStake = Coin.valueOf(100000000) // 1 BABA minimum
        return balance.isGreaterThan(minimumStake)
    }
    
    /**
     * Get staking status information
     */
    fun getStakingStatus(): StakingStatus {
        val balance = getWalletBalance()
        val isStaking = isBackgroundStakingActive()
        val hasMinBalance = hasMinimumStakeBalance()
        
        return StakingStatus(
            isActive = isStaking,
            balance = balance,
            hasMinimumBalance = hasMinBalance,
            estimatedRewards = calculateEstimatedRewards(balance)
        )
    }
    
    /**
     * Calculate estimated staking rewards
     */
    private fun calculateEstimatedRewards(balance: Coin): Coin {
        // Simple calculation: 5% annual return
        if (balance.isLessThan(Coin.valueOf(100000000))) {
            return Coin.ZERO
        }
        
        val annualRate = 0.05
        val dailyRate = annualRate / 365
        val dailyReward = balance.value * dailyRate
        
        return Coin.valueOf(dailyReward.toLong())
    }
    
    /**
     * Auto-start staking when coins mature
     */
    fun checkAndAutoStartStaking(lifecycleOwner: LifecycleOwner) {
        if (!isAutoStakingEnabled()) {
            return
        }
        
        lifecycleOwner.lifecycleScope.launch {
            val wallet = application.wallet ?: return@launch
            
            // Check if we have mature coins
            val balance = wallet.getBalance(org.bitcoinj.wallet.Wallet.BalanceType.AVAILABLE)
            if (balance.isGreaterThan(Coin.valueOf(100000000))) {
                // Check coin age - in real implementation, this would check actual coin age
                val transactions = wallet.getTransactionsByTime()
                if (transactions.isNotEmpty()) {
                    val lastTx = transactions.first()
                    val coinAge = System.currentTimeMillis() - lastTx.updateTime.time
                    val minimumAge = 8 * 60 * 60 * 1000L // 8 hours
                    
                    if (coinAge > minimumAge && !isBackgroundStakingActive()) {
                        log.info("Coins have matured, auto-starting staking")
                        startBackgroundStaking()
                    }
                }
            }
        }
    }
    
    private fun isAutoStakingEnabled(): Boolean {
        val prefs = context.getSharedPreferences("mobile_wallet_prefs", Context.MODE_PRIVATE)
        return prefs.getBoolean("auto_staking_enabled", false)
    }
    
    private fun setAutoStakingEnabled(enabled: Boolean) {
        val prefs = context.getSharedPreferences("mobile_wallet_prefs", Context.MODE_PRIVATE)
        prefs.edit().putBoolean("auto_staking_enabled", enabled).apply()
    }
    
    /**
     * Data class for staking status
     */
    data class StakingStatus(
        val isActive: Boolean,
        val balance: Coin,
        val hasMinimumBalance: Boolean,
        val estimatedRewards: Coin
    )
}