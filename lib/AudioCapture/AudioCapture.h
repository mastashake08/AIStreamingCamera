#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct AudioBuffer {
    int16_t* data;
    size_t samples;
    uint32_t timestamp;
};

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();
    
    // Initialize PDM microphone via I2S
    bool begin();
    
    // Shutdown audio capture
    void end();
    
    // Read audio samples (blocking)
    size_t read(int16_t* buffer, size_t samples);
    
    // Check if audio data is available
    bool available();
    
    // Get audio statistics
    uint32_t getSampleRate() { return _sampleRate; }
    uint8_t getChannels() { return _channels; }
    size_t getBufferSize() { return _bufferSize; }
    
    // Adjust volume (software gain)
    void setGain(float gain) { _gain = constrain(gain, 0.0f, 4.0f); }
    float getGain() { return _gain; }
    
private:
    i2s_config_t _i2sConfig;
    i2s_pin_config_t _pinConfig;
    
    uint32_t _sampleRate;
    uint8_t _channels;
    size_t _bufferSize;
    float _gain;
    
    bool configureI2S();
};

#endif // AUDIO_CAPTURE_H
