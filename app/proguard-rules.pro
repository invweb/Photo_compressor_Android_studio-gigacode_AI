# Photo Filter - ProGuard rules
# Keep JNI native methods
-keep class com.zx_tole.photocompressor.MainActivity {
    <methods>;
}

# Keep native method signatures
-keepclasseswithmembernames,class * {
    native <methods>;
}

# OpenCV
-keep class org.opencv.** { *; }
-dontwarn org.opencv.**
