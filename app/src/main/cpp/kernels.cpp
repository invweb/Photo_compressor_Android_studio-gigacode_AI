#include "kernels.h"
#include <opencv2/imgproc.hpp>

void gaussianBlurKernel(cv::Mat& image, float sigma) {
    if (sigma <= 0.0f) return;
    int kernelSize = static_cast<int>(std::round(2.0f * 3.0f * sigma + 1.0f));
    // Ensure odd kernel size
    if (kernelSize % 2 == 0) kernelSize++;
    kernelSize = std::max(kernelSize, 3);
    cv::GaussianBlur(image, image, cv::Size(kernelSize, kernelSize), sigma);
}

void unsharpMask(cv::Mat& image, float amount) {
    if (amount <= 0.0f) return;

    cv::Mat blurred;
    cv::GaussianBlur(image, blurred, cv::Size(0, 0), 3.0f);

    // sharpened = original + (original - blurred) * amount
    cv::addWeighted(image, 1.0f + amount, blurred, -amount, 0.0f, image);
}

void bilateralFilterCustom(cv::Mat& image, float d, float sigmaColor, float sigmaSpace) {
    if (d <= 0.0f && sigmaColor <= 0.0f && sigmaSpace <= 0.0f) return;

    int diameter = d > 0 ? static_cast<int>(d) : 9;
    float sc = sigmaColor > 0 ? sigmaColor : diameter / 3.0f;
    float ss = sigmaSpace > 0 ? sigmaSpace : diameter / 3.0f;

    cv::bilateralFilter(image, image, diameter, sc, ss);
}

void applyConvolution(cv::Mat& image, const cv::Mat& kernel) {
    if (kernel.empty()) return;
    cv::filter2D(image, image, image.depth(), kernel);
}

void applyEmboss(cv::Mat& image) {
    // Emboss kernel
    static const float embossData[] = {
        -2.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,
         0.0f, 1.0f, 2.0f
    };
    cv::Mat kernel(3, 3, CV_32F, const_cast<float*>(embossData));
    applyConvolution(image, kernel);
}

void applyEdgeEnhance(cv::Mat& image) {
    // Sharpen/edge-enhance kernel
    static const float edgeData[] = {
         0.0f, -1.0f,  0.0f,
        -1.0f,  5.0f, -1.0f,
         0.0f, -1.0f,  0.0f
    };
    cv::Mat kernel(3, 3, CV_32F, const_cast<float*>(edgeData));
    applyConvolution(image, kernel);
}
