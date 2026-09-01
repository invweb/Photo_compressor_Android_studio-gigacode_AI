#include "image_filters.h"
#include "color_spaces.h"
#include "kernels.h"

#include <opencv2/imgproc.hpp>
#include <cmath>
#include <algorithm>

// ============================================================================
// LUT building from control points (Catmull-Rom spline interpolation)
// ============================================================================

std::array<uint8_t, 256> FilterEngine::buildLUT(const std::array<uint8_t, 10>& controlPoints) {
    std::array<uint8_t, 256> lut{};

    // For each output value, find the corresponding input via spline
    for (int out = 0; out < 256; out++) {
        // Map output to parameter t in [0, 1]
        float t = static_cast<float>(out) / 255.0f;

        // Find which segment we're in
        int segment = static_cast<int>(t * 9.0f);
        segment = std::max(0, std::min(6, segment)); // Clamp to valid range

        // Local t within segment
        float localT = t * 9.0f - static_cast<float>(segment);

        // Get 4 control points for Catmull-Rom
        int p0 = std::max(0, segment - 1);
        int p1 = segment;
        int p2 = std::min(9, segment + 1);
        int p3 = std::min(9, segment + 2);

        float cp[4] = {
            static_cast<float>(controlPoints[p0]),
            static_cast<float>(controlPoints[p1]),
            static_cast<float>(controlPoints[p2]),
            static_cast<float>(controlPoints[p3])
        };

        // Catmull-Rom spline
        float result = 0.5f * ((2.0f * cp[1]) +
                               (-cp[0] + cp[2]) * localT +
                               (2.0f * cp[0] - 5.0f * cp[1] + 4.0f * cp[2] - cp[3]) * localT * localT +
                               (-cp[0] + 3.0f * cp[1] - 3.0f * cp[2] + cp[3]) * localT * localT * localT);

        lut[static_cast<uint8_t>(out)] = static_cast<uint8_t>(std::clamp(std::round(result), 0.0f, 255.0f));
    }

    return lut;
}

// ============================================================================
// Filter pipeline application
// ============================================================================

void FilterEngine::apply(cv::Mat& image, const FilterParams& params) {
    if (image.empty()) return;

    // Ensure 3-channel BGR
    cv::Mat work;
    if (image.channels() == 4) {
        cv::cvtColor(image, work, cv::COLOR_BGRA2BGR);
    } else if (image.channels() == 3) {
        work = image.clone();
    } else {
        return; // Unsupported channel count
    }

    // 1. Brightness adjustment
    applyBrightness(work, params.brightness);

    // 2. Contrast adjustment
    applyContrast(work, params.contrast);

    // 3. Color correction matrix
    applyColorMatrix(work, params.colorMatrix);

    // 4. Saturation via HSV
    applySaturation(work, params.saturation);

    // 5. Tone curves via LUT
    applyCurves(work, params.curveR, params.curveG, params.curveB);

    // 6. Gaussian blur (if specified)
    if (params.blurRadius > 0.0f) {
        gaussianBlurKernel(work, params.blurRadius);
    }

    // 7. Unsharp mask sharpening (if specified)
    if (params.sharpness > 0.0f) {
        unsharpMask(work, params.sharpness);
    }

    // 8. Vignette effect
    if (params.vignetteStrength > 0.0f) {
        applyVignette(work, params.vignetteStrength);
    }

    // Write back to original image
    image = work;
}

// ============================================================================
// Individual filter operations
// ============================================================================

void FilterEngine::applyBrightness(cv::Mat& image, float brightness) {
    if (brightness == 0.0f) return;
    int value = static_cast<int>(std::round(brightness * 255.0f));
    cv::add(image, cv::Scalar(value, value, value), image);
}

void FilterEngine::applyContrast(cv::Mat& image, float contrast) {
    if (contrast == 1.0f) return;
    // OpenCV's convertScaleAbs: dst = src * contrast + 0
    cv::convertScaleAbs(image, image, contrast, 0);
}

void FilterEngine::applyColorMatrix(cv::Mat& image, const std::array<float, 9>& matrix) {
    // Check if matrix is identity
    bool isIdentity = true;
    for (int i = 0; i < 9; i++) {
        float expected = (i / 3 == i % 3) ? 1.0f : 0.0f;
        if (std::abs(matrix[i] - expected) > 0.001f) {
            isIdentity = false;
            break;
        }
    }
    if (isIdentity) return;

    cv::Mat mat(3, 3, CV_32F, const_cast<float*>(matrix.data()));
    cv::transform(image, image, mat);
}

void FilterEngine::applySaturation(cv::Mat& image, float saturation) {
    if (saturation == 1.0f) return;

    cv::Mat hsv;
    bgrToHsv(image, hsv);

    // Split channels
    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);

    // Scale saturation channel
    channels[1] = channels[1] * saturation;

    // Clamp to valid range
    cv::min(channels[1], cv::Scalar(255.0f), channels[1]);
    cv::max(channels[1], cv::Scalar(0.0f), channels[1]);

    // Merge back and convert
    cv::merge(channels, hsv);
    hsvToBgr(hsv, image);
}

void FilterEngine::applyCurves(cv::Mat& image,
                                const std::array<uint8_t, 256>& curveR,
                                const std::array<uint8_t, 256>& curveG,
                                const std::array<uint8_t, 256>& curveB) {
    // Check if all curves are identity
    bool isIdentity = true;
    for (int i = 0; i < 256; i++) {
        if (curveR[i] != static_cast<uint8_t>(i) ||
            curveG[i] != static_cast<uint8_t>(i) ||
            curveB[i] != static_cast<uint8_t>(i)) {
            isIdentity = false;
            break;
        }
    }
    if (isIdentity) return;

    // OpenCV LUT expects 256 entries as CV_8U 1-channel mat
    cv::Mat lutR(1, 256, CV_8U, const_cast<uint8_t*>(curveR.data()));
    cv::Mat lutG(1, 256, CV_8U, const_cast<uint8_t*>(curveG.data()));
    cv::Mat lutB(1, 256, CV_8U, const_cast<uint8_t*>(curveB.data()));

    // Split, apply LUT, merge (BGR order)
    std::vector<cv::Mat> bgr;
    cv::split(image, bgr);

    cv::LUT(bgr[0], lutB, bgr[0]); // Blue
    cv::LUT(bgr[1], lutG, bgr[1]); // Green
    cv::LUT(bgr[2], lutR, bgr[2]); // Red

    cv::merge(bgr, image);
}

void FilterEngine::applyVignette(cv::Mat& image, float strength) {
    if (strength <= 0.0f) return;

    int rows = image.rows;
    int cols = image.cols;
    cv::Point center(cols / 2, rows / 2);
    float maxRadius = std::sqrt(static_cast<float>(center.x * center.x + center.y * center.y));

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            float dist = std::sqrt(static_cast<float>((x - center.x) * (x - center.x) +
                                                       (y - center.y) * (y - center.y)));
            float factor = 1.0f - (dist / maxRadius) * strength;
            factor = std::max(0.0f, std::min(1.0f, factor));

            cv::Vec3b& pixel = image.at<cv::Vec3b>(y, x);
            pixel[0] = static_cast<uchar>(pixel[0] * factor);
            pixel[1] = static_cast<uchar>(pixel[1] * factor);
            pixel[2] = static_cast<uchar>(pixel[2] * factor);
        }
    }
}

// ============================================================================
// Preset definitions
// ============================================================================

FilterParams FilterEngine::presetHDR() {
    FilterParams p;
    p.name = "hdr_stylization";

    // Boost contrast and saturation
    p.contrast = 1.5f;
    p.saturation = 1.6f;
    p.sharpness = 0.6f;
    p.vignetteStrength = 0.15f;

    // S-curve for dramatic look
    std::array<uint8_t, 10> sCurve = {0, 16, 48, 80, 128, 160, 192, 224, 240, 255};
    p.curveR = buildLUT(sCurve);
    p.curveG = buildLUT(sCurve);
    p.curveB = buildLUT(sCurve);

    return p;
}

FilterParams FilterEngine::presetFilm() {
    FilterParams p;
    p.name = "film_effect";

    // Warm color shift
    p.colorMatrix = {
        1.1f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.05f, 0.9f
    };
    p.saturation = 0.85f;
    p.contrast = 1.2f;
    p.vignetteStrength = 0.4f;

    // Cross-process curve
    std::array<uint8_t, 10> crossCurve = {0, 8, 32, 64, 112, 144, 176, 208, 240, 255};
    p.curveR = buildLUT(crossCurve);
    p.curveG = buildLUT(crossCurve);
    p.curveB = buildLUT(crossCurve);

    return p;
}

FilterParams FilterEngine::presetOldCamera() {
    FilterParams p;
    p.name = "old_camera";

    p.saturation = 0.6f;
    p.brightness = -0.05f;
    p.blurRadius = 0.8f;
    p.vignetteStrength = 0.5f;

    // Warm sepia-ish curve
    std::array<uint8_t, 10> oldCurve = {0, 4, 24, 56, 104, 144, 184, 216, 240, 255};
    p.curveR = buildLUT(oldCurve);
    p.curveG = buildLUT(oldCurve);
    p.curveB = buildLUT(oldCurve);

    return p;
}

FilterParams FilterEngine::presetVivid() {
    FilterParams p;
    p.name = "vivid";

    // Maximum saturation and contrast
    p.saturation = 1.8f;
    p.contrast = 1.4f;
    p.sharpness = 0.3f;

    // Midtone boost curve
    std::array<uint8_t, 10> vividCurve = {0, 8, 32, 72, 120, 152, 192, 216, 240, 255};
    p.curveR = buildLUT(vividCurve);
    p.curveG = buildLUT(vividCurve);
    p.curveB = buildLUT(vividCurve);

    return p;
}

FilterParams FilterEngine::presetBWClassic() {
    FilterParams p;
    p.name = "bw_classic";

    // Desaturate completely
    p.saturation = 0.0f;
    p.contrast = 1.3f;

    // Dramatic S-curve for B&W
    std::array<uint8_t, 10> bwCurve = {0, 4, 24, 56, 112, 144, 184, 216, 240, 255};
    p.curveR = buildLUT(bwCurve);
    p.curveG = buildLUT(bwCurve);
    p.curveB = buildLUT(bwCurve);

    return p;
}

FilterParams FilterEngine::presetVintage() {
    FilterParams p;
    p.name = "vintage";

    p.saturation = 0.75f;
    p.brightness = 0.05f;
    p.contrast = 0.9f;
    p.vignetteStrength = 0.35f;

    // Warm tint curve
    std::array<uint8_t, 10> warmCurve = {0, 8, 32, 64, 112, 144, 184, 208, 232, 255};
    p.curveR = buildLUT(warmCurve);
    p.curveG = buildLUT(warmCurve);
    p.curveB = buildLUT(warmCurve);

    return p;
}

FilterParams FilterEngine::presetCinematic() {
    FilterParams p;
    p.name = "cinematic";

    // Teal/orange color split
    p.colorMatrix = {
        1.0f, 0.0f, 0.0f,
        0.05f, 0.95f, 0.0f,
        0.0f, 0.1f, 0.9f
    };
    p.saturation = 0.8f;
    p.contrast = 1.3f;
    p.vignetteStrength = 0.5f;

    // Desaturate shadows, boost highlights
    std::array<uint8_t, 10> cineCurve = {0, 4, 24, 56, 104, 152, 192, 216, 240, 255};
    p.curveR = buildLUT(cineCurve);
    p.curveG = buildLUT(cineCurve);
    p.curveB = buildLUT(cineCurve);

    return p;
}

std::string FilterEngine::getPresetName(int index) {
    switch (index) {
        case 0: return "HDR Stylization";
        case 1: return "Film Effect";
        case 2: return "Old Camera";
        case 3: return "Vivid";
        case 4: return "B&W Classic";
        case 5: return "Vintage";
        case 6: return "Cinematic";
        default: return "Unknown";
    }
}
