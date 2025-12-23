# AI Streaming Camera

AIoT streaming camera for Seeed XIAO ESP32-S3 Sense with on-device AI inference, wireless provisioning, and RTMP streaming capabilities.

## Features

- 📷 **OV2640 Camera** - QVGA/VGA video capture with PSRAM buffering
- 🎤 **PDM Microphone** - 16kHz audio capture via I2S
- 🧠 **AI Inference** - On-device ML processing (stub ready for ESP-DL, TensorFlow Lite Micro, or Edge Impulse)
- 📡 **BLE Provisioning** - Wireless WiFi and RTMP credential configuration
- 🌐 **RTMP Streaming** - Live video/audio streaming to YouTube, Twitch, or custom servers
- 💾 **NVS Storage** - Persistent credential storage
- ⚡ **Dual-Core Architecture** - Optimized task distribution across ESP32-S3 cores

## Hardware Requirements

### Primary Board
- **Seeed XIAO ESP32-S3 Sense** (with OV2640 camera module and PDM microphone)
  - Dual-core Xtensa LX7 @ 240MHz
  - 8MB PSRAM, 8MB Flash
  - WiFi 802.11 b/g/n, Bluetooth 5.0 (BLE)
  - USB-C for programming and power

### Connections
- Camera module must be connected to XIAO ESP32-S3 Sense
- USB-C cable for programming (left port marked "USB")
- Power via USB-C (5V) or battery connector

## Architecture

### Dual-Core Task Distribution
- **Core 0 (Protocol CPU)**: WiFi stack, RTMP client, BLE services, network I/O
- **Core 1 (App CPU)**: Camera capture, audio processing, AI inference, encoding

### State Machine
```
INIT → PROVISIONING → CONNECTING_WIFI → CONNECTING_RTMP → STREAMING
```

### Data Flow
```
Camera → PSRAM Buffer → AI Model → Annotated Frame → H.264 Encoder → RTMP Client → Internet
Microphone → I2S Buffer → PCM/AAC Encoder ────────────────────────────┘
```

## Getting Started

### Prerequisites
- [PlatformIO](https://platformio.org/) installed (VS Code extension or CLI)
- Seeed XIAO ESP32-S3 Sense board
- USB-C cable
- Mobile device with BLE app (e.g., nRF Connect) for provisioning

### Installation

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd AIStreamingCamera
   ```

2. **Open in PlatformIO**
   - Open the project folder in VS Code with PlatformIO extension
   - Or use PlatformIO CLI

3. **Build the project**
   ```bash
   pio run
   ```

4. **Upload to board**
   ```bash
   pio run --target upload
   ```

5. **Monitor serial output**
   ```bash
   pio device monitor
   ```

## BLE Provisioning

On first boot, the camera enters provisioning mode:

1. **LED indicator** - Blinking indicates provisioning mode
2. **Connect via BLE** - Device name: `AICamera-XXXX`
3. **Configure credentials**:
   - **WiFi SSID** - Your WiFi network name
   - **WiFi Password** - Your WiFi password
   - **RTMP URL** - Streaming server URL (e.g., `rtmp://live.twitch.tv/app/`)
   - **RTMP Stream Key** - Your stream key from platform
4. **Auto-save** - Credentials stored in NVS flash memory
5. **Auto-connect** - Device connects to WiFi and begins streaming

### Using nRF Connect App
1. Install nRF Connect (iOS/Android)
2. Scan for `AICamera-XXXX`
3. Connect and discover services
4. Write WiFi/RTMP credentials to respective characteristics
5. Device will auto-reboot and connect

## Configuration

### Camera Settings
Edit [include/config.h](include/config.h):
```cpp
#define CAMERA_FRAME_SIZE FRAMESIZE_QVGA  // QVGA (320x240) or VGA (640x480)
#define CAMERA_JPEG_QUALITY 12             // 0-63, lower = higher quality
#define CAMERA_FRAME_RATE 15               // Target FPS
```

### Audio Settings
```cpp
#define AUDIO_SAMPLE_RATE 16000   // 16kHz for voice
#define AUDIO_BUFFER_SIZE 512     // Samples per buffer
```

### AI Model Settings
```cpp
#define AI_INPUT_WIDTH 224        // Model input dimensions
#define AI_INPUT_HEIGHT 224
#define AI_INFERENCE_INTERVAL 500 // ms between inferences
```

### Task Stack Sizes
```cpp
#define CAMERA_TASK_STACK_SIZE 8192
#define AUDIO_TASK_STACK_SIZE 4096
#define AI_TASK_STACK_SIZE 8192
#define RTMP_TASK_STACK_SIZE 8192
```

## Project Structure

```
AIStreamingCamera/
├── include/
│   ├── pins.h              # Hardware pin definitions
│   ├── config.h            # Application configuration
│   └── model_placeholder.h # ML integration guide
├── lib/
│   ├── BLEProvisioning/    # BLE credential configuration
│   ├── WiFiManager/        # WiFi connection management
│   ├── CameraCapture/      # OV2640 camera driver
│   ├── AudioCapture/       # PDM microphone via I2S
│   ├── AIInference/        # ML inference (stub)
│   └── RTMPClient/         # RTMP streaming (stub)
├── src/
│   └── main.cpp            # Application entry point
├── platformio.ini          # Build configuration
└── README.md
```

## Development Status

### ✅ Completed
- Modular library architecture
- BLE provisioning service with NVS storage
- WiFi manager with auto-reconnect
- Camera capture with PSRAM optimization
- Audio capture via I2S PDM
- Dual-core FreeRTOS task orchestration
- State machine implementation

### 🚧 In Progress (Stubs Provided)
- **AI Inference** - Framework ready, requires ML library integration
  - Options: ESP-DL, TensorFlow Lite Micro, Edge Impulse
  - See [include/model_placeholder.h](include/model_placeholder.h) for integration guide
- **RTMP Client** - Protocol outlined, requires implementation
  - Handshake, FLV muxing, streaming logic needed
  - May require external library or manual protocol implementation

### 📋 Roadmap
- [ ] Integrate ML framework (ESP-DL recommended)
- [ ] Implement RTMP protocol client
- [ ] Add H.264/AAC encoding
- [ ] Implement audio/video synchronization
- [ ] Add web configuration interface
- [ ] Power optimization modes
- [ ] SD card recording support

## AI Model Integration

### Recommended: ESP-DL (Espressif Deep Learning)
```bash
# Add to platformio.ini
lib_deps = 
    espressif/esp-dl@^1.0.0
```

### Alternative: TensorFlow Lite Micro
- Manual integration required
- See [TensorFlow Lite Micro documentation](https://github.com/tensorflow/tflite-micro)

### Alternative: Edge Impulse
- Train model in Edge Impulse Studio
- Export C++ library
- Add to `lib/` folder

See [include/model_placeholder.h](include/model_placeholder.h) for detailed integration instructions.

## Troubleshooting

### Build Issues
```bash
# Clean build
pio run --target clean
pio run
```

### Upload Issues
- Ensure USB-C cable is connected to left port (marked "USB")
- Hold BOOT button during upload if auto-reset fails
- Check board selection in platformio.ini

### Camera Not Working
- Verify camera module is properly connected
- Check PSRAM is enabled in platformio.ini
- Monitor serial output for error messages

### BLE Not Visible
- Ensure BLE is enabled on mobile device
- Check device enters provisioning mode (LED blinking)
- Restart device and try again

### WiFi Connection Fails
- Verify credentials via serial monitor
- Check WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Ensure signal strength is adequate (check RSSI in logs)

## Serial Monitor Commands

Connect at **115200 baud** to see debug output:
```bash
pio device monitor
```

Expected output:
```
AI Camera Starting...
State: INIT
Camera: Initializing OV2640...
Camera: Initialized at 15.2 FPS
Audio: Initializing PDM microphone...
Audio: Initialized at 16000 Hz
State: PROVISIONING
BLE: Starting provisioning...
```

## Contributing

Contributions welcome! Areas needing development:
- RTMP protocol implementation
- AI model integration examples
- Audio/video encoding optimization
- Power management
- Additional streaming protocols (WebRTC, HLS)

## License

This project is provided as-is for educational and development purposes.

## Credits

- **Hardware**: [Seeed Studio XIAO ESP32-S3 Sense](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html)
- **Frameworks**: 
  - [Arduino for ESP32](https://github.com/espressif/arduino-esp32)
  - [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
  - [ESP32 Camera Driver](https://github.com/espressif/esp32-camera)

## Support

For issues, questions, or contributions:
- Check [Troubleshooting](#troubleshooting) section
- Review serial monitor output
- Check [.github/copilot-instructions.md](.github/copilot-instructions.md) for development guidelines
