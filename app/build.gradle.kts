plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.zx_tole.photocompressor"
    compileSdk {
        version = release(37)
    }

    defaultConfig {
        applicationId = "com.zx_tole.photocompressor"
        minSdk = 24
        targetSdk = 37
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        // NDK ABI filters - only build for ARM (most common on Android)
        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a")
            stl = "c++_shared"
        }
    }

    buildTypes {
        release {
            optimization {
                enable = false
            }
            isMinifyEnabled = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildFeatures {
        viewBinding = true
    }

    packaging {
        jniLibs {
            // Include libc++_shared.so for OpenCV compatibility
            pickFirsts += listOf(
                "**/libc++_shared.so"
            )
        }
    }
}

dependencies {
    implementation(libs.androidx.appcompat)
    implementation(libs.androidx.constraintlayout)
    implementation(libs.androidx.core.ktx)
    implementation(libs.material)
    implementation("androidx.activity:activity-ktx:1.9.3")
    implementation("androidx.exifinterface:exifinterface:1.3.7")
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.espresso.core)
    androidTestImplementation(libs.androidx.junit)
}
