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
import android.os.Bundle
import android.widget.Toast
import androidx.activity.viewModels
import androidx.lifecycle.lifecycleScope
import dagger.hilt.android.AndroidEntryPoint
import de.schildbach.wallet.ui.LockScreenActivity
import de.schildbach.wallet_test.R
import de.schildbach.wallet_test.databinding.ActivityMobileWalletSettingsBinding
import kotlinx.coroutines.launch
import org.babachain.wallet.common.services.AuthenticationManager
import javax.inject.Inject

/**
 * Settings activity for BabaChain Mobile Wallet features
 */
@AndroidEntryPoint
class MobileWalletSettingsActivity : LockScreenActivity() {
    
    companion object {
        fun createIntent(context: Context): Intent {
            return Intent(context, MobileWalletSettingsActivity::class.java)
        }
    }
    
    private lateinit var binding: ActivityMobileWalletSettingsBinding
    private val viewModel: MobileWalletSettingsViewModel by viewModels()
    
    @Inject
    lateinit var mobileWalletController: MobileWalletController
    
    @Inject
    lateinit var authenticationManager: AuthenticationManager
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMobileWalletSettingsBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        setupToolbar()
        setupViews()
        observeViewModel()
        updateUI()
    }
    
    private fun setupToolbar() {
        binding.toolbar.setNavigationOnClickListener { finish() }
        binding.toolbar.title = getString(R.string.mobile_wallet_settings_title)
    }
    
    private fun setupViews() {
        // Light Node Settings
        binding.lightNodeSwitch.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                mobileWalletController.startLightNodeService()
            } else {
                mobileWalletController.stopLightNodeService()
            }
        }
        
        // Background Staking Settings
        binding.backgroundStakingSwitch.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                if (mobileWalletController.hasMinimumStakeBalance()) {
                    mobileWalletController.startBackgroundStaking()
                } else {
                    binding.backgroundStakingSwitch.isChecked = false
                    Toast.makeText(this, R.string.insufficient_balance_for_staking, Toast.LENGTH_LONG).show()
                }
            } else {
                mobileWalletController.stopBackgroundStaking()
            }
        }
        
        // Biometric Authentication Settings
        binding.biometricSwitch.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) {
                setupBiometricAuth()
            } else {
                mobileWalletController.disableBiometricAuth()
                updateBiometricUI()
            }
        }
        
        // QR Scanner Button
        binding.qrScannerButton.setOnClickListener {
            mobileWalletController.launchQrScanner(this)
        }
        
        // Staking Status Card
        binding.stakingStatusCard.setOnClickListener {
            // TODO: Navigate to detailed staking screen
            Toast.makeText(this, R.string.feature_coming_soon, Toast.LENGTH_SHORT).show()
        }
    }
    
    private fun observeViewModel() {
        viewModel.stakingStatus.observe(this) { status ->
            updateStakingStatusUI(status)
        }
        
        viewModel.lightNodeStatus.observe(this) { status ->
            updateLightNodeStatusUI(status)
        }
    }
    
    private fun updateUI() {
        // Update switches based on current state
        binding.lightNodeSwitch.isChecked = true // Light node is always running in mobile wallet
        binding.backgroundStakingSwitch.isChecked = mobileWalletController.isBackgroundStakingActive()
        
        updateBiometricUI()
        updateStakingInfo()
    }
    
    private fun updateBiometricUI() {
        val isAvailable = mobileWalletController.isBiometricAvailable()
        val isEnabled = mobileWalletController.isBiometricEnabled()
        
        binding.biometricSwitch.isEnabled = isAvailable
        binding.biometricSwitch.isChecked = isEnabled
        
        if (!isAvailable) {
            binding.biometricDescription.text = getString(R.string.biometric_not_available)
        } else {
            val types = mobileWalletController.getAvailableBiometricTypes()
            binding.biometricDescription.text = getString(R.string.biometric_available_types, types.joinToString(", "))
        }
    }
    
    private fun updateStakingInfo() {
        val status = mobileWalletController.getStakingStatus()
        
        binding.currentBalanceText.text = getString(R.string.current_balance, status.balance.toFriendlyString())
        binding.estimatedRewardsText.text = getString(R.string.estimated_daily_rewards, status.estimatedRewards.toFriendlyString())
        
        if (!status.hasMinimumBalance) {
            binding.stakingWarning.text = getString(R.string.minimum_staking_balance_warning)
            binding.stakingWarning.visibility = android.view.View.VISIBLE
        } else {
            binding.stakingWarning.visibility = android.view.View.GONE
        }
    }
    
    private fun updateStakingStatusUI(status: MobileWalletController.StakingStatus) {
        binding.stakingStatusText.text = if (status.isActive) {
            getString(R.string.staking_active)
        } else {
            getString(R.string.staking_inactive)
        }
        
        binding.stakingStatusIndicator.setBackgroundColor(
            if (status.isActive) {
                getColor(R.color.success_green)
            } else {
                getColor(R.color.warning_orange)
            }
        )
    }
    
    private fun updateLightNodeStatusUI(status: String) {
        binding.lightNodeStatusText.text = status
    }
    
    private fun setupBiometricAuth() {
        lifecycleScope.launch {
            try {
                val pin = authenticationManager.authenticate(this@MobileWalletSettingsActivity)
                
                if (pin != null) {
                    val result = mobileWalletController.setupBiometricAuth(this@MobileWalletSettingsActivity, pin)
                    
                    if (result.success) {
                        Toast.makeText(this@MobileWalletSettingsActivity, R.string.biometric_setup_success, Toast.LENGTH_SHORT).show()
                        updateBiometricUI()
                    } else {
                        binding.biometricSwitch.isChecked = false
                        Toast.makeText(this@MobileWalletSettingsActivity, 
                            result.error ?: getString(R.string.biometric_setup_failed), 
                            Toast.LENGTH_LONG).show()
                    }
                } else {
                    binding.biometricSwitch.isChecked = false
                }
            } catch (e: Exception) {
                binding.biometricSwitch.isChecked = false
                Toast.makeText(this@MobileWalletSettingsActivity, 
                    getString(R.string.biometric_setup_error), 
                    Toast.LENGTH_LONG).show()
            }
        }
    }
    
    override fun onResume() {
        super.onResume()
        updateUI()
        viewModel.refreshStatus()
    }
}