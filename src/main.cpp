#include <Arduino.h>
#include "pins.h"
#include "config.h"
#include <BLEProvisioning.h>
#include <WiFiManager.h>
#include <CameraCapture.h>
#include <AudioCapture.h>
#include <RTMPClient.h>

// ============================================================================
// State Machine
// ============================================================================

enum class AppState {
    INIT,
    PROVISIONING,
    CONNECTING_WIFI,
    CONNECTING_RTMP,
    STREAMING,
    ERROR
};

AppState currentState = AppState::INIT;

// ============================================================================
// Global Objects
// ============================================================================

BLEProvisioning bleProvisioning;
WiFiManager wifiManager;
CameraCapture camera;
AudioCapture audio;
RTMPClient rtmpClient;

// Credentials
String wifiSSID, wifiPassword;
String rtmpURL, rtmpKey;

// FreeRTOS Task Handles
TaskHandle_t cameraTaskHandle = NULL;
TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t streamTaskHandle = NULL;

// Shared data queues
QueueHandle_t videoFrameQueue = NULL;
QueueHandle_t audioBufferQueue = NULL;

// LED control
void setLED(bool on) {
    digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void blinkLED(uint8_t count, uint16_t delayMs = 200) {
    for (uint8_t i = 0; i < count; i++) {
        setLED(true);
        delay(delayMs);
        setLED(false);
        if (i < count - 1) delay(delayMs);
    }
}

// ============================================================================
// FreeRTOS Tasks
// ============================================================================

// Camera capture task (Core 1 - App CPU)
void cameraTask(void* parameter) {
    Serial.println("Task: Camera task started");
    
    while (true) {
        if (currentState == AppState::STREAMING) {
            camera_fb_t* fb = camera.captureFrame();
            
            if (fb) {
                // Send frame to streaming queue
                xQueueSend(videoFrameQueue, &fb, 0);  // Non-blocking
                
                // Note: Frame will be released by streaming task
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(33));  // ~30 FPS
    }
}

// Audio capture task (Core 1 - App CPU)
void audioTask(void* parameter) {
    Serial.println("Task: Audio task started");
    
    const size_t bufferSize = AUDIO_BUFFER_SIZE;
    int16_t* audioBuffer = (int16_t*)malloc(bufferSize * sizeof(int16_t));
    
    while (true) {
        if (currentState == AppState::STREAMING) {
            size_t samplesRead = audio.read(audioBuffer, bufferSize);
            
            if (samplesRead > 0) {
                // Send to streaming task (simplified - needs proper queue handling)
                // TODO: Implement audio queue with timestamp synchronization
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(64));  // 16kHz / 1024 samples ≈ 64ms
    }
    
    free(audioBuffer);
}

// RTMP streaming task (Core 0 - Protocol CPU)
void streamTask(void* parameter) {
    Serial.println("Task: Streaming task started");
    
    camera_fb_t* fb = NULL;
    uint32_t frameTimestamp = 0;
    
    while (true) {
        if (currentState == AppState::STREAMING && rtmpClient.isConnected()) {
            // Get frame from queue
            if (xQueueReceive(videoFrameQueue, &fb, pdMS_TO_TICKS(100)) == pdTRUE) {
                // Send frame to RTMP server
                bool sent = rtmpClient.sendVideoFrame(fb, frameTimestamp);
                
                if (sent) {
                    frameTimestamp += 33;  // 30 FPS = 33ms per frame
                }
                
                // Release frame buffer
                camera.releaseFrame(fb);
            }
            
            // Handle RTMP keepalive
            rtmpClient.handle();
            
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// ============================================================================
// State Machine Functions
// ============================================================================

void enterProvisioning() {
    Serial.println("State: Entering provisioning mode");
    currentState = AppState::PROVISIONING;
    
    // Start BLE provisioning
    String deviceName = String(BLE_DEVICE_NAME) + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    bleProvisioning.begin(deviceName.c_str());
    
    // Blink LED to indicate provisioning mode
    blinkLED(3, 100);
    
    // Set callback for when credentials are received
    bleProvisioning.onCredentialsReceived([]() {
        Serial.println("State: Credentials received via BLE");
        currentState = AppState::CONNECTING_WIFI;
    });
}

void enterConnectingWiFi() {
    Serial.println("State: Connecting to WiFi");
    currentState = AppState::CONNECTING_WIFI;
    
    // Stop BLE to save resources
    bleProvisioning.end();
    
    // Load credentials
    if (!bleProvisioning.loadWiFiCredentials(wifiSSID, wifiPassword)) {
        Serial.println("State: Failed to load WiFi credentials");
        currentState = AppState::ERROR;
        return;
    }
    
    // Connect to WiFi
    wifiManager.setAutoReconnect(true);
    
    if (wifiManager.connect(wifiSSID, wifiPassword, WIFI_CONNECT_TIMEOUT_MS)) {
        currentState = AppState::CONNECTING_RTMP;
        setLED(true);  // Solid LED when WiFi connected
    } else {
        Serial.println("State: WiFi connection failed");
        currentState = AppState::ERROR;
    }
}

void enterConnectingRTMP() {
    Serial.println("State: Connecting to RTMP");
    currentState = AppState::CONNECTING_RTMP;
    
    // Load RTMP credentials
    if (!bleProvisioning.loadRTMPCredentials(rtmpURL, rtmpKey)) {
        Serial.println("State: Failed to load RTMP credentials");
        currentState = AppState::ERROR;
        return;
    }
    
    // Connect to RTMP server
    if (rtmpClient.connect(rtmpURL, rtmpKey)) {
        currentState = AppState::STREAMING;
        Serial.println("State: Streaming started!");
    } else {
        Serial.println("State: RTMP connection failed (implementation required)");
        // For now, continue anyway to test camera/audio
        currentState = AppState::STREAMING;
    }
}

void enterStreaming() {
    Serial.println("State: Streaming mode");
    currentState = AppState::STREAMING;
    
    // Start FreeRTOS tasks on appropriate cores
    xTaskCreatePinnedToCore(
        cameraTask,
        "CameraTask",
        TASK_CAMERA_STACK_SIZE,
        NULL,
        TASK_CAMERA_PRIORITY,
        &cameraTaskHandle,
        TASK_CAMERA_CORE
    );
    
    xTaskCreatePinnedToCore(
        audioTask,
        "AudioTask",
        TASK_AUDIO_STACK_SIZE,
        NULL,
        TASK_AUDIO_PRIORITY,
        &audioTaskHandle,
        TASK_AUDIO_CORE
    );
    
    xTaskCreatePinnedToCore(
        streamTask,
        "StreamTask",
        TASK_STREAM_STACK_SIZE,
        NULL,
        TASK_STREAM_PRIORITY,
        &streamTaskHandle,
        TASK_STREAM_CORE
    );
    
    Serial.println("State: All tasks started");
}

// ============================================================================
// Arduino Setup & Loop
// ============================================================================

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=================================");
    Serial.println("Streaming Camera v1.0");
    Serial.println("=================================\n");
    
    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    setLED(false);
    
    // Print system info
    Serial.printf("Chip: ESP32-S3 @ %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("PSRAM: %d KB\n", ESP.getPsramSize() / 1024);
    Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("Free PSRAM: %d KB\n", ESP.getFreePsram() / 1024);
    Serial.println();
    
    // Initialize hardware
    Serial.println("Initializing hardware...");
    
    if (!camera.begin()) {
        Serial.println("ERROR: Camera initialization failed!");
        currentState = AppState::ERROR;
        return;
    }
    Serial.println("✓ Camera initialized");
    
    if (!audio.begin()) {
        Serial.println("ERROR: Audio initialization failed!");
        currentState = AppState::ERROR;
        return;
    }
    Serial.println("✓ Audio initialized");
    
    // Create queues
    videoFrameQueue = xQueueCreate(2, sizeof(camera_fb_t*));
    audioBufferQueue = xQueueCreate(4, sizeof(void*));
    
    Serial.println("\nHardware initialization complete\n");
    
    // Check if already provisioned
    if (bleProvisioning.hasStoredCredentials()) {
        Serial.println("Found stored credentials, connecting to WiFi...");
        enterConnectingWiFi();
    } else {
        Serial.println("No stored credentials, entering provisioning mode...");
        enterProvisioning();
    }
}

void loop() {
    // State machine handler
    switch (currentState) {
        case AppState::INIT:
            // Should not reach here
            break;
            
        case AppState::PROVISIONING:
            // BLE handles provisioning, wait for callback
            blinkLED(1, 50);
            delay(2000);
            break;
            
        case AppState::CONNECTING_WIFI:
            enterConnectingWiFi();
            break;
            
        case AppState::CONNECTING_RTMP:
            enterConnectingRTMP();
            break;
            
        case AppState::STREAMING:
            if (streamTaskHandle == NULL) {
                enterStreaming();
            }
            
            // Monitor system health
            static uint32_t lastHealthCheck = 0;
            if (millis() - lastHealthCheck >= 10000) {
                lastHealthCheck = millis();
                
                Serial.printf("\n[Health] Heap: %d KB, PSRAM: %d KB, FPS: %.1f\n",
                             ESP.getFreeHeap() / 1024,
                             ESP.getFreePsram() / 1024,
                             camera.getFrameRate());
                
                if (rtmpClient.isConnected()) {
                    Serial.printf("[RTMP] Frames: %d, Dropped: %d, Bytes: %d KB\n",
                                 rtmpClient.getFramesSent(),
                                 rtmpClient.getDroppedFrames(),
                                 rtmpClient.getBytesSent() / 1024);
                }
            }
            
            // Handle WiFi reconnection
            wifiManager.handle();
            delay(100);
            break;
            
        case AppState::ERROR:
            Serial.println("ERROR: System in error state");
            blinkLED(5, 100);
            delay(5000);
            break;
    }
}