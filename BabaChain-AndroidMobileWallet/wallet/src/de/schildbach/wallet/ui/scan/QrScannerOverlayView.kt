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

import android.animation.ValueAnimator
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.Rect
import android.graphics.RectF
import android.util.AttributeSet
import android.view.View
import androidx.core.content.ContextCompat
import de.schildbach.wallet_test.R
import kotlin.math.min

/**
 * Custom overlay view for QR code scanner with animated scanning line
 */
class QrScannerOverlayView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private val overlayPaint = Paint().apply {
        color = Color.parseColor("#80000000") // Semi-transparent black
        style = Paint.Style.FILL
    }
    
    private val framePaint = Paint().apply {
        color = ContextCompat.getColor(context, R.color.dash_blue)
        style = Paint.Style.STROKE
        strokeWidth = 8f
        isAntiAlias = true
    }
    
    private val cornerPaint = Paint().apply {
        color = ContextCompat.getColor(context, R.color.dash_blue)
        style = Paint.Style.STROKE
        strokeWidth = 12f
        strokeCap = Paint.Cap.ROUND
        isAntiAlias = true
    }
    
    private val scanLinePaint = Paint().apply {
        color = ContextCompat.getColor(context, R.color.dash_blue)
        style = Paint.Style.FILL
        isAntiAlias = true
    }
    
    private val textPaint = Paint().apply {
        color = Color.WHITE
        textSize = 48f
        textAlign = Paint.Align.CENTER
        isAntiAlias = true
    }
    
    private var scanRect = Rect()
    private var scanLineY = 0f
    private var scanLineAnimator: ValueAnimator? = null
    private var onScanAreaChangedListener: ((Rect) -> Unit)? = null
    
    private val cornerLength = 60f
    private val cornerRadius = 16f
    
    init {
        setupScanLineAnimation()
    }
    
    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        calculateScanRect(w, h)
        onScanAreaChangedListener?.invoke(scanRect)
    }
    
    private fun calculateScanRect(width: Int, height: Int) {
        val size = min(width, height) * 0.7f
        val left = (width - size) / 2
        val top = (height - size) / 2
        
        scanRect.set(
            left.toInt(),
            top.toInt(),
            (left + size).toInt(),
            (top + size).toInt()
        )
    }
    
    private fun setupScanLineAnimation() {
        scanLineAnimator = ValueAnimator.ofFloat(0f, 1f).apply {
            duration = 2000
            repeatCount = ValueAnimator.INFINITE
            repeatMode = ValueAnimator.REVERSE
            
            addUpdateListener { animator ->
                val progress = animator.animatedValue as Float
                scanLineY = scanRect.top + (scanRect.height() * progress)
                invalidate()
            }
        }
        scanLineAnimator?.start()
    }
    
    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        
        drawOverlay(canvas)
        drawScanFrame(canvas)
        drawCorners(canvas)
        drawScanLine(canvas)
        drawInstructions(canvas)
    }
    
    private fun drawOverlay(canvas: Canvas) {
        // Draw semi-transparent overlay everywhere except scan area
        val path = Path().apply {
            addRect(0f, 0f, width.toFloat(), height.toFloat(), Path.Direction.CW)
            addRect(scanRect.left.toFloat(), scanRect.top.toFloat(), 
                   scanRect.right.toFloat(), scanRect.bottom.toFloat(), Path.Direction.CCW)
        }
        canvas.drawPath(path, overlayPaint)
    }
    
    private fun drawScanFrame(canvas: Canvas) {
        // Draw the main scanning frame
        val rectF = RectF(scanRect)
        canvas.drawRoundRect(rectF, cornerRadius, cornerRadius, framePaint)
    }
    
    private fun drawCorners(canvas: Canvas) {
        // Draw corner indicators
        val left = scanRect.left.toFloat()
        val top = scanRect.top.toFloat()
        val right = scanRect.right.toFloat()
        val bottom = scanRect.bottom.toFloat()
        
        // Top-left corner
        canvas.drawLine(left, top + cornerLength, left, top + cornerRadius, cornerPaint)
        canvas.drawLine(left + cornerRadius, top, left + cornerLength, top, cornerPaint)
        
        // Top-right corner
        canvas.drawLine(right, top + cornerLength, right, top + cornerRadius, cornerPaint)
        canvas.drawLine(right - cornerRadius, top, right - cornerLength, top, cornerPaint)
        
        // Bottom-left corner
        canvas.drawLine(left, bottom - cornerLength, left, bottom - cornerRadius, cornerPaint)
        canvas.drawLine(left + cornerRadius, bottom, left + cornerLength, bottom, cornerPaint)
        
        // Bottom-right corner
        canvas.drawLine(right, bottom - cornerLength, right, bottom - cornerRadius, cornerPaint)
        canvas.drawLine(right - cornerRadius, bottom, right - cornerLength, bottom, cornerPaint)
    }
    
    private fun drawScanLine(canvas: Canvas) {
        // Draw animated scanning line
        val gradient = android.graphics.LinearGradient(
            scanRect.left.toFloat(), scanLineY - 2,
            scanRect.right.toFloat(), scanLineY + 2,
            intArrayOf(Color.TRANSPARENT, scanLinePaint.color, Color.TRANSPARENT),
            floatArrayOf(0f, 0.5f, 1f),
            android.graphics.Shader.TileMode.CLAMP
        )
        
        scanLinePaint.shader = gradient
        
        canvas.drawRect(
            scanRect.left.toFloat(),
            scanLineY - 2,
            scanRect.right.toFloat(),
            scanLineY + 2,
            scanLinePaint
        )
    }
    
    private fun drawInstructions(canvas: Canvas) {
        // Draw instruction text
        val instructionText = context.getString(R.string.qr_scanner_instruction)
        val textY = scanRect.bottom + 100f
        
        canvas.drawText(instructionText, width / 2f, textY, textPaint)
    }
    
    fun setOnScanAreaChangedListener(listener: (Rect) -> Unit) {
        onScanAreaChangedListener = listener
    }
    
    fun getScanRect(): Rect = scanRect
    
    override fun onDetachedFromWindow() {
        super.onDetachedFromWindow()
        scanLineAnimator?.cancel()
    }
}