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

package de.schildbach.wallet.ui.notifications

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.os.Build
import androidx.core.app.NotificationCompat
import com.google.firebase.messaging.FirebaseMessagingService
import com.google.firebase.messaging.RemoteMessage
import dagger.hilt.android.AndroidEntryPoint
import de.schildbach.wallet.Constants
import de.schildbach.wallet.WalletApplication
import de.schildbach.wallet.ui.main.MainActivity
import de.schildbach.wallet_test.R
import org.bitcoinj.core.Coin
import org.slf4j.LoggerFactory
import javax.inject.Inject

/**
 * Enhanced Push Messaging Service for BabaChain staking notifications
 */
@AndroidEntryPoint
class StakingNotificationService : FirebaseMessagingService() {

    companion object {
        private val log = LoggerFactory.getLogger(StakingNotificationService::class.java)
        
        // Notification channels
        const val CHANNEL_STAKING_REWARDS = "staking_rewards"
        const val CHANNEL_NETWORK_EVENTS = "network_events"
        const val CHANNEL_STAKING_STATUS = "staking_status"
        
        // Notification types
        const val TYPE_STAKING_REWARD = "staking_reward"
        const val TYPE_NETWORK_EVENT = "network_event"
        const val TYPE_STAKING_STATUS = "staking_status"
        const val TYPE_VALIDATOR_EVENT = "validator_event"
        
        fun createNotificationChannels(context: Context) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                val notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
                
                // Staking Rewards Channel
                val stakingRewardsChannel = NotificationChannel(
                    CHANNEL_STAKING_REWARDS,
                    context.getString(R.string.notification_channel_staking_rewards),
                    NotificationManager.IMPORTANCE_HIGH
                ).apply {
                    description = context.getString(R.string.notification_channel_staking_rewards_desc)
                    enableVibration(true)
                    enableLights(true)
                }
                
                // Network Events Channel
                val networkEventsChannel = NotificationChannel(
                    CHANNEL_NETWORK_EVENTS,
                    context.getString(R.string.notification_channel_network_events),
                    NotificationManager.IMPORTANCE_DEFAULT
                ).apply {
                    description = context.getString(R.string.notification_channel_network_events_desc)
                }
                
                // Staking Status Channel
                val stakingStatusChannel = NotificationChannel(
                    CHANNEL_STAKING_STATUS,
                    context.getString(R.string.notification_channel_staking_status),
                    NotificationManager.IMPORTANCE_LOW
                ).apply {
                    description = context.getString(R.string.notification_channel_staking_status_desc)
                }
                
                notificationManager.createNotificationChannels(listOf(
                    stakingRewardsChannel,
                    networkEventsChannel,
                    stakingStatusChannel
                ))
            }
        }
    }

    @Inject
    lateinit var application: WalletApplication

    override fun onCreate() {
        super.onCreate()
        createNotificationChannels(this)
    }

    override fun onMessageReceived(remoteMessage: RemoteMessage) {
        super.onMessageReceived(remoteMessage)
        
        log.info("Received push message: ${remoteMessage.from}")
        
        val data = remoteMessage.data
        val type = data["type"] ?: return
        
        when (type) {
            TYPE_STAKING_REWARD -> handleStakingReward(data)
            TYPE_NETWORK_EVENT -> handleNetworkEvent(data)
            TYPE_STAKING_STATUS -> handleStakingStatus(data)
            TYPE_VALIDATOR_EVENT -> handleValidatorEvent(data)
        }
    }

    private fun handleStakingReward(data: Map<String, String>) {
        val amount = data["amount"]?.toLongOrNull() ?: return
        val txHash = data["tx_hash"] ?: ""
        
        val coin = Coin.valueOf(amount)
        showStakingRewardNotification(coin, txHash)
    }

    private fun handleNetworkEvent(data: Map<String, String>) {
        val eventType = data["event_type"] ?: return
        val message = data["message"] ?: return
        
        when (eventType) {
            "network_upgrade" -> showNetworkUpgradeNotification(message)
            "maintenance" -> showMaintenanceNotification(message)
            "security_alert" -> showSecurityAlertNotification(message)
            "validator_selection" -> showValidatorSelectionNotification(message)
        }
    }

    private fun handleStakingStatus(data: Map<String, String>) {
        val status = data["status"] ?: return
        val message = data["message"] ?: return
        
        when (status) {
            "staking_started" -> showStakingStartedNotification()
            "staking_stopped" -> showStakingStoppedNotification(message)
            "coins_matured" -> showCoinsMaturedNotification(message)
            "low_balance" -> showLowBalanceNotification()
        }
    }

    private fun handleValidatorEvent(data: Map<String, String>) {
        val eventType = data["event_type"] ?: return
        val message = data["message"] ?: return
        
        when (eventType) {
            "selected_as_validator" -> showValidatorSelectedNotification()
            "validator_reward" -> {
                val amount = data["amount"]?.toLongOrNull() ?: return
                showValidatorRewardNotification(Coin.valueOf(amount))
            }
            "validator_penalty" -> showValidatorPenaltyNotification(message)
        }
    }

    private fun showStakingRewardNotification(reward: Coin, txHash: String) {
        val intent = Intent(this, MainActivity::class.java).apply {
            putExtra("show_transaction", txHash)
        }
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val notification = NotificationCompat.Builder(this, CHANNEL_STAKING_REWARDS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.staking_reward_received))
            .setContentText(getString(R.string.staking_reward_amount, reward.toFriendlyString()))
            .setStyle(NotificationCompat.BigTextStyle()
                .bigText(getString(R.string.staking_reward_details, 
                    reward.toFriendlyString(), 
                    txHash.take(8) + "...")))
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .setCategory(NotificationCompat.CATEGORY_MESSAGE)
            .build()
        
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(System.currentTimeMillis().toInt(), notification)
    }

    private fun showNetworkUpgradeNotification(message: String) {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val notification = NotificationCompat.Builder(this, CHANNEL_NETWORK_EVENTS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.network_upgrade_title))
            .setContentText(message)
            .setStyle(NotificationCompat.BigTextStyle().bigText(message))
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .build()
        
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(1001, notification)
    }

    private fun showMaintenanceNotification(message: String) {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val notification = NotificationCompat.Builder(this, CHANNEL_NETWORK_EVENTS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.network_maintenance_title))
            .setContentText(message)
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_DEFAULT)
            .build()
        
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(1002, notification)
    }

    private fun showSecurityAlertNotification(message: String) {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val notification = NotificationCompat.Builder(this, CHANNEL_NETWORK_EVENTS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.security_alert_title))
            .setContentText(message)
            .setStyle(NotificationCompat.BigTextStyle().bigText(message))
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .setCategory(NotificationCompat.CATEGORY_ALARM)
            .build()
        
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(1003, notification)
    }

    private fun showValidatorSelectionNotification(message: String) {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val notification = NotificationCompat.Builder(this, CHANNEL_NETWORK_EVENTS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.validator_selection_title))
            .setContentText(message)
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_DEFAULT)
            .build()
        
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(1004, notification)
    }

    private fun showStakingStartedNotification() {
        val notification = NotificationCompat.Builder(this, CHANNEL_STAKING_STATUS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.staking_started_title))
            .setContentText(getString(R.string.staking_started_message))
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()
        
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(2001, notification)
    }

    private fun showStakingStoppedNotification(reason: String) {
        val notification = NotificationCompat.Builder(this, CHANNEL_STAKING_STATUS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.staking_stopped_title))
            .setContentText(reason)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()
        
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(2002, notification)
    }

    private fun showCoinsMaturedNotification(message: String) {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val notification = NotificationCompat.Builder(this, CHANNEL_STAKING_STATUS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.coins_matured_title))
            .setContentText(message)
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_DEFAULT)
            .build()
        
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(2003, notification)
    }

    private fun showLowBalanceNotification() {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val notification = NotificationCompat.Builder(this, CHANNEL_STAKING_STATUS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.low_balance_staking_title))
            .setContentText(getString(R.string.low_balance_staking_message))
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_DEFAULT)
            .build()
        
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(2004, notification)
    }

    private fun showValidatorSelectedNotification() {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val notification = NotificationCompat.Builder(this, CHANNEL_STAKING_REWARDS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.validator_selected_title))
            .setContentText(getString(R.string.validator_selected_message))
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .build()
        
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(3001, notification)
    }

    private fun showValidatorRewardNotification(reward: Coin) {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val notification = NotificationCompat.Builder(this, CHANNEL_STAKING_REWARDS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.validator_reward_title))
            .setContentText(getString(R.string.validator_reward_amount, reward.toFriendlyString()))
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .build()
        
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(3002, notification)
    }

    private fun showValidatorPenaltyNotification(reason: String) {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        )
        
        val notification = NotificationCompat.Builder(this, CHANNEL_NETWORK_EVENTS)
            .setSmallIcon(R.drawable.ic_dash_d_white)
            .setContentTitle(getString(R.string.validator_penalty_title))
            .setContentText(reason)
            .setStyle(NotificationCompat.BigTextStyle().bigText(reason))
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .setCategory(NotificationCompat.CATEGORY_ALARM)
            .build()
        
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.notify(3003, notification)
    }

    override fun onNewToken(token: String) {
        super.onNewToken(token)
        log.info("New FCM token: $token")
        
        // Send token to server for push notifications
        // In a real implementation, this would send the token to your backend
        sendTokenToServer(token)
    }

    private fun sendTokenToServer(token: String) {
        // TODO: Implement server communication to register device for push notifications
        log.info("Sending FCM token to server: $token")
    }
}