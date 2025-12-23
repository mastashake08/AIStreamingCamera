#ifndef MODEL_PLACEHOLDER_H
#define MODEL_PLACEHOLDER_H

// ============================================================================
// AI/ML INTEGRATION GUIDE FOR ESP32-S3
// ============================================================================
//
// This project supports multiple AI frameworks. Choose based on your needs:
//
// ============================================================================
// OPTION 1: ESP-DL (RECOMMENDED - Best Performance)
// ============================================================================
//
// Espressif's official deep learning library, optimized for ESP32.
//
// Setup:
//   1. Add to platformio.ini lib_deps:
//      espressif/esp-dl@^1.0.0
//
//   2. Include headers:
//      #include "esp_dl.hpp"
//
//   3. Supported models:
//      - MobileNet v1/v2 (quantized)
//      - Human face detection
//      - Human face recognition
//      - Cat face detection
//
//   4. Documentation:
//      https://github.com/espressif/esp-dl
//
// ============================================================================
// OPTION 2: TENSORFLOW LITE MICRO (Most Compatible)
// ============================================================================
//
// Industry standard, supports most TensorFlow models.
//
// Setup:
//   1. Clone TFLite Micro:
//      git clone https://github.com/tensorflow/tflite-micro.git lib/tflite-micro
//
//   2. Follow ESP32 integration guide:
//      https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples
//
//   3. Convert your model:
//      xxd -i model.tflite > model.h
//
//   4. Include and use:
//      #include "tensorflow/lite/micro/micro_interpreter.h"
//      extern const unsigned char model_data[];
//      extern const int model_data_len;
//
// ============================================================================
// OPTION 3: EDGE IMPULSE (Easiest End-to-End)
// ============================================================================
//
// Complete ML pipeline from training to deployment.
//
// Setup:
//   1. Train model at edgeimpulse.com
//   2. Export as "Arduino library"
//   3. Extract to lib/ folder
//   4. Include generated header:
//      #include "edge-impulse-sdk/classifier/ei_run_classifier.h"
//
//   5. Use built-in functions:
//      ei_impulse_result_t result;
//      run_classifier(&signal, &result, debug);
//
// Documentation:
//   https://docs.edgeimpulse.com/docs/deployment/arduino-library
//
// ============================================================================
// OPTION 4: CUSTOM MODEL (C ARRAY)
// ============================================================================
//
// If you have a pre-trained .tflite model:
//
// 1. Ensure model is INT8 quantized (ESP32-S3 has no FPU)
//
// 2. Convert to C array:
//    xxd -i model.tflite > model.h
//
// 3. Produces:
//    unsigned char model_tflite[] = { 0x1c, 0x00, ... };
//    unsigned int model_tflite_len = 12345;
//
// 4. Rename to match code:
//    const unsigned char model_data[] = { ... };
//    const int model_data_len = ...;
//
// 5. Call in main.cpp:
//    aiModel.loadModel(model_data, model_data_len);
//
// ============================================================================
// PERFORMANCE TIPS
// ============================================================================
//
// - Use INT8 quantization (10-20x faster than float)
// - Keep models < 2MB to fit in flash
// - Use 96x96 or 128x128 input for real-time (30+ FPS)
// - Allocate tensor arena in PSRAM (60-100KB typical)
// - Run inference at reduced rate (10 FPS) to save CPU for streaming
// - Pin inference task to Core 1 (App CPU)
//
// ============================================================================

#endif // MODEL_PLACEHOLDER_H
