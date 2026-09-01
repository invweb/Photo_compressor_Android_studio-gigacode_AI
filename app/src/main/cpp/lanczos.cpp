#include "lanczos.h"
#include <opencv2/imgproc.hpp>

void resizeLanczos(const cv::Mat& src, cv::Mat& dst, int width, int height) {
    cv::resize(src, dst, cv::Size(width, height), 0, 0, cv::INTER_LANCZOS4);
}
