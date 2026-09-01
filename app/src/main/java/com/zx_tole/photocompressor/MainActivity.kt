package com.zx_tole.photocompressor

import android.graphics.Bitmap
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.MediaStore
import android.view.MotionEvent
import android.view.View
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.chip.Chip
import com.zx_tole.photocompressor.databinding.ActivityMainBinding
import java.io.OutputStream
import java.util.concurrent.Executors

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private val executor = Executors.newSingleThreadExecutor()
    private val mainHandler = Handler(Looper.getMainLooper())

    // Image state
    private var originalBitmap: Bitmap? = null
    private var processedBitmap: Bitmap? = null
    private var currentPreviewBitmap: Bitmap? = null

    // Preset state
    private var selectedPresetId = "hdr_stylization"
    private val presetNames = arrayOf(
        "hdr_stylization", "film_effect", "old_camera",
        "vivid", "bw_classic", "vintage", "cinematic"
    )

    // Photo picker
    private val pickImage = registerForActivityResult(
        ActivityResultContracts.GetContent()
    ) { uri: Uri? ->
        uri?.let { loadPhoto(it) }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupToolbar()
        setupPresetChips()
        setupBeforeAfterSlider()
        setupListeners()
    }

    // ==================== Toolbar ====================

    private fun setupToolbar() {
        setSupportActionBar(binding.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(false)

        // Set navigation icon programmatically to ensure visibility
        binding.toolbar.setNavigationIcon(android.R.drawable.ic_menu_gallery)
        binding.toolbar.setNavigationOnClickListener {
            pickImage.launch("image/*")
        }

        // Also make the placeholder text and preview container clickable
        binding.placeholderText.setOnClickListener {
            pickImage.launch("image/*")
        }
        binding.previewContainer.setOnClickListener {
            if (originalBitmap == null) {
                pickImage.launch("image/*")
            }
        }
    }

    // ==================== Preset Chips ====================

    private fun setupPresetChips() {
        val names = resources.getStringArray(R.array.preset_names)
        for (i in names.indices) {
            val chip = Chip(this).apply {
                text = names[i]
                isCheckable = true
                setPadding(32, 16, 32, 16)
                if (i == 0) isChecked = true
            }
            binding.presetChips.addView(chip)
            chip.setOnCheckedChangeListener { _, checked ->
                if (checked) {
                    selectedPresetId = presetNames[i]
                    applyFilterIfNeeded()
                }
            }
        }
    }

    // ==================== Before/After Slider ====================

    private fun setupBeforeAfterSlider() {
        val thumb = binding.dividerThumb
        val dividerLine = binding.dividerLine
        val imageAfter = binding.imageAfter

        var startX = 0f

        thumb.setOnTouchListener { view, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> {
                    startX = event.x
                    view.performClick()
                    true
                }
                MotionEvent.ACTION_MOVE -> {
                    val deltaX = event.x - startX
                    val containerW = binding.previewContainer.width.toFloat()
                    val thumbW = thumb.width.toFloat()
                    val newLeft = (thumb.left.toFloat() + deltaX).coerceIn(
                        0f,
                        containerW - thumbW
                    )
                    thumb.layout(newLeft.toInt(), thumb.top, (newLeft + thumbW).toInt(), thumb.bottom)
                    val dividerX = (newLeft + thumbW / 2f).toInt()
                    dividerLine.layout(dividerX - 1, dividerLine.top, dividerX + 1, dividerLine.bottom)
                    // Clip the "after" image to show only the right portion
                    val clipRight = dividerX
                    imageAfter.clipBounds = android.graphics.Rect(clipRight, 0, binding.previewContainer.width, binding.previewContainer.height)
                    startX = event.x
                    true
                }
                else -> false
            }
        }
    }

    // ==================== Listeners ====================

    private fun setupListeners() {
        // Re-apply filter when sliders change (after preset is applied)
        val sliderListener = com.google.android.material.slider.Slider.OnChangeListener { _, value, _ ->
            applyFilterIfNeeded()
        }
        binding.saturationSlider.addOnChangeListener(sliderListener)
        binding.contrastSlider.addOnChangeListener(sliderListener)
        binding.brightnessSlider.addOnChangeListener(sliderListener)
    }

    // ==================== Photo Loading ====================

    private fun loadPhoto(uri: Uri) {
        showProgress(true)
        binding.placeholderText.visibility = View.GONE

        executor.execute {
            try {
                val inputStream = contentResolver.openInputStream(uri)
                val bitmap = android.graphics.BitmapFactory.decodeStream(inputStream)
                inputStream?.close()

                if (bitmap == null) {
                    mainHandler.post {
                        showProgress(false)
                        Toast.makeText(this, "Не удалось загрузить фото", Toast.LENGTH_SHORT).show()
                    }
                    return@execute
                }

                // Downsample large images for performance
                val downscaled = downsampleBitmap(bitmap, 1920)

                originalBitmap = downscaled
                currentPreviewBitmap = downscaled.copy(Bitmap.Config.ARGB_8888, true)
                processedBitmap = null

                mainHandler.post {
                    showPreview()
                    showProgress(false)
                    binding.btnApply.isEnabled = true
                }
            } catch (e: Exception) {
                mainHandler.post {
                    showProgress(false)
                    Toast.makeText(this, "Ошибка загрузки: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    private fun downsampleBitmap(bitmap: Bitmap, maxSize: Int): Bitmap {
        val (width, height) = Pair(bitmap.width, bitmap.height)
        if (width <= maxSize && height <= maxSize) return bitmap

        val scale = minOf(maxSize.toFloat() / width, maxSize.toFloat() / height)
        return Bitmap.createScaledBitmap(bitmap, (width * scale).toInt(), (height * scale).toInt(), true)
    }

    // ==================== Preview ====================

    private fun showPreview() {
        val bmp = currentPreviewBitmap ?: return

        binding.imageBefore.apply {
            setImageBitmap(bmp)
            visibility = View.VISIBLE
        }

        binding.imageAfter.apply {
            setImageBitmap(bmp)
            visibility = View.VISIBLE
            clipBounds = null // Show full image
        }

        // Reset slider to middle
        val thumbWidth = binding.dividerThumb.width.takeIf { it > 0 } ?: 48
        val containerWidth = binding.previewContainer.width.takeIf { it > 0 } ?: bmp.width
        val middlePos = (containerWidth - thumbWidth) / 2
        binding.dividerThumb.layout(middlePos, 0, middlePos + thumbWidth, binding.dividerThumb.height)
        binding.dividerLine.layout(
            middlePos + thumbWidth / 2 - 1, 0,
            middlePos + thumbWidth / 2 + 1, binding.previewContainer.height
        )
        binding.imageAfter.clipBounds = android.graphics.Rect(containerWidth, 0, containerWidth, binding.previewContainer.height)

        binding.dividerLine.visibility = View.VISIBLE
        binding.dividerThumb.visibility = View.VISIBLE
    }

    // ==================== Filter Application ====================

    private fun applyFilterIfNeeded() {
        if (currentPreviewBitmap == null) return

        // Only re-apply if sliders have moved from preset defaults
        val satChanged = binding.saturationSlider.value != 1.0f
        val contrastChanged = binding.contrastSlider.value != 1.0f
        val brightChanged = binding.brightnessSlider.value != 0.0f

        if (!satChanged && !contrastChanged && !brightChanged) return

        applyFilter()
    }

    private fun applyFilter() {
        val bmp = currentPreviewBitmap ?: return
        showProgress(true)
        binding.btnApply.isEnabled = false

        executor.execute {
            try {
                val processed = processBitmap(bmp, selectedPresetId).also { processedBmp ->
                    // Update sliders to reflect preset values (simplified: reset to defaults)
                    mainHandler.post {
                        binding.saturationSlider.value = 1.0f
                        binding.contrastSlider.value = 1.0f
                        binding.brightnessSlider.value = 0.0f
                    }
                }

                processedBitmap = processed
                currentPreviewBitmap = processed

                mainHandler.post {
                    showPreview()
                    showProgress(false)
                    binding.btnApply.isEnabled = true
                    binding.btnSave.isEnabled = true
                }
            } catch (e: Exception) {
                mainHandler.post {
                    showProgress(false)
                    binding.btnApply.isEnabled = true
                    Toast.makeText(this, "Ошибка фильтра: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    // ==================== JNI Processing ====================

    private fun processBitmap(source: Bitmap, presetId: String): Bitmap {
        val width = source.width
        val height = source.height

        // Copy pixels from source bitmap
        val pixels = IntArray(width * height)
        source.getPixels(pixels, 0, width, 0, 0, width, height)

        // Build preset JSON
        val presetJson = """{"name":"$presetId"}"""

        // Call native processing (in-place)
        val success = processBitmapInPlace(pixels, width, height, presetJson)

        if (!success) {
            throw RuntimeException("Native processing failed")
        }

        // Create result bitmap
        val result = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        result.setPixels(pixels, 0, width, 0, 0, width, height)

        return result
    }

    // ==================== Save ====================

    private fun savePhoto() {
        val bmp = processedBitmap ?: originalBitmap ?: return
        showProgress(true)

        executor.execute {
            try {
                val outputStream = contentResolver.openOutputStream(
                    createImageURI()
                )

                if (outputStream != null) {
                    bmp.compress(Bitmap.CompressFormat.JPEG, 90, outputStream)
                    outputStream.close()

                    mainHandler.post {
                        showProgress(false)
                        Toast.makeText(this, "Фото сохранено", Toast.LENGTH_SHORT).show()

                        // Notify media scanner
                        val uri = createImageURI()
                        sendBroadcast(android.content.Intent(android.content.Intent.ACTION_MEDIA_SCANNER_SCAN_FILE, uri))
                    }
                } else {
                    mainHandler.post {
                        showProgress(false)
                        Toast.makeText(this, "Ошибка сохранения", Toast.LENGTH_SHORT).show()
                    }
                }
            } catch (e: Exception) {
                mainHandler.post {
                    showProgress(false)
                    Toast.makeText(this, "Ошибка: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    private fun createImageURI(): Uri {
        val fileName = "photo_filter_${System.currentTimeMillis()}.jpg"
        return Uri.parse(MediaStore.Images.Media.insertImage(contentResolver,
            processedBitmap ?: originalBitmap, fileName, "Filtered photo"))
    }

    // ==================== UI Helpers ====================

    private fun showProgress(show: Boolean) {
        binding.progressBar.visibility = if (show) View.VISIBLE else View.GONE
    }

    // ==================== Native Methods ====================

    external fun processBitmapInPlace(
        pixels: IntArray,
        width: Int,
        height: Int,
        presetJson: String
    ): Boolean

    external fun stringFromJNI(): String

    companion object {
        init {
            System.loadLibrary("photocompressor")
        }
    }
}
