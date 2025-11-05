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
import androidx.core.app.NotificationCompat
import androidx.lifecycle.LifecycleService
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
import org.bitcoinj.core.Block
import org.bitcoinj.core.BlockChain
import org.bitcoinj.core.Coin
import org.bitcoinj.core.FilteredBlock
import org.bitcoinj.core.Peer
import org.bitcoinj.core.PeerGroup
import org.bitcoinj.core.Transaction
import org.bitcoinj.core.listeners.DownloadProgressTracker
import org.bitcoinj.store.BlockStore
import org.bitcoinj.store.SPVBlockStore
import org.bitcoinj.wallet.Wallet
import org.slf4j.LoggerFactory
import java.io.File
import java.util.Date
import java.util.concurrent.TimeUnit
import javax.inject.Inject

/**
 * Light Node Service for BabaChain Android Wallet
 * Provides SPV (Simplified Payment Verification) functionality with background staking
 */
@AndroidEntryPoint
class LightNodeService : LifecycleService() {

    companion object {
        private val log = LoggerFactory.getLogger(LightNodeService::class.java)
        const val ACTION_START_LIGHT_NODE = "start_light_node"
        const val ACTION_STOP_LIGHT_NODE = "stop_light_node"
        const val ACTION_START_STAKING = "start_staking"
        const val ACTION_STOP_STAKING = "stop_staking"
        const val NOTIFICATION_ID_LIGHT_NODE = 1001
        const val NOTIFICATION_ID_STAKING = 1002
    }

    @Inject
    lateinit var application: WalletApplication

    private val serviceJob = SupervisorJob()
    private val serviceScope = CoroutineScope(Dispatchers.IO + serviceJob)
    
    private var blockStore: BlockStore? = null
    private var blockChain: BlockChain? = null
    private var peerGroup: PeerGroup? = null
    private var wallet: Wallet? = null
    private var nm: NotificationManager? = null
    
    private var isStaking = false
    private var stakingJob: Job? = null
    private var syncProgress = 0
    
    inner class LocalBinder : Binder() {
        val service: LightNodeService get() = this@LightNodeService
    }

    private val binder = LocalBinder()

    override fun onBind(intent: Intent): IBinder {
        super.onBind(intent)
        return binder
    }

    override fun onCreate() {
        super.onCreate()
        log.info("LightNodeService created")
        nm = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        
        serviceScope.launch {
            initializeLightNode()
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        super.onStartCommand(intent, flags, startId)
        
        intent?.action?.let { action ->
            when (action) {
                ACTION_START_LIGHT_NODE -> startLightNode()
                ACTION_STOP_LIGHT_NODE -> stopLightNode()
                ACTION_START_STAKING -> startStaking()
                ACTION_STOP_STAKING -> stopStaking()
            }
        }
        
        return START_STICKY // Keep service running
    }

    private suspend fun initializeLightNode() {
        try {
            wallet = application.wallet
            if (wallet == null) {
                log.error("Wallet is null, cannot initialize light node")
                return
            }

            // Initialize block store for SPV
            val blockStoreFile = File(getDir("lightnode", MODE_PRIVATE), "spv_blockchain")
            blockStore = SPVBlockStore(Constants.NETWORK_PARAMETERS, blockStoreFile)
            
            // Initialize blockchain
            blockChain = BlockChain(Constants.NETWORK_PARAMETERS, wallet, blockStore)
            
            log.info("Light node initialized successfully")
            
        } catch (e: Exception) {
            log.error("Failed to initialize light node", e)
        }
    }

    private fun startLightNode() {
        serviceScope.launch {
            try {
                if (peerGroup != null) {
                    log.info("Light node already running")
                    return@launch
                }

                log.info("Starting light node...")
                
                // Create peer group for SPV
                peerGroup = PeerGroup(Constants.NETWORK_PARAMETERS, blockChain)
                peerGroup?.let { pg ->
                    pg.addWallet(wallet)
                    pg.setUserAgent(Constants.USER_AGENT, "1.0")
                    pg.maxConnections = 4 // Light node uses fewer connections
                    
                    // Add download progress listener
                    pg.startBlockChainDownload(object : DownloadProgressTracker() {
                        override fun progress(pct: Double, blocksLeft: Int, date: Date) {
                            syncProgress = pct.toInt()
                            updateLightNodeNotification()
                        }
                        
                        override fun doneDownload() {
                            syncProgress = 100
                            updateLightNodeNotification()
                            log.info("Light node sync completed")
                        }
                    })
                    
                    pg.startAsync()
                }
                
                startForeground(NOTIFICATION_ID_LIGHT_NODE, createLightNodeNotification())
                log.info("Light node started successfully")
                
            } catch (e: Exception) {
                log.error("Failed to start light node", e)
            }
        }
    }

    private fun stopLightNode() {
        serviceScope.launch {
            try {
                log.info("Stopping light node...")
                
                peerGroup?.let { pg ->
                    pg.removeWallet(wallet)
                    pg.stopAsync()
                }
                peerGroup = null
                
                stopForeground(true)
                log.info("Light node stopped")
                
            } catch (e: Exception) {
                log.error("Failed to stop light node", e)
            }
        }
    }

    private fun startStaking() {
        if (isStaking) {
            log.info("Staking already active")
            return
        }
        
        log.info("Starting background staking...")
        isStaking = true
        
        stakingJob = serviceScope.launch {
            try {
                while (isStaking) {
                    performStakingRound()
                    delay(TimeUnit.MINUTES.toMillis(1)) // Check every minute
                }
            } catch (e: Exception) {
                log.error("Staking error", e)
                isStaking = false
            }
        }
        
        updateStakingNotification()
    }

    private fun stopStaking() {
        log.info("Stopping background staking...")
        isStaking = false
        stakingJob?.cancel()
        stakingJob = null
        nm?.cancel(NOTIFICATION_ID_STAKING)
    }

    private suspend fun performStakingRound() {
        try {
            val wallet = this.wallet ?: return
            val balance = wallet.getBalance(Wallet.BalanceType.AVAILABLE)
            
            if (balance.isGreaterThan(Coin.COIN)) { // Minimum 1 BABA to stake
                // Simulate staking logic - in real implementation this would:
                // 1. Check if coins are mature (age > minimum stake age)
                // 2. Calculate stake weight
                // 3. Attempt to create stake transaction
                // 4. Broadcast stake transaction if successful
                
                log.debug("Performing staking round with balance: {}", balance.toFriendlyString())
                
                // For now, just update the notification to show staking is active
                updateStakingNotification()
            }
            
        } catch (e: Exception) {
            log.error("Error in staking round", e)
        }
    }

    private fun createLightNodeNotification(): Notification {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val title = getString(R.string.light_node_notification_title)
        val text = if (syncProgress < 100) {
            getString(R.string.light_node_syncing, syncProgress)
        } else {
            getString(R.string.light_node_synced)
        }
        
        return NotificationCompat.Builder(this, Constants.NOTIFICATION_CHANNEL_ID_ONGOING)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(title)
            .setContentText(text)
            .setContentIntent(pendingIntent)
            .setOngoing(true)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()
    }

    private fun updateLightNodeNotification() {
        nm?.notify(NOTIFICATION_ID_LIGHT_NODE, createLightNodeNotification())
    }

    private fun updateStakingNotification() {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val notification = NotificationCompat.Builder(this, Constants.NOTIFICATION_CHANNEL_ID_ONGOING)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.staking_notification_title))
            .setContentText(getString(R.string.staking_notification_text))
            .setContentIntent(pendingIntent)
            .setOngoing(true)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()
        
        nm?.notify(NOTIFICATION_ID_STAKING, notification)
    }

    override fun onDestroy() {
        super.onDestroy()
        log.info("LightNodeService destroyed")
        
        stopStaking()
        stopLightNode()
        
        try {
            blockStore?.close()
        } catch (e: Exception) {
            log.error("Error closing block store", e)
        }
        
        serviceJob.cancel()
    }

    // Public interface for activities to interact with the service
    fun isLightNodeRunning(): Boolean = peerGroup != null
    fun isStakingActive(): Boolean = isStaking
    fun getSyncProgress(): Int = syncProgress
}