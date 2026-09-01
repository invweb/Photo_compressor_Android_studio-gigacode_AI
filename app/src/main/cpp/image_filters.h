#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <array>

/**
 * Filter parameters that define a complete image processing preset.
 */
struct FilterParams {
    // Color correction 3x3 matrix (row-major), applied per-channel
    std::array<float, 9> colorMatrix = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };

    // Tone curves: 10 control points each, values 0-255 (input -> output mapping)
    std::array<uint8_t, 256> curveR{};
    std::array<uint8_t, 256> curveG{};
    std::array<uint8_t, 256> curveB{};

    // Adjustment factors
    float saturation = 1.0f;
    float contrast = 1.0f;
    float brightness = 0.0f;
    float blurRadius = 0.0f;       // Gaussian blur sigma
    float sharpness = 0.0f;        // Unsharp mask amount
    float vignetteStrength = 0.0f; // 0.0 to 1.0

    // Preset name identifier
    std::string name;

    FilterParams() {
        // Initialize curves to identity (linear)
        for (int i = 0; i < 256; i++) {
            curveR[i] = curveG[i] = curveB[i] = static_cast<uint8_t>(i);
        }
    }
};

/**
 * Main filter engine.
 * Processes a 3-channel BGR cv::Mat in-place according to FilterParams.
 */
class FilterEngine {
public:
    // Apply the complete filter pipeline to an image
    static void apply(cv::Mat& image, const FilterParams& params);

    // Build a 256-entry LUT from 10 control points (catmull-rom spline interpolation)
    static std::array<uint8_t, 256> buildLUT(const std::array<uint8_t, 10>& controlPoints);

    // Generate the 7 built-in preset parameter sets
    static FilterParams presetHDR();
    static FilterParams presetFilm();
    static FilterParams presetOldCamera();
    static FilterParams presetVivid();
    static FilterParams presetBWClassic();
    static FilterParams presetVintage();
    static FilterParams presetCinematic();

    // Get preset name by index (0-6)
    static std::string getPresetName(int index);

private:
    // Apply brightness adjustment
    static void applyBrightness(cv::Mat& image, float brightness);

    // Apply contrast adjustment
    static void applyContrast(cv::Mat& image, float contrast);

    // Apply color correction matrix
    static void applyColorMatrix(cv::Mat& image, const std::array<float, 9>& matrix);

    // Apply saturation via HSV conversion
    static void applySaturation(cv::Mat& image, float saturation);

    // Apply tone curves via LUT
    static void applyCurves(cv::Mat& image,
                            const std::array<uint8_t, 256>& curveR,
                            const std::array<uint8_t, 256>& curveG,
                            const std::array<uint8_t, 256>& curveB);

    // Apply vignette effect
    static void applyVignette(cv::Mat& image, float strength);
};
