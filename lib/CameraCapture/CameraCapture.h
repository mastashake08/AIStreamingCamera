#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H

#include "esp_camera.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class CameraCapture {
public:
    CameraCapture();
    ~CameraCapture();
    
    // Initialize camera with XIAO ESP32-S3 Sense configuration
    bool begin();
    
    // Shutdown camera
    void end();
    
    // Get current frame (must call releaseFrame when done)
    camera_fb_t* captureFrame();
    
    // Release frame buffer back to driver
    void releaseFrame(camera_fb_t* fb);
    
    // Get camera sensor info
    sensor_t* getSensor();
    
    // Frame statistics
    size_t getFrameSize() { return _lastFrameSize; }
    uint32_t getLastCaptureTime() { return _lastCaptureTime; }
    float getFrameRate() { return _frameRate; }
    
    // Camera settings
    bool setFrameSize(framesize_t size);
    bool setQuality(uint8_t quality);  // 0-63, lower is higher quality
    bool setPixelFormat(pixformat_t format);
    
private:
    camera_config_t _config;
    size_t _lastFrameSize;
    uint32_t _lastCaptureTime;
    uint32_t _frameCount;
    uint32_t _fpsStartTime;
    float _frameRate;
    
    void updateFrameRate();
    bool configurePins();
};

#endif // CAMERA_CAPTURE_H
