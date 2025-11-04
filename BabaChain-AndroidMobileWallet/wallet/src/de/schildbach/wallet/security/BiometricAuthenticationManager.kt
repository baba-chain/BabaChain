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

package de.schildbach.wallet.security

import android.content.Context
import android.content.SharedPreferences
import android.os.Build
import androidx.biometric.BiometricManager
import androidx.biometric.BiometricPrompt
import androidx.core.content.ContextCompat
import androidx.fragment.app.FragmentActivity
import androidx.lifecycle.lifecycleScope
import dagger.hilt.android.qualifiers.ApplicationContext
import de.schildbach.wallet_test.R
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.launch
import org.slf4j.LoggerFactory
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec
import javax.inject.Inject
import javax.inject.Singleton
import java.security.KeyStore
import java.util.Base64

/**
 * Enhanced Biometric Authentication Manager for BabaChain
 * Supports fingerprint, face unlock, and iris scanning
 */
@Singleton
class BiometricAuthenticationManager @Inject constructor(
    @ApplicationContext private val context: Context
) {
    
    companion object {
        private val log = LoggerFactory.getLogger(BiometricAuthenticationManager::class.java)
        private const val PREFS_NAME = "biometric_prefs"
        private const val KEY_BIOMETRIC_ENABLED = "biometric_enabled"
        private const val KEY_ENCRYPTED_PIN = "encrypted_pin"
        private const val KEY_ENCRYPTION_IV = "encryption_iv"
        private const val KEYSTORE_ALIAS = "BabaChainBiometricKey"
        private const val TRANSFORMATION = "AES/GCM/NoPadding"
    }
    
    private val prefs: SharedPreferences = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    private val biometricManager = BiometricManager.from(context)
    
    data class BiometricAuthResult(
        val success: Boolean,
        val pin: String? = null,
        val error: String? = null
    )
    
    /**
     * Check if biometric authentication is available on the device
     */
    fun isBiometricAvailable(): Boolean {
        return when (biometricManager.canAuthenticate(BiometricManager.Authenticators.BIOMETRIC_STRONG)) {
            BiometricManager.BIOMETRIC_SUCCESS -> true
            else -> false
        }
    }
    
    /**
     * Check if biometric authentication is enabled by user
     */
    fun isBiometricEnabled(): Boolean {
        return prefs.getBoolean(KEY_BIOMETRIC_ENABLED, false) && isBiometricAvailable()
    }
    
    /**
     * Enable biometric authentication and store encrypted PIN
     */
    suspend fun enableBiometricAuth(activity: FragmentActivity, pin: String): BiometricAuthResult {
        if (!isBiometricAvailable()) {
            return BiometricAuthResult(false, error = context.getString(R.string.biometric_not_available))
        }
        
        return try {
            val cipher = getCipher()
            val secretKey = getOrCreateSecretKey()
            cipher.init(Cipher.ENCRYPT_MODE, secretKey)
            
            val result = CompletableDeferred<BiometricAuthResult>()
            
            val biometricPrompt = BiometricPrompt(activity, ContextCompat.getMainExecutor(context),
                object : BiometricPrompt.AuthenticationCallback() {
                    override fun onAuthenticationSucceeded(authResult: BiometricPrompt.AuthenticationResult) {
                        super.onAuthenticationSucceeded(authResult)
                        
                        activity.lifecycleScope.launch {
                            try {
                                val encryptedPin = authResult.cryptoObject?.cipher?.doFinal(pin.toByteArray())
                                val iv = authResult.cryptoObject?.cipher?.iv
                                
                                if (encryptedPin != null && iv != null) {
                                    saveBiometricData(encryptedPin, iv)
                                    prefs.edit().putBoolean(KEY_BIOMETRIC_ENABLED, true).apply()
                                    result.complete(BiometricAuthResult(true))
                                } else {
                                    result.complete(BiometricAuthResult(false, error = "Encryption failed"))
                                }
                            } catch (e: Exception) {
                                log.error("Error enabling biometric auth", e)
                                result.complete(BiometricAuthResult(false, error = e.message))
                            }
                        }
                    }
                    
                    override fun onAuthenticationError(errorCode: Int, errString: CharSequence) {
                        super.onAuthenticationError(errorCode, errString)
                        result.complete(BiometricAuthResult(false, error = errString.toString()))
                    }
                    
                    override fun onAuthenticationFailed() {
                        super.onAuthenticationFailed()
                        result.complete(BiometricAuthResult(false, error = context.getString(R.string.biometric_auth_failed)))
                    }
                })
            
            val promptInfo = BiometricPrompt.PromptInfo.Builder()
                .setTitle(context.getString(R.string.biometric_setup_title))
                .setSubtitle(context.getString(R.string.biometric_setup_subtitle))
                .setDescription(context.getString(R.string.biometric_setup_description))
                .setNegativeButtonText(context.getString(R.string.cancel))
                .setAllowedAuthenticators(BiometricManager.Authenticators.BIOMETRIC_STRONG)
                .build()
            
            biometricPrompt.authenticate(promptInfo, BiometricPrompt.CryptoObject(cipher))
            
            result.await()
        } catch (e: Exception) {
            log.error("Error setting up biometric authentication", e)
            BiometricAuthResult(false, error = e.message)
        }
    }
    
    /**
     * Authenticate using biometrics and return decrypted PIN
     */
    suspend fun authenticateWithBiometrics(activity: FragmentActivity): BiometricAuthResult {
        if (!isBiometricEnabled()) {
            return BiometricAuthResult(false, error = context.getString(R.string.biometric_not_enabled))
        }
        
        return try {
            val cipher = getCipher()
            val secretKey = getOrCreateSecretKey()
            val (encryptedPin, iv) = getBiometricData()
            
            if (encryptedPin == null || iv == null) {
                return BiometricAuthResult(false, error = context.getString(R.string.biometric_data_not_found))
            }
            
            cipher.init(Cipher.DECRYPT_MODE, secretKey, GCMParameterSpec(128, iv))
            
            val result = CompletableDeferred<BiometricAuthResult>()
            
            val biometricPrompt = BiometricPrompt(activity, ContextCompat.getMainExecutor(context),
                object : BiometricPrompt.AuthenticationCallback() {
                    override fun onAuthenticationSucceeded(authResult: BiometricPrompt.AuthenticationResult) {
                        super.onAuthenticationSucceeded(authResult)
                        
                        activity.lifecycleScope.launch {
                            try {
                                val decryptedPin = authResult.cryptoObject?.cipher?.doFinal(encryptedPin)
                                val pin = String(decryptedPin ?: byteArrayOf())
                                result.complete(BiometricAuthResult(true, pin = pin))
                            } catch (e: Exception) {
                                log.error("Error decrypting PIN", e)
                                result.complete(BiometricAuthResult(false, error = e.message))
                            }
                        }
                    }
                    
                    override fun onAuthenticationError(errorCode: Int, errString: CharSequence) {
                        super.onAuthenticationError(errorCode, errString)
                        result.complete(BiometricAuthResult(false, error = errString.toString()))
                    }
                    
                    override fun onAuthenticationFailed() {
                        super.onAuthenticationFailed()
                        result.complete(BiometricAuthResult(false, error = context.getString(R.string.biometric_auth_failed)))
                    }
                })
            
            val promptInfo = BiometricPrompt.PromptInfo.Builder()
                .setTitle(context.getString(R.string.biometric_unlock_title))
                .setSubtitle(context.getString(R.string.biometric_unlock_subtitle))
                .setDescription(context.getString(R.string.biometric_unlock_description))
                .setNegativeButtonText(context.getString(R.string.use_pin))
                .setAllowedAuthenticators(BiometricManager.Authenticators.BIOMETRIC_STRONG)
                .build()
            
            biometricPrompt.authenticate(promptInfo, BiometricPrompt.CryptoObject(cipher))
            
            result.await()
        } catch (e: Exception) {
            log.error("Error authenticating with biometrics", e)
            BiometricAuthResult(false, error = e.message)
        }
    }
    
    /**
     * Disable biometric authentication
     */
    fun disableBiometricAuth() {
        prefs.edit()
            .putBoolean(KEY_BIOMETRIC_ENABLED, false)
            .remove(KEY_ENCRYPTED_PIN)
            .remove(KEY_ENCRYPTION_IV)
            .apply()
        
        // Remove key from keystore
        try {
            val keyStore = KeyStore.getInstance("AndroidKeyStore")
            keyStore.load(null)
            keyStore.deleteEntry(KEYSTORE_ALIAS)
        } catch (e: Exception) {
            log.error("Error removing biometric key", e)
        }
    }
    
    /**
     * Get available biometric types
     */
    fun getAvailableBiometricTypes(): List<String> {
        val types = mutableListOf<String>()
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            when (biometricManager.canAuthenticate(BiometricManager.Authenticators.BIOMETRIC_STRONG)) {
                BiometricManager.BIOMETRIC_SUCCESS -> {
                    // Check specific biometric types if possible
                    types.add(context.getString(R.string.biometric_type_fingerprint))
                    
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                        // Face unlock available on Android 9+
                        types.add(context.getString(R.string.biometric_type_face))
                    }
                    
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                        // Iris scanning available on Android 10+
                        types.add(context.getString(R.string.biometric_type_iris))
                    }
                }
            }
        }
        
        return types
    }
    
    private fun getOrCreateSecretKey(): SecretKey {
        val keyStore = KeyStore.getInstance("AndroidKeyStore")
        keyStore.load(null)
        
        return if (keyStore.containsAlias(KEYSTORE_ALIAS)) {
            keyStore.getKey(KEYSTORE_ALIAS, null) as SecretKey
        } else {
            createSecretKey()
        }
    }
    
    private fun createSecretKey(): SecretKey {
        val keyGenerator = KeyGenerator.getInstance("AES", "AndroidKeyStore")
        val keyGenParameterSpec = android.security.keystore.KeyGenParameterSpec.Builder(
            KEYSTORE_ALIAS,
            android.security.keystore.KeyProperties.PURPOSE_ENCRYPT or android.security.keystore.KeyProperties.PURPOSE_DECRYPT
        )
            .setBlockModes(android.security.keystore.KeyProperties.BLOCK_MODE_GCM)
            .setEncryptionPaddings(android.security.keystore.KeyProperties.ENCRYPTION_PADDING_NONE)
            .setUserAuthenticationRequired(true)
            .apply {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                    setUserAuthenticationParameters(
                        0, // Timeout (0 means auth required for every use)
                        android.security.keystore.KeyProperties.AUTH_BIOMETRIC_STRONG
                    )
                } else {
                    @Suppress("DEPRECATION")
                    setUserAuthenticationValidityDurationSeconds(-1) // Auth required for every use
                }
            }
            .build()
        
        keyGenerator.init(keyGenParameterSpec)
        return keyGenerator.generateKey()
    }
    
    private fun getCipher(): Cipher {
        return Cipher.getInstance(TRANSFORMATION)
    }
    
    private fun saveBiometricData(encryptedPin: ByteArray, iv: ByteArray) {
        val encryptedPinBase64 = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            Base64.getEncoder().encodeToString(encryptedPin)
        } else {
            android.util.Base64.encodeToString(encryptedPin, android.util.Base64.DEFAULT)
        }
        
        val ivBase64 = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            Base64.getEncoder().encodeToString(iv)
        } else {
            android.util.Base64.encodeToString(iv, android.util.Base64.DEFAULT)
        }
        
        prefs.edit()
            .putString(KEY_ENCRYPTED_PIN, encryptedPinBase64)
            .putString(KEY_ENCRYPTION_IV, ivBase64)
            .apply()
    }
    
    private fun getBiometricData(): Pair<ByteArray?, ByteArray?> {
        val encryptedPinBase64 = prefs.getString(KEY_ENCRYPTED_PIN, null)
        val ivBase64 = prefs.getString(KEY_ENCRYPTION_IV, null)
        
        if (encryptedPinBase64 == null || ivBase64 == null) {
            return Pair(null, null)
        }
        
        val encryptedPin = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            Base64.getDecoder().decode(encryptedPinBase64)
        } else {
            android.util.Base64.decode(encryptedPinBase64, android.util.Base64.DEFAULT)
        }
        
        val iv = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            Base64.getDecoder().decode(ivBase64)
        } else {
            android.util.Base64.decode(ivBase64, android.util.Base64.DEFAULT)
        }
        
        return Pair(encryptedPin, iv)
    }
}