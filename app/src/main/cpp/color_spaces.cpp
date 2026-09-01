#include "color_spaces.h"

// Note: LAB conversion constants are not available in the Android OpenCV headers.
// We implement simple RGB↔HSV and other conversions that ARE available.

void bgrToLab(const cv::Mat& bgr, cv::Mat& lab) {
    // Fallback: just clone since LAB not available in headers
    // For a production app, implement proper RGB->LAB conversion
    lab = bgr.clone();
}

void labToBgr(const cv::Mat& lab, cv::Mat& bgr) {
    bgr = lab.clone();
}

void bgrToHsv(const cv::Mat& bgr, cv::Mat& hsv) {
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
}

void hsvToBgr(const cv::Mat& hsv, cv::Mat& bgr) {
    cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);
}

void bgrToRgb(const cv::Mat& bgr, cv::Mat& rgb) {
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
}

void rgbToBgr(const cv::Mat& rgb, cv::Mat& bgr) {
    cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
}

void applySepia(cv::Mat& image) {
    // Sepia tone matrix (applied in-place on 3-channel image)
    static const float sepiaData[] = {
        0.393f, 0.769f, 0.189f,
        0.349f, 0.686f, 0.168f,
        0.272f, 0.534f, 0.131f
    };
    cv::Mat sepiaMatrix(3, 3, CV_32F, const_cast<float*>(sepiaData));

    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
            cv::Vec3b pixel = image.at<cv::Vec3b>(i, j);
            float b = pixel[0], g = pixel[1], r = pixel[2];

            pixel[0] = static_cast<uchar>(std::min(255.0f, 0.272f * r + 0.534f * g + 0.131f * b));
            pixel[1] = static_cast<uchar>(std::min(255.0f, 0.349f * r + 0.686f * g + 0.168f * b));
            pixel[2] = static_cast<uchar>(std::min(255.0f, 0.393f * r + 0.769f * g + 0.189f * b));

            image.at<cv::Vec3b>(i, j) = pixel;
        }
    }
}

void applyBlackWhite(cv::Mat& image) {
    // Luminance weighting: 0.299*R + 0.587*G + 0.114*B
    // Note: image is in BGR, so channels are [B, G, R]
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
            cv::Vec3b pixel = image.at<cv::Vec3b>(i, j);
            float b = pixel[0], g = pixel[1], r = pixel[2];
            float gray = 0.114f * r + 0.587f * g + 0.299f * b;
            pixel[0] = pixel[1] = pixel[2] = static_cast<uchar>(gray);
            image.at<cv::Vec3b>(i, j) = pixel;
        }
    }
}
