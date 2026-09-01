#pragma once

#include <opencv2/core.hpp>

/**
 * Convolution kernel operations for blur, sharpen, and custom filters.
 * All functions operate in-place on 3-channel (BGR) Mats.
 */

// Gaussian blur with adjustable radius (sigma)
void gaussianBlurKernel(cv::Mat& image, float sigma);

// Unsharp mask for sharpening (amount: 0.0 to 2.0+)
void unsharpMask(cv::Mat& image, float amount);

// Bilateral filter (edge-preserving blur)
// d: diameter of each pixel neighborhood
// sigmaColor: filter sigma in the color space
// sigmaSpace: filter sigma in the coordinate space
void bilateralFilterCustom(cv::Mat& image, float d, float sigmaColor, float sigmaSpace);

// Apply a custom convolution kernel
void applyConvolution(cv::Mat& image, const cv::Mat& kernel);

// Emboss kernel (for edge detection effects)
void applyEmboss(cv::Mat& image);

// Edge enhance kernel
void applyEdgeEnhance(cv::Mat& image);
