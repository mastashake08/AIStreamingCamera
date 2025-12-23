# Streaming Camera - Copilot Instructions

## Project Overview
Streaming camera for Seeed XIAO ESP32-S3 Sense. Streams audio/video to RTMP endpoints. Uses BLE for initial provisioning of WiFi and RTMP credentials.

## Hardware Target
- **Board**: Seeed XIAO ESP32-S3 Sense (OV2640 camera + PDM microphone)
- **Capabilities**: Dual-core Xtensa LX7 @ 240MHz, 8MB PSRAM, WiFi 802.11n, BLE 5.0
- **Peripherals**: Camera (OV2640), PDM microphone, LED (GPIO21), USB-C

## Development Environment

### PlatformIO Workflow
- **Build**: `pio run` (or use PlatformIO IDE buttons)
- **Upload**: `pio run --target upload` - USB-C cable to left port (marked "USB")
- **Monitor**: `pio device monitor` - 115200 baud for serial debugging
- **Clean**: `pio run --target clean` - required when changing partitions/memory config

### Project Structure
- `src/main.cpp` - Main application entry point (setup/loop pattern)
- `include/` - Header files: config structures, pin definitions, credentials template
- `lib/` - Custom libraries for BLE provisioning, RTMP client, audio capture
- `platformio.ini` - Build configuration, dependencies, partition scheme

## Architecture Overview

### Core Components
1. **BLE Provisioning Service** - Accepts WiFi SSID/password and RTMP URL/key during initial setup
2. **NVS Credential Storage** - Persists config to flash using ESP32 Preferences API
3. **WiFi Manager** - Auto-connects using stored credentials, fallback to BLE provisioning mode
4. **Camera Capture Pipeline** - OV2640 frame acquisition with PSRAM buffering
5. **Audio Capture** - PDM microphone sampling (I2S driver) with PCM encoding
6. **RTMP Streaming Client** - Multiplexed audio/video to remote endpoints (YouTube, Twitch, custom)

### Data Flow
```
Camera → PSRAM Buffer → H.264 Encoder → RTMP Client → Internet
Microphone → I2S Buffer → PCM/AAC Encoder ──┘
```

### Dual-Core Task Assignment
- **Core 0 (Protocol CPU)**: WiFi stack, RTMP client, BLE services
- **Core 1 (App CPU)**: Camera capture, audio processing, encoding

## ESP32-S3 Implementation Patterns

### BLE Provisioning
- Use NimBLE library (lighter than Bluedroid): `lib_deps = h2zero/NimBLE-Arduino`
- Create GATT service with characteristics for WiFi SSID, password, RTMP URL, stream key
- Implement write callbacks that store to NVS: `Preferences.begin("config", false)`
- LED blinks during provisioning mode, solid when connected to WiFi

### NVS Credential Storage
```cpp
#include <Preferences.h>
Preferences prefs;
prefs.begin("wifi", false);
prefs.putString("ssid", ssid);
prefs.putString("pass", password);
prefs.end();
```
- Separate namespaces for WiFi ("wifi") and RTMP ("rtmp") credentials
- Check `prefs.isKey("ssid")` on boot to determine if provisioning needed

### Camera Integration
- Use ESP32 Camera library (`esp_camera.h`) - add to `lib_deps` when implementing
- OV2640 pin configuration required for XIAO ESP32-S3 camera module
- PSRAM must be enabled for frame buffers: `board_build.arduino.memory_type = qio_opi`
- Frame buffer sizes: QVGA (320x240) typical, VGA (640x480) max with PSRAM

### Audio Capture (PDM Microphone)
- Use I2S driver for PDM microphone: `#include <driver/i2s.h>`
- Configure I2S in PDM RX mode with appropriate clock/data pins for XIAO ESP32-S3 Sense
- Sample at 16kHz for voice, 44.1kHz for music - balance quality vs bandwidth
- Buffer size impacts latency: 512-1024 samples typical

### RTMP Streaming
- No official ESP32 RTMP library - may need custom implementation or Arduino RTMP port
- FLV container format required (video: H.264, audio: AAC or MP3)
- Consider Motion JPEG fallback if H.264 encoding too resource-intensive
- Maintain persistent TCP connection to RTMP server with keepalive

## Coding Conventions

### Arduino Framework Style
- Use `Serial.begin(115200)` in `setup()` for debugging output
- Prefer `Serial.println()` for debug messages with context (e.g., "Camera init: OK")
- Delays in `loop()` should use `vTaskDelay(pdMS_TO_TICKS(ms))` for FreeRTOS compatibility

### Error Handling
- Check all `esp_err_t` return codes from ESP-IDF functions
- Use LED indicators for status (XIAO has built-in LED on GPIO21)
- Serial output for detailed errors during development

### State Machine Pattern
```cpp
enum State { PROVISIONING, CONNECTING, STREAMING, ERROR };
State currentState = PROVISIONING;
// Handle transitions in loop() based on events
```
- Main loop drives state transitions (BLE provisioning → WiFi connect → RTMP stream)

### Dependencies Management
Add libraries to platformio.ini `[env:seeed_xiao_esp32s3]` section:
```ini
lib_deps = 
    h2zero/NimBLE-Arduino
    espressif/esp32-camera
    ; RTMP library TBD
```

## Common Tasks

### BLE Provisioning Setup
1. Create BLE service with UUIDs for config characteristics
2. Implement callbacks to write WiFi/RTMP credentials to NVS
3. Set device name to discoverable pattern (e.g., "StreamCamera-XXXX")
4. Auto-disable BLE after successful provisioning to save power

### Camera + Audio Pipeline
- Initialize camera first, then I2S microphone
- Sync timestamps between video frames and audio samples
- Use frame buffer queue pattern to decouple capture from encoding
- Handle I2S DMA buffer overflow gracefully

### RTMP Connection Flow
1. Connect to WiFi using stored credentials
2. Resolve RTMP URL and establish TCP connection
3. Send handshake (C0, C1, C2 packets)
4. Publish stream with stored stream key
5. Multiplex encoded video/audio in FLV format

## Testing & Debugging
- Use `#ifdef DEBUG` guards for verbose logging
- Monitor serial output during upload with `pio device monitor -f send_on_enter`
- Test BLE provisioning with nRF Connect mobile app
- Verify NVS storage: read back credentials after writing
- Frame rate issues? Check camera buffer return timing
- RTMP connection drops? Implement reconnection logic with exponential backoff
- Audio/video sync issues? Log timestamps to identify drift

## Key Files to Reference
- [platformio.ini](platformio.ini) - Platform configuration and dependencies
- [src/main.cpp](src/main.cpp) - Application entry point
