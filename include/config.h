#ifndef CONFIG_H
#define CONFIG_H

// Application Configuration

// Camera Configuration
#define CAMERA_FRAME_SIZE   FRAMESIZE_QVGA  // 320x240 for performance
#define CAMERA_JPEG_QUALITY 12               // 0-63, lower means higher quality
#define CAMERA_FB_COUNT     2                // Frame buffer count (double buffering)

// AI Model Configuration
#define AI_MODEL_PATH       "/model.tflite"
#define AI_INFERENCE_FPS    10               // Run inference at 10 FPS
#define AI_INPUT_WIDTH      224
#define AI_INPUT_HEIGHT     224

// Audio Configuration
#define AUDIO_SAMPLE_RATE   16000            // 16kHz for voice
#define AUDIO_BUFFER_SIZE   1024             // Samples per buffer
#define AUDIO_CHANNELS      1                // Mono

// RTMP Configuration
#define RTMP_CONNECT_TIMEOUT_MS  5000
#define RTMP_KEEPALIVE_INTERVAL_MS 30000
#define RTMP_MAX_RECONNECT_ATTEMPTS 5

// WiFi Configuration
#define WIFI_CONNECT_TIMEOUT_MS 10000
#define WIFI_MAX_RECONNECT_ATTEMPTS 3

// BLE Configuration
#define BLE_DEVICE_NAME     "AICamera"
#define BLE_PROVISIONING_TIMEOUT_MS 300000   // 5 minutes

// NVS Namespaces
#define NVS_NAMESPACE_WIFI  "wifi"
#define NVS_NAMESPACE_RTMP  "rtmp"

// FreeRTOS Task Configuration
#define TASK_CAMERA_STACK_SIZE    8192
#define TASK_CAMERA_PRIORITY      2
#define TASK_CAMERA_CORE          1          // App CPU

#define TASK_AUDIO_STACK_SIZE     4096
#define TASK_AUDIO_PRIORITY       2
#define TASK_AUDIO_CORE           1          // App CPU

#define TASK_AI_STACK_SIZE        8192
#define TASK_AI_PRIORITY          1
#define TASK_AI_CORE              1          // App CPU

#define TASK_STREAM_STACK_SIZE    8192
#define TASK_STREAM_PRIORITY      3
#define TASK_STREAM_CORE          0          // Protocol CPU

#define TASK_WIFI_STACK_SIZE      4096
#define TASK_WIFI_PRIORITY        2
#define TASK_WIFI_CORE            0          // Protocol CPU

// Debug Configuration
#define DEBUG_SERIAL_ENABLED  true
#define DEBUG_LOG_LEVEL       3              // 0=None, 1=Error, 2=Warn, 3=Info, 4=Debug

#endif // CONFIG_H
