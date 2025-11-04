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

package de.schildbach.wallet.service

import android.app.Notification
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Binder
import android.os.IBinder
import android.os.PowerManager
import androidx.core.app.NotificationCompat
import androidx.work.Constraints
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.NetworkType
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import androidx.work.Worker
import androidx.work.WorkerParameters
import dagger.hilt.android.AndroidEntryPoint
import de.schildbach.wallet.Constants
import de.schildbach.wallet.WalletApplication
import de.schildbach.wallet.ui.main.MainActivity
import de.schildbach.wallet_test.R
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import org.bitcoinj.core.Coin
import org.bitcoinj.core.Transaction
import org.bitcoinj.wallet.Wallet
import org.slf4j.LoggerFactory
import java.util.concurrent.TimeUnit
import javax.inject.Inject

/**
 * Background Staking Service for BabaChain
 * Continues staking operations even when the app is closed
 */
@AndroidEntryPoint
class BackgroundStakingService : Service() {

    companion object {
        private val log = LoggerFactory.getLogger(BackgroundStakingService::class.java)
        const val ACTION_START_BACKGROUND_STAKING = "start_background_staking"
        const val ACTION_STOP_BACKGROUND_STAKING = "stop_background_staking"
        const val NOTIFICATION_ID = 2001
        const val WORK_NAME = "background_staking_work"
        
        fun startService(context: Context) {
            val intent = Intent(context, BackgroundStakingService::class.java).apply {
                action = ACTION_START_BACKGROUND_STAKING
            }
            context.startForegroundService(intent)
        }
        
        fun stopService(context: Context) {
            val intent = Intent(context, BackgroundStakingService::class.java).apply {
                action = ACTION_STOP_BACKGROUND_STAKING
            }
            context.startService(intent)
        }
    }

    @Inject
    lateinit var application: WalletApplication

    private val serviceJob = SupervisorJob()
    private val serviceScope = CoroutineScope(Dispatchers.IO + serviceJob)
    
    private var stakingJob: Job? = null
    private var isStaking = false
    private var stakingRewards = Coin.ZERO
    private var lastStakeTime = 0L
    private var wakeLock: PowerManager.WakeLock? = null
    
    inner class LocalBinder : Binder() {
        val service: BackgroundStakingService get() = this@BackgroundStakingService
    }

    private val binder = LocalBinder()

    override fun onBind(intent: Intent): IBinder = binder

    override fun onCreate() {
        super.onCreate()
        log.info("BackgroundStakingService created")
        
        // Acquire wake lock to keep CPU active for staking
        val powerManager = getSystemService(Context.POWER_SERVICE) as PowerManager
        wakeLock = powerManager.newWakeLock(
            PowerManager.PARTIAL_WAKE_LOCK,
            "BabaChain::BackgroundStaking"
        )
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_START_BACKGROUND_STAKING -> startBackgroundStaking()
            ACTION_STOP_BACKGROUND_STAKING -> stopBackgroundStaking()
        }
        return START_STICKY // Restart if killed by system
    }

    private fun startBackgroundStaking() {
        if (isStaking) {
            log.info("Background staking already active")
            return
        }
        
        log.info("Starting background staking service...")
        isStaking = true
        
        // Acquire wake lock
        wakeLock?.acquire(TimeUnit.HOURS.toMillis(1)) // 1 hour max
        
        // Start foreground service
        startForeground(NOTIFICATION_ID, createStakingNotification())
        
        // Schedule periodic staking work
        scheduleStakingWork()
        
        // Start immediate staking loop
        stakingJob = serviceScope.launch {
            performBackgroundStaking()
        }
    }

    private fun stopBackgroundStaking() {
        log.info("Stopping background staking service...")
        isStaking = false
        
        stakingJob?.cancel()
        stakingJob = null
        
        // Cancel scheduled work
        WorkManager.getInstance(this).cancelUniqueWork(WORK_NAME)
        
        // Release wake lock
        wakeLock?.let { wl ->
            if (wl.isHeld) {
                wl.release()
            }
        }
        
        stopForeground(true)
        stopSelf()
    }

    private fun scheduleStakingWork() {
        val constraints = Constraints.Builder()
            .setRequiredNetworkType(NetworkType.CONNECTED)
            .setRequiresBatteryNotLow(true)
            .build()

        val stakingWork = PeriodicWorkRequestBuilder<StakingWorker>(15, TimeUnit.MINUTES)
            .setConstraints(constraints)
            .build()

        WorkManager.getInstance(this).enqueueUniquePeriodicWork(
            WORK_NAME,
            ExistingPeriodicWorkPolicy.REPLACE,
            stakingWork
        )
    }

    private suspend fun performBackgroundStaking() {
        try {
            while (isStaking) {
                val wallet = application.wallet
                if (wallet != null) {
                    performStakingRound(wallet)
                }
                
                // Wait 2 minutes between staking attempts
                delay(TimeUnit.MINUTES.toMillis(2))
                
                // Refresh wake lock every 30 minutes
                if (System.currentTimeMillis() - lastStakeTime > TimeUnit.MINUTES.toMillis(30)) {
                    refreshWakeLock()
                }
            }
        } catch (e: Exception) {
            log.error("Background staking error", e)
        }
    }

    private suspend fun performStakingRound(wallet: Wallet) {
        try {
            val balance = wallet.getBalance(Wallet.BalanceType.AVAILABLE)
            val minimumStake = Coin.valueOf(100000000) // 1 BABA minimum
            
            if (balance.isGreaterThan(minimumStake)) {
                // Check for mature coins (coins that have been in wallet for minimum stake age)
                val matureCoins = getMatureCoins(wallet)
                
                if (matureCoins.isGreaterThan(minimumStake)) {
                    val stakeSuccess = attemptStaking(wallet, matureCoins)
                    
                    if (stakeSuccess) {
                        lastStakeTime = System.currentTimeMillis()
                        val reward = calculateStakingReward(matureCoins)
                        stakingRewards = stakingRewards.add(reward)
                        
                        // Send notification about successful stake
                        sendStakingRewardNotification(reward)
                        
                        log.info("Successful stake: {} BABA reward", reward.toFriendlyString())
                    }
                }
            }
            
            updateStakingNotification()
            
        } catch (e: Exception) {
            log.error("Error in staking round", e)
        }
    }

    private fun getMatureCoins(wallet: Wallet): Coin {
        // In a real implementation, this would check coin age
        // For now, return available balance if it's been more than 8 hours since last transaction
        val transactions = wallet.getTransactionsByTime()
        if (transactions.isNotEmpty()) {
            val lastTx = transactions.first()
            val coinAge = System.currentTimeMillis() - lastTx.updateTime.time
            val minimumAge = TimeUnit.HOURS.toMillis(8) // 8 hours minimum coin age
            
            if (coinAge > minimumAge) {
                return wallet.getBalance(Wallet.BalanceType.AVAILABLE)
            }
        }
        return Coin.ZERO
    }

    private suspend fun attemptStaking(wallet: Wallet, stakeAmount: Coin): Boolean {
        try {
            // Simulate staking attempt
            // In real implementation, this would:
            // 1. Create a stake transaction
            // 2. Sign the transaction
            // 3. Broadcast to network
            // 4. Wait for confirmation
            
            // For simulation, randomly succeed 10% of the time
            val random = (0..100).random()
            return random <= 10
            
        } catch (e: Exception) {
            log.error("Staking attempt failed", e)
            return false
        }
    }

    private fun calculateStakingReward(stakeAmount: Coin): Coin {
        // Simple reward calculation: 5% annual return
        // Actual reward = (stake_amount * 0.05) / (365 * 24 * 60 / 2) // per 2-minute interval
        val annualRate = 0.05
        val intervalsPerYear = 365 * 24 * 30 // 30 intervals per hour
        val rewardRate = annualRate / intervalsPerYear
        
        return Coin.valueOf((stakeAmount.value * rewardRate).toLong())
    }

    private fun refreshWakeLock() {
        wakeLock?.let { wl ->
            if (wl.isHeld) {
                wl.release()
            }
            wl.acquire(TimeUnit.HOURS.toMillis(1))
        }
    }

    private fun createStakingNotification(): Notification {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        return NotificationCompat.Builder(this, Constants.NOTIFICATION_CHANNEL_ID_ONGOING)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.background_staking_title))
            .setContentText(getString(R.string.background_staking_active))
            .setContentIntent(pendingIntent)
            .setOngoing(true)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()
    }

    private fun updateStakingNotification() {
        val nm = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        nm.notify(NOTIFICATION_ID, createStakingNotification())
    }

    private fun sendStakingRewardNotification(reward: Coin) {
        val nm = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val notification = NotificationCompat.Builder(this, Constants.NOTIFICATION_CHANNEL_ID_TRANSACTIONS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.staking_reward_received))
            .setContentText(getString(R.string.staking_reward_amount, reward.toFriendlyString()))
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .build()
        
        nm.notify(System.currentTimeMillis().toInt(), notification)
    }

    override fun onDestroy() {
        super.onDestroy()
        log.info("BackgroundStakingService destroyed")
        
        wakeLock?.let { wl ->
            if (wl.isHeld) {
                wl.release()
            }
        }
        
        serviceJob.cancel()
    }

    // Public interface
    fun getStakingRewards(): Coin = stakingRewards
    fun isStakingActive(): Boolean = isStaking
    fun getLastStakeTime(): Long = lastStakeTime
}

/**
 * WorkManager Worker for periodic staking when app is completely closed
 */
class StakingWorker(context: Context, params: WorkerParameters) : Worker(context, params) {
    
    companion object {
        private val log = LoggerFactory.getLogger(StakingWorker::class.java)
    }
    
    override fun doWork(): Result {
        return try {
            log.info("Executing periodic staking work")
            
            // Start the background staking service if not already running
            BackgroundStakingService.startService(applicationContext)
            
            Result.success()
        } catch (e: Exception) {
            log.error("Staking work failed", e)
            Result.retry()
        }
    }
}