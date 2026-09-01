#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include <android/log.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "image_filters.h"

#define LOG_TAG "PhotoFilterJNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * Convert Java jintArray (ARGB_8888 pixels) to cv::Mat (CV_8UC4).
 * On little-endian ARM, Java ARGB_8888 is stored as ABGR in native byte order.
 */
static cv::Mat jintArrayToMat(JNIEnv* env, jintArray pixels, int width, int height) {
    jint* raw = env->GetIntArrayElements(pixels, nullptr);
    if (!raw) {
        LOGE("Failed to get jintArray elements");
        return cv::Mat();
    }

    // Create a cv::Mat that shares memory with the jint[]
    // Java ARGB_8888 on little-endian: each int is 0xAABBGGRR (ABGR order)
    // We treat it as CV_8UC4 and convert later
    cv::Mat mat(height, width, CV_8UC4, raw);
    return mat;
}

/**
 * Write processed cv::Mat data back to Java jintArray.
 * Expects mat to be CV_8UC4 (BGRA format from OpenCV).
 */
static void matToJintArray(JNIEnv* env, const cv::Mat& mat, jintArray pixels) {
    if (mat.empty() || mat.type() != CV_8UC4) return;

    jint* raw = env->GetIntArrayElements(pixels, nullptr);
    if (!raw) {
        LOGE("Failed to get jintArray elements for write");
        return;
    }

    // Copy BGRA data directly (Java ARGB_8888 on LE is ABGR, same layout as BGRA on LE)
    std::memcpy(raw, mat.data, mat.total() * sizeof(jint));

    env->ReleaseIntArrayElements(pixels, raw, 0); // 0 = copy back, don't delete
}

extern "C"

/*
 * Process a Bitmap in-place: reads pixels, applies filter, writes back.
 * Returns true on success, false on error.
 */
JNIEXPORT jboolean JNICALL
Java_com_zx_1tole_photocompressor_MainActivity_processBitmapInPlace(
        JNIEnv* env,
        jobject /* this */,
        jintArray pixels,
        jint width,
        jint height,
        jstring presetJson) {

    if (!pixels || width <= 0 || height <= 0) {
        LOGE("Invalid parameters: pixels=%p, w=%d, h=%d", pixels, (int)width, (int)height);
        return JNI_FALSE;
    }

    try {
        // Get pixel data
        cv::Mat mat = jintArrayToMat(env, pixels, (int)width, (int)height);
        if (mat.empty()) {
            LOGE("Failed to create cv::Mat from pixels");
            return JNI_FALSE;
        }

        // Parse preset name from JSON (simplified: extract "name" field)
        const char* jsonStr = env->GetStringUTFChars(presetJson, nullptr);
        std::string json(jsonStr);
        env->ReleaseStringUTFChars(presetJson, jsonStr);

        // Extract preset name from JSON
        std::string presetName;
        size_t namePos = json.find("\"name\"");
        if (namePos != std::string::npos) {
            size_t colonPos = json.find(':', namePos);
            size_t quoteStart = json.find('"', colonPos != std::string::npos ? colonPos : namePos);
            if (quoteStart != std::string::npos) {
                size_t quoteEnd = json.find('"', quoteStart + 1);
                if (quoteEnd != std::string::npos) {
                    presetName = json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                }
            }
        }

        // Get FilterParams based on preset name
        FilterParams params;
        if (presetName == "hdr_stylization") params = FilterEngine::presetHDR();
        else if (presetName == "film_effect") params = FilterEngine::presetFilm();
        else if (presetName == "old_camera") params = FilterEngine::presetOldCamera();
        else if (presetName == "vivid") params = FilterEngine::presetVivid();
        else if (presetName == "bw_classic") params = FilterEngine::presetBWClassic();
        else if (presetName == "vintage") params = FilterEngine::presetVintage();
        else if (presetName == "cinematic") params = FilterEngine::presetCinematic();
        else params = FilterEngine::presetHDR(); // Default to HDR

        LOGD("Applying preset: %s (contrast=%.2f, sat=%.2f, sharp=%.2f)",
             presetName.c_str(), params.contrast, params.saturation, params.sharpness);

        // Convert BGRA -> BGR for processing (some filters expect 3-channel)
        cv::Mat bgrMat;
        cv::cvtColor(mat, bgrMat, cv::COLOR_BGRA2BGR);

        // Apply filter pipeline (in-place on BGR)
        FilterEngine::apply(bgrMat, params);

        // Convert BGR -> BGRA for output
        cv::cvtColor(bgrMat, mat, cv::COLOR_BGR2BGRA);

        // Write back to Java int array
        matToJintArray(env, mat, pixels);

        return JNI_TRUE;

    } catch (const std::exception& e) {
        LOGE("Exception in processBitmapInPlace: %s", e.what());
        return JNI_FALSE;
    } catch (...) {
        LOGE("Unknown exception in processBitmapInPlace");
        return JNI_FALSE;
    }
}

extern "C"

/*
 * Process a Bitmap and return a new pixel array.
 * Returns jintArray with processed pixels, or null on error.
 */
JNIEXPORT jintArray JNICALL
Java_com_zx_1tole_photocompressor_MainActivity_processBitmapNew(
        JNIEnv* env,
        jobject /* this */,
        jintArray srcPixels,
        jint width,
        jint height,
        jstring presetJson) {

    if (!srcPixels || width <= 0 || height <= 0) {
        return nullptr;
    }

    try {
        // Get source pixel data
        cv::Mat srcMat = jintArrayToMat(env, srcPixels, (int)width, (int)height);
        if (srcMat.empty()) {
            return nullptr;
        }

        // Parse preset
        const char* jsonStr = env->GetStringUTFChars(presetJson, nullptr);
        std::string json(jsonStr);
        env->ReleaseStringUTFChars(presetJson, jsonStr);

        std::string presetName;
        size_t namePos = json.find("\"name\"");
        if (namePos != std::string::npos) {
            size_t colonPos = json.find(':', namePos);
            size_t quoteStart = json.find('"', colonPos != std::string::npos ? colonPos : namePos);
            if (quoteStart != std::string::npos) {
                size_t quoteEnd = json.find('"', quoteStart + 1);
                if (quoteEnd != std::string::npos) {
                    presetName = json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                }
            }
        }

        FilterParams params;
        if (presetName == "hdr_stylization") params = FilterEngine::presetHDR();
        else if (presetName == "film_effect") params = FilterEngine::presetFilm();
        else if (presetName == "old_camera") params = FilterEngine::presetOldCamera();
        else if (presetName == "vivid") params = FilterEngine::presetVivid();
        else if (presetName == "bw_classic") params = FilterEngine::presetBWClassic();
        else if (presetName == "vintage") params = FilterEngine::presetVintage();
        else if (presetName == "cinematic") params = FilterEngine::presetCinematic();
        else params = FilterEngine::presetHDR();

        // Convert BGRA -> BGR
        cv::Mat bgrMat;
        cv::cvtColor(srcMat, bgrMat, cv::COLOR_BGRA2BGR);

        // Apply filter
        FilterEngine::apply(bgrMat, params);

        // Convert BGR -> BGRA
        cv::Mat resultMat;
        cv::cvtColor(bgrMat, resultMat, cv::COLOR_BGR2BGRA);

        // Create new jintArray for result
        jintArray result = env->NewIntArray((int)width * (int)height);
        if (!result) {
            LOGE("Failed to allocate result array");
            return nullptr;
        }

        // Copy data to Java array
        matToJintArray(env, resultMat, result);

        return result;

    } catch (const std::exception& e) {
        LOGE("Exception in processBitmapNew: %s", e.what());
        return nullptr;
    } catch (...) {
        LOGE("Unknown exception in processBitmapNew");
        return nullptr;
    }
}

extern "C"

/*
 * Return the library version string.
 */
JNIEXPORT jstring JNICALL
Java_com_zx_1tole_photocompressor_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "PhotoFilter C++ v1.0 - OpenCV " + std::string(cv::getVersionString());
    return env->NewStringUTF(hello.c_str());
}

extern "C"

/*
 * Get list of available preset names (comma-separated).
 */
JNIEXPORT jstring JNICALL
Java_com_zx_1tole_photocompressor_MainActivity_getPresetNames(
        JNIEnv* env,
        jobject /* this */) {
    std::string names = "hdr_stylization,film_effect,old_camera,vivid,bw_classic,vintage,cinematic";
    return env->NewStringUTF(names.c_str());
}
