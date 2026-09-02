# Photo Filter — Android App

> Native C++ photo filter engine for stories with 7 signature presets and real-time preview.

---

## 📱 Screenshots / Скриншоты

| Screen 1                             |
|--------------------------------------|
| ![Home](https://github.com/invweb/Photo_compressor_Android_studio-gigacode_AI/blob/main/screenshots/Screenshot.jpg) |

> **Note:** Replace placeholder images with actual screenshots:
> ```bash
> adb exec-out screencap -p > screenshots/01_home.png
> ```

---

## 🇬🇧 English

### Features

- **7 Signature Presets:** HDR Stylization, Film Effect, Old Camera, Vivid, B&W Classic, Vintage, Cinematic
- **Real-time Preview:** Before/after comparison with draggable slider
- **Fine-tuning:** Saturation, contrast, and brightness sliders
- **Native C++ Engine:** All filters run on CPU via OpenCV for maximum performance
- **Save & Share:** Export filtered photos to gallery

### Architecture

```
┌──────────────────────────────────────────────┐
│              Kotlin UI Layer                  │
│  ┌──────────┐  ┌───────────┐  ┌────────────┐ │
│  │ Photo    │  │ Preset    │  │ Before/After│ │
│  │ Picker   │  │ Selector  │  │ Slider     │ │
│  └────┬─────┘  └─────┬─────┘  └─────┬──────┘ │
│       │              │              │         │
│       ▼              ▼              ▼         │
│  ┌──────────────────────────────────────┐     │
│  │         MainActivity.kt              │     │
│  │  - manages Bitmap lifecycle          │     │
│  │  - calls JNI processBitmapInPlace()  │     │
│  └──────────────────┬───────────────────┘     │
│                     │ JNI boundary             │
├─────────────────────┼─────────────────────────┤
│          C++ / JNI Layer                        │
│  ┌──────────────────────────────────────┐     │
│  │        native-lib.cpp (JNI)          │     │
│  │  Java_com_zx_1tole_..._processBitmap │     │
│  └──────────────────┬───────────────────┘     │
│                     │                          │
│  ┌──────────────────┴───────────────────┐     │
│  │  image_filters.cpp  color_spaces.cpp │     │
│  │  kernels.cpp         lanczos.cpp     │     │
│  └──────────────────┬───────────────────┘     │
│                     │                          │
│  ┌──────────────────┴───────────────────┐     │
│  │         OpenCV 4.11 (libopencv)      │     │
│  │  cv::Mat, cv::GaussianBlur, cv::LUT  │     │
│  └──────────────────────────────────────┘     │
└──────────────────────────────────────────────┘
```

### C++ Filter Pipeline

```
Input (ARGB_8888)
    → cv::Mat (BGRA)
    → cv::cvtColor → BGR
    → Brightness adjustment (cv::add)
    → Contrast adjustment (cv::convertScaleAbs)
    → Color correction matrix (cv::transform)
    → Saturation via HSV (RGB→HSV → scale S → HSV→RGB)
    → Tone curves via LUT (cv::LUT per channel)
    → Gaussian blur (cv::GaussianBlur)
    → Unsharp mask sharpening
    → Vignette effect (radial gradient)
    → cv::cvtColor → BGRA
    → Write back to int[]
```

### Presets

| # | Preset | Key Parameters |
|---|--------|----------------|
| 1 | **HDR Stylization** | contrast=1.5, saturation=1.6, S-curve, sharpness=0.6 |
| 2 | **Film Effect** | warm color matrix, saturation=0.85, cross-process curve |
| 3 | **Old Camera** | saturation=0.6, blur=0.8, sepia curve, vignette=0.5 |
| 4 | **Vivid** | saturation=1.8, contrast=1.4, midtone boost |
| 5 | **B&W Classic** | saturation=0.0, luminance weights, dramatic S-curve |
| 6 | **Vintage** | warm tint, saturation=0.75, vignette=0.35 |
| 7 | **Cinematic** | teal/orange matrix, saturation=0.8, vignette=0.5 |

### Tech Stack

- **Language:** Kotlin + C++17
- **UI:** Material Design 3, ViewBinding
- **Native:** OpenCV 4.11, CMake, NDK r28
- **Min SDK:** 24 (Android 7.0)
- **Target SDK:** 37 (Android 15)

### Build Instructions

```bash
# Build debug APK
./gradlew assembleDebug

# Install on connected device
./gradlew installDebug

# Clean build
./gradlew clean assembleDebug
```

---

## 🇷🇺 Русский

### Возможности

- **7 фирменных пресетов:** HDR, Плёнка, Старая камера, Яркий, Ч/Б, Винтаж, Кинематографичный
- **Предпросмотр в реальном времени:** Слайдер сравнения до/после с перетаскиванием
- **Тонкая настройка:** Слайдеры насыщенности, контраста и яркости
- **Нативный C++ движок:** Все фильтры работают на CPU через OpenCV для максимальной производительности
- **Сохранение и обмен:** Экспорт обработанных фото в галерею

### Архитектура

```
┌──────────────────────────────────────────────┐
│              Слой Kotlin UI                   │
│  ┌──────────┐  ┌───────────┐  ┌────────────┐ │
│  │ Выбор    │  │ Выбор     │  │ Слайдер     │ │
│  │ фото     │  │ пресетов  │  │ до/после   │ │
│  └────┬─────┘  └─────┬─────┘  └─────┬──────┘ │
│       │              │              │         │
│       ▼              ▼              ▼         │
│  ┌──────────────────────────────────────┐     │
│  │         MainActivity.kt              │     │
│  │  - управление Bitmap                 │     │
│  │  - вызов JNI processBitmapInPlace()  │     │
│  └──────────────────┬───────────────────┘     │
│                     │ Граница JNI              │
├─────────────────────┼─────────────────────────┤
│         Слой C++ / JNI                          │
│  ┌──────────────────────────────────────┐     │
│  │        native-lib.cpp (JNI)          │     │
│  │  Java_com_zx_1tole_..._processBitmap │     │
│  └──────────────────┬───────────────────┘     │
│                     │                          │
│  ┌──────────────────┴───────────────────┐     │
│  │  image_filters.cpp  color_spaces.cpp │     │
│  │  kernels.cpp         lanczos.cpp     │     │
│  └──────────────────┬───────────────────┘     │
│                     │                          │
│  ┌──────────────────┴───────────────────┐     │
│  │         OpenCV 4.11 (libopencv)      │     │
│  │  cv::Mat, cv::GaussianBlur, cv::LUT  │     │
│  └──────────────────────────────────────┘     │
└──────────────────────────────────────────────┘
```

### Пайплайн C++ фильтров

```
Вход (ARGB_8888)
    → cv::Mat (BGRA)
    → cv::cvtColor → BGR
    → Яркость (cv::add)
    → Контраст (cv::convertScaleAbs)
    → Матрица цветовой коррекции (cv::transform)
    → Насыщенность через HSV (RGB→HSV → масштаб S → HSV→RGB)
    → Тоновые кривые через LUT (cv::LUT по каналам)
    → Размытие Gaussian (cv::GaussianBlur)
    → Резкость unsharp mask
    → Виньетка (радиальный градиент)
    → cv::cvtColor → BGRA
    → Запись обратно в int[]
```

### Пресеты

| # | Пресет | Ключевые параметры |
|---|--------|-------------------|
| 1 | **HDR Stylization** | контраст=1.5, насыщенность=1.6, S-кривая, резкость=0.6 |
| 2 | **Film Effect** | тёплая матрица, насыщенность=0.85, cross-process кривая |
| 3 | **Old Camera** | насыщенность=0.6, размытие=0.8, сепия-кривая, виньетка=0.5 |
| 4 | **Vivid** | насыщенность=1.8, контраст=1.4, подъём полутонов |
| 5 | **B&W Classic** | насыщенность=0.0, веса яркости, драматичная S-кривая |
| 6 | **Vintage** | тёплый оттенок, насыщенность=0.75, виньетка=0.35 |
| 7 | **Cinematic** | teal/orange матрица, насыщенность=0.8, виньетка=0.5 |

### Технологический стек

- **Языки:** Kotlin + C++17
- **UI:** Material Design 3, ViewBinding
- **Native:** OpenCV 4.11, CMake, NDK r28
- **Min SDK:** 24 (Android 7.0)
- **Target SDK:** 37 (Android 15)

### Сборка

```bash
# Сборка debug APK
./gradlew assembleDebug

# Установка на подключённое устройство
./gradlew installDebug

# Полная пересборка
./gradlew clean assembleDebug
```

---

## 📄 License

MIT License
