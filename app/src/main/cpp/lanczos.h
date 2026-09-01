#pragma once

#include <opencv2/core.hpp>

/**
 * High-quality image resize using Lanczos interpolation.
 * Uses OpenCV's built-in INTER_LANCZOS4 for optimal performance.
 */

// Resize image to specified dimensions using Lanczos
void resizeLanczos(const cv::Mat& src, cv::Mat& dst, int width, int height);
