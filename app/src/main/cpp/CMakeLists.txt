cmake_minimum_required(VERSION 3.22.1)
project("photocompressor")

# OpenCV include directory (copied from SDK, in same dir as CMakeLists.txt)
set(OpenCV_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/opencv-include")

# OpenCV library path (prebuilt .so from SDK)
set(OpenCV_LIB_PATH "${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libopencv_java4.so")

# Main native library
add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp
    image_filters.cpp
    color_spaces.cpp
    kernels.cpp
    lanczos.cpp
)

# Link OpenCV and Android system libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    ${OpenCV_LIB_PATH}
    android
    log
    jnigraphics
)

# C++ standard and optimizations
target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -O3 -ffast-math)

# Add OpenCV include path
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIR})
