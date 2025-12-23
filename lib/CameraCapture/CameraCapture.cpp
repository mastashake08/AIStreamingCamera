#include <Arduino.h>
#include "CameraCapture.h"
#include "../../include/pins.h"
#include "../../include/config.h"

CameraCapture::CameraCapture() 
    : _lastFrameSize(0),
      _lastCaptureTime(0),
      _frameCount(0),
      _fpsStartTime(0),
      _frameRate(0.0f) {
}

CameraCapture::~CameraCapture() {
    end();
}

bool CameraCapture::configurePins() {
    _config.ledc_channel = LEDC_CHANNEL_0;
    _config.ledc_timer = LEDC_TIMER_0;
    _config.pin_d0 = CAMERA_PIN_D0;
    _config.pin_d1 = CAMERA_PIN_D1;
    _config.pin_d2 = CAMERA_PIN_D2;
    _config.pin_d3 = CAMERA_PIN_D3;
    _config.pin_d4 = CAMERA_PIN_D4;
    _config.pin_d5 = CAMERA_PIN_D5;
    _config.pin_d6 = CAMERA_PIN_D6;
    _config.pin_d7 = CAMERA_PIN_D7;
    _config.pin_xclk = CAMERA_PIN_XCLK;
    _config.pin_pclk = CAMERA_PIN_PCLK;
    _config.pin_vsync = CAMERA_PIN_VSYNC;
    _config.pin_href = CAMERA_PIN_HREF;
    _config.pin_sccb_sda = CAMERA_PIN_SIOD;
    _config.pin_sccb_scl = CAMERA_PIN_SIOC;
    _config.pin_pwdn = CAMERA_PIN_PWDN;
    _config.pin_reset = CAMERA_PIN_RESET;
    _config.xclk_freq_hz = 20000000;  // 20MHz
    _config.pixel_format = PIXFORMAT_JPEG;
    _config.frame_size = CAMERA_FRAME_SIZE;
    _config.jpeg_quality = CAMERA_JPEG_QUALITY;
    _config.fb_count = CAMERA_FB_COUNT;
    _config.grab_mode = CAMERA_GRAB_LATEST;  // Always get latest frame
    
    return true;
}

bool CameraCapture::begin() {
    Serial.println("Camera: Initializing OV2640...");
    
    // Configure camera pins
    configurePins();
    
    // Initialize camera
    esp_err_t err = esp_camera_init(&_config);
    if (err != ESP_OK) {
        Serial.printf("Camera: Init failed with error 0x%x\n", err);
        return false;
    }
    
    // Get sensor handle
    sensor_t* sensor = esp_camera_sensor_get();
    if (!sensor) {
        Serial.println("Camera: Failed to get sensor");
        return false;
    }
    
    // Apply sensor optimizations
    sensor->set_brightness(sensor, 0);     // -2 to 2
    sensor->set_contrast(sensor, 0);       // -2 to 2
    sensor->set_saturation(sensor, 0);     // -2 to 2
    sensor->set_special_effect(sensor, 0); // 0 = No effect
    sensor->set_whitebal(sensor, 1);       // 0 = disable, 1 = enable
    sensor->set_awb_gain(sensor, 1);       // 0 = disable, 1 = enable
    sensor->set_wb_mode(sensor, 0);        // 0 = auto
    sensor->set_exposure_ctrl(sensor, 1);  // 0 = disable, 1 = enable
    sensor->set_aec2(sensor, 0);           // 0 = disable, 1 = enable
    sensor->set_ae_level(sensor, 0);       // -2 to 2
    sensor->set_aec_value(sensor, 300);    // 0 to 1200
    sensor->set_gain_ctrl(sensor, 1);      // 0 = disable, 1 = enable
    sensor->set_agc_gain(sensor, 0);       // 0 to 30
    sensor->set_gainceiling(sensor, (gainceiling_t)0);  // 0 to 6
    sensor->set_bpc(sensor, 0);            // 0 = disable, 1 = enable
    sensor->set_wpc(sensor, 1);            // 0 = disable, 1 = enable
    sensor->set_raw_gma(sensor, 1);        // 0 = disable, 1 = enable
    sensor->set_lenc(sensor, 1);           // 0 = disable, 1 = enable
    sensor->set_hmirror(sensor, 0);        // 0 = disable, 1 = enable
    sensor->set_vflip(sensor, 0);          // 0 = disable, 1 = enable
    sensor->set_dcw(sensor, 1);            // 0 = disable, 1 = enable
    sensor->set_colorbar(sensor, 0);       // 0 = disable, 1 = enable
    
    Serial.printf("Camera: Initialized successfully (PID: 0x%02x, VER: 0x%02x, MIDL: 0x%02x, MIDH: 0x%02x)\n",
                 sensor->id.PID, sensor->id.VER, sensor->id.MIDL, sensor->id.MIDH);
    
    _fpsStartTime = millis();
    return true;
}

void CameraCapture::end() {
    esp_camera_deinit();
    Serial.println("Camera: Deinitialized");
}

camera_fb_t* CameraCapture::captureFrame() {
    uint32_t startTime = millis();
    
    camera_fb_t* fb = esp_camera_fb_get();
    
    if (!fb) {
        Serial.println("Camera: Frame capture failed");
        return nullptr;
    }
    
    _lastCaptureTime = millis() - startTime;
    _lastFrameSize = fb->len;
    
    updateFrameRate();
    
    return fb;
}

void CameraCapture::releaseFrame(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

sensor_t* CameraCapture::getSensor() {
    return esp_camera_sensor_get();
}

bool CameraCapture::setFrameSize(framesize_t size) {
    sensor_t* sensor = getSensor();
    if (sensor) {
        if (sensor->set_framesize(sensor, size) == 0) {
            Serial.printf("Camera: Frame size changed to %d\n", size);
            return true;
        }
    }
    return false;
}

bool CameraCapture::setQuality(uint8_t quality) {
    sensor_t* sensor = getSensor();
    if (sensor) {
        if (sensor->set_quality(sensor, quality) == 0) {
            Serial.printf("Camera: JPEG quality changed to %d\n", quality);
            return true;
        }
    }
    return false;
}

bool CameraCapture::setPixelFormat(pixformat_t format) {
    sensor_t* sensor = getSensor();
    if (sensor) {
        if (sensor->set_pixformat(sensor, format) == 0) {
            Serial.printf("Camera: Pixel format changed to %d\n", format);
            return true;
        }
    }
    return false;
}

void CameraCapture::updateFrameRate() {
    _frameCount++;
    
    uint32_t elapsed = millis() - _fpsStartTime;
    if (elapsed >= 1000) {  // Update FPS every second
        _frameRate = (_frameCount * 1000.0f) / elapsed;
        _frameCount = 0;
        _fpsStartTime = millis();
    }
}
