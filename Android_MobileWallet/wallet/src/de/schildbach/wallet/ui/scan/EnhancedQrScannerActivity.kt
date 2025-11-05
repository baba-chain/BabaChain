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

package de.schildbach.wallet.ui.scan

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.ImageFormat
import android.hardware.camera2.CameraAccessException
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CaptureRequest
import android.media.Image
import android.media.ImageReader
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.util.Size
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.google.zxing.BarcodeFormat
import com.google.zxing.BinaryBitmap
import com.google.zxing.DecodeHintType
import com.google.zxing.MultiFormatReader
import com.google.zxing.PlanarYUVLuminanceSource
import com.google.zxing.Result
import com.google.zxing.common.HybridBinarizer
import dagger.hilt.android.AndroidEntryPoint
import de.schildbach.wallet.data.PaymentIntent
import de.schildbach.wallet.ui.send.SendCoinsActivity
import de.schildbach.wallet_test.R
import de.schildbach.wallet_test.databinding.ActivityEnhancedQrScannerBinding
import org.bitcoinj.core.Address
import org.bitcoinj.core.AddressFormatException
import org.bitcoinj.core.Coin
import org.slf4j.LoggerFactory
import java.nio.ByteBuffer
import java.util.EnumMap

/**
 * Enhanced QR Code Scanner for BabaChain payments
 * Supports multiple QR code formats and provides better user experience
 */
@AndroidEntryPoint
class EnhancedQrScannerActivity : AppCompatActivity(), SurfaceHolder.Callback {

    companion object {
        private val log = LoggerFactory.getLogger(EnhancedQrScannerActivity::class.java)
        const val EXTRA_RESULT_QR_CONTENT = "qr_content"
        const val EXTRA_RESULT_PAYMENT_INTENT = "payment_intent"
        
        fun createIntent(context: Context): Intent {
            return Intent(context, EnhancedQrScannerActivity::class.java)
        }
    }

    private lateinit var binding: ActivityEnhancedQrScannerBinding
    private var cameraDevice: CameraDevice? = null
    private var captureSession: CameraCaptureSession? = null
    private var imageReader: ImageReader? = null
    private var backgroundThread: HandlerThread? = null
    private var backgroundHandler: Handler? = null
    
    private val multiFormatReader = MultiFormatReader()
    private var isScanning = true
    private var lastScanTime = 0L
    
    private val cameraPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        if (isGranted) {
            setupCamera()
        } else {
            Toast.makeText(this, R.string.camera_permission_required, Toast.LENGTH_LONG).show()
            finish()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityEnhancedQrScannerBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        setupUI()
        setupQrReader()
        checkCameraPermission()
    }

    private fun setupUI() {
        binding.toolbar.setNavigationOnClickListener { finish() }
        
        binding.flashToggle.setOnClickListener {
            toggleFlash()
        }
        
        binding.galleryButton.setOnClickListener {
            // TODO: Implement gallery image scanning
            Toast.makeText(this, R.string.feature_coming_soon, Toast.LENGTH_SHORT).show()
        }
        
        // Setup scanning overlay
        binding.scannerOverlay.setOnScanAreaChangedListener { rect ->
            // Update scanning area if needed
        }
    }

    private fun setupQrReader() {
        val hints = EnumMap<DecodeHintType, Any>(DecodeHintType::class.java)
        hints[DecodeHintType.POSSIBLE_FORMATS] = listOf(
            BarcodeFormat.QR_CODE,
            BarcodeFormat.DATA_MATRIX,
            BarcodeFormat.AZTEC
        )
        hints[DecodeHintType.TRY_HARDER] = true
        multiFormatReader.setHints(hints)
    }

    private fun checkCameraPermission() {
        when {
            ContextCompat.checkSelfPermission(
                this, Manifest.permission.CAMERA
            ) == PackageManager.PERMISSION_GRANTED -> {
                setupCamera()
            }
            else -> {
                cameraPermissionLauncher.launch(Manifest.permission.CAMERA)
            }
        }
    }

    private fun setupCamera() {
        val surfaceView = binding.cameraPreview
        surfaceView.holder.addCallback(this)
        
        startBackgroundThread()
    }

    private fun startBackgroundThread() {
        backgroundThread = HandlerThread("CameraBackground").also { it.start() }
        backgroundHandler = Handler(backgroundThread!!.looper)
    }

    private fun stopBackgroundThread() {
        backgroundThread?.quitSafely()
        try {
            backgroundThread?.join()
            backgroundThread = null
            backgroundHandler = null
        } catch (e: InterruptedException) {
            log.error("Error stopping background thread", e)
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        openCamera()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // Camera preview size changed
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        closeCamera()
    }

    private fun openCamera() {
        val cameraManager = getSystemService(Context.CAMERA_SERVICE) as CameraManager
        
        try {
            val cameraId = cameraManager.cameraIdList[0] // Use back camera
            
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) 
                != PackageManager.PERMISSION_GRANTED) {
                return
            }
            
            cameraManager.openCamera(cameraId, object : CameraDevice.StateCallback() {
                override fun onOpened(camera: CameraDevice) {
                    cameraDevice = camera
                    createCameraPreviewSession()
                }

                override fun onDisconnected(camera: CameraDevice) {
                    camera.close()
                    cameraDevice = null
                }

                override fun onError(camera: CameraDevice, error: Int) {
                    camera.close()
                    cameraDevice = null
                    log.error("Camera error: $error")
                }
            }, backgroundHandler)
            
        } catch (e: CameraAccessException) {
            log.error("Error opening camera", e)
        }
    }

    private fun createCameraPreviewSession() {
        try {
            val surface = binding.cameraPreview.holder.surface
            
            // Setup ImageReader for QR code analysis
            imageReader = ImageReader.newInstance(640, 480, ImageFormat.YUV_420_888, 1)
            imageReader?.setOnImageAvailableListener({ reader ->
                val image = reader.acquireLatestImage()
                if (image != null && isScanning) {
                    processImage(image)
                    image.close()
                }
            }, backgroundHandler)
            
            val surfaces = listOf(surface, imageReader?.surface)
            
            cameraDevice?.createCaptureSession(surfaces, object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(session: CameraCaptureSession) {
                    captureSession = session
                    startPreview()
                }

                override fun onConfigureFailed(session: CameraCaptureSession) {
                    log.error("Camera capture session configuration failed")
                }
            }, backgroundHandler)
            
        } catch (e: CameraAccessException) {
            log.error("Error creating camera preview session", e)
        }
    }

    private fun startPreview() {
        try {
            val captureRequestBuilder = cameraDevice?.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            captureRequestBuilder?.addTarget(binding.cameraPreview.holder.surface)
            captureRequestBuilder?.addTarget(imageReader?.surface!!)
            
            captureSession?.setRepeatingRequest(
                captureRequestBuilder?.build()!!,
                null,
                backgroundHandler
            )
        } catch (e: CameraAccessException) {
            log.error("Error starting camera preview", e)
        }
    }

    private fun processImage(image: Image) {
        // Prevent too frequent scanning
        val currentTime = System.currentTimeMillis()
        if (currentTime - lastScanTime < 1000) { // 1 second cooldown
            return
        }
        
        try {
            val buffer = image.planes[0].buffer
            val data = ByteArray(buffer.remaining())
            buffer.get(data)
            
            val source = PlanarYUVLuminanceSource(
                data, image.width, image.height, 0, 0, image.width, image.height, false
            )
            val bitmap = BinaryBitmap(HybridBinarizer(source))
            
            val result = multiFormatReader.decode(bitmap)
            if (result != null) {
                lastScanTime = currentTime
                handleQrCodeResult(result)
            }
        } catch (e: Exception) {
            // No QR code found or decode error, continue scanning
        }
    }

    private fun handleQrCodeResult(result: Result) {
        runOnUiThread {
            isScanning = false
            val qrContent = result.text
            
            log.info("QR Code scanned: $qrContent")
            
            // Vibrate to indicate successful scan
            // TODO: Add vibration feedback
            
            // Parse QR code content
            val paymentIntent = parseQrContent(qrContent)
            
            if (paymentIntent != null) {
                // Valid payment QR code
                val intent = Intent(this, SendCoinsActivity::class.java).apply {
                    putExtra(SendCoinsActivity.INTENT_EXTRA_PAYMENT_INTENT, paymentIntent)
                }
                startActivity(intent)
                finish()
            } else {
                // Generic QR code or unsupported format
                val resultIntent = Intent().apply {
                    putExtra(EXTRA_RESULT_QR_CONTENT, qrContent)
                }
                setResult(RESULT_OK, resultIntent)
                finish()
            }
        }
    }

    private fun parseQrContent(content: String): PaymentIntent? {
        try {
            // Handle BabaChain URI format: babachain:address?amount=x&label=y
            if (content.startsWith("babachain:", ignoreCase = true)) {
                return PaymentIntent.fromBitcoinUri(content)
            }
            
            // Handle Bitcoin URI format for compatibility
            if (content.startsWith("bitcoin:", ignoreCase = true)) {
                return PaymentIntent.fromBitcoinUri(content.replace("bitcoin:", "babachain:"))
            }
            
            // Handle plain address
            if (isValidAddress(content)) {
                return PaymentIntent.fromAddress(Address.fromString(de.schildbach.wallet.Constants.NETWORK_PARAMETERS, content), null)
            }
            
            // Handle BIP21 payment requests
            if (content.contains("amount=") || content.contains("label=")) {
                return PaymentIntent.fromBitcoinUri("babachain:$content")
            }
            
        } catch (e: Exception) {
            log.error("Error parsing QR content: $content", e)
        }
        
        return null
    }

    private fun isValidAddress(address: String): Boolean {
        return try {
            Address.fromString(de.schildbach.wallet.Constants.NETWORK_PARAMETERS, address)
            true
        } catch (e: AddressFormatException) {
            false
        }
    }

    private fun toggleFlash() {
        try {
            val captureRequestBuilder = cameraDevice?.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            captureRequestBuilder?.addTarget(binding.cameraPreview.holder.surface)
            captureRequestBuilder?.addTarget(imageReader?.surface!!)
            
            // Toggle flash
            val currentFlashMode = captureRequestBuilder?.get(CaptureRequest.FLASH_MODE)
            val newFlashMode = if (currentFlashMode == CaptureRequest.FLASH_MODE_TORCH) {
                CaptureRequest.FLASH_MODE_OFF
            } else {
                CaptureRequest.FLASH_MODE_TORCH
            }
            
            captureRequestBuilder?.set(CaptureRequest.FLASH_MODE, newFlashMode)
            
            captureSession?.setRepeatingRequest(
                captureRequestBuilder?.build()!!,
                null,
                backgroundHandler
            )
            
            // Update UI
            binding.flashToggle.isSelected = newFlashMode == CaptureRequest.FLASH_MODE_TORCH
            
        } catch (e: CameraAccessException) {
            log.error("Error toggling flash", e)
        }
    }

    private fun closeCamera() {
        captureSession?.close()
        captureSession = null
        
        cameraDevice?.close()
        cameraDevice = null
        
        imageReader?.close()
        imageReader = null
    }

    override fun onResume() {
        super.onResume()
        isScanning = true
        startBackgroundThread()
        
        if (binding.cameraPreview.holder.surface.isValid) {
            openCamera()
        }
    }

    override fun onPause() {
        super.onPause()
        isScanning = false
        closeCamera()
        stopBackgroundThread()
    }

    override fun onDestroy() {
        super.onDestroy()
        closeCamera()
        stopBackgroundThread()
    }
}