#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

/**
 * Color space conversion utilities.
 * All functions operate on 3-channel (BGR) or 4-channel (BGRA) Mats.
 */

// Convert BGR (3ch) to LAB (3ch)
void bgrToLab(const cv::Mat& bgr, cv::Mat& lab);

// Convert LAB (3ch) to BGR (3ch)
void labToBgr(const cv::Mat& lab, cv::Mat& bgr);

// Convert BGR (3ch) to HSV (3ch)
void bgrToHsv(const cv::Mat& bgr, cv::Mat& hsv);

// Convert HSV (3ch) to BGR (3ch)
void hsvToBgr(const cv::Mat& hsv, cv::Mat& bgr);

// Convert BGR to RGB (channel swap)
void bgrToRgb(const cv::Mat& bgr, cv::Mat& rgb);

// Convert RGB to BGR (channel swap)
void rgbToBgr(const cv::Mat& rgb, cv::Mat& bgr);

// Apply sepia tone to a BGR image (in-place)
void applySepia(cv::Mat& image);

// Convert to black & white using luminance weights (in-place)
void applyBlackWhite(cv::Mat& image);
