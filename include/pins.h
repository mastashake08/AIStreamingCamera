#ifndef PINS_H
#define PINS_H

// Seeed XIAO ESP32-S3 Sense Pin Definitions
// Camera module (OV2640) uses dedicated CAM pins
// PDM microphone uses I2S interface

// Camera pins (OV2640 on XIAO ESP32-S3 Sense)
#define CAMERA_PIN_PWDN     -1
#define CAMERA_PIN_RESET    -1
#define CAMERA_PIN_XCLK     10
#define CAMERA_PIN_SIOD     40  // SDA
#define CAMERA_PIN_SIOC     39  // SCL

#define CAMERA_PIN_D7       48
#define CAMERA_PIN_D6       11
#define CAMERA_PIN_D5       12
#define CAMERA_PIN_D4       14
#define CAMERA_PIN_D3       16
#define CAMERA_PIN_D2       18
#define CAMERA_PIN_D1       17
#define CAMERA_PIN_D0       15
#define CAMERA_PIN_VSYNC    38
#define CAMERA_PIN_HREF     47
#define CAMERA_PIN_PCLK     13

// PDM Microphone pins (I2S)
#define I2S_MIC_SERIAL_CLOCK 42  // PDM_CLK
#define I2S_MIC_SERIAL_DATA  41  // PDM_DATA

// Status LED
#define LED_PIN             21

// I2S Configuration
#define I2S_MIC_PORT        I2S_NUM_0
#define I2S_MIC_SAMPLE_RATE 16000
#define I2S_MIC_CHANNELS    1
#define I2S_MIC_BITS        16

#endif // PINS_H
