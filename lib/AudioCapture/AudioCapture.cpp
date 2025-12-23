#include <Arduino.h>
#include "AudioCapture.h"
#include "../../include/pins.h"

AudioCapture::AudioCapture()
    : _sampleRate(16000),
      _channels(1),
      _bufferSize(512),
      _gain(1.0f) {
}

AudioCapture::~AudioCapture() {
    end();
}

bool AudioCapture::configureI2S() {
    _i2sConfig.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    _i2sConfig.sample_rate = _sampleRate;
    _i2sConfig.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    _i2sConfig.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    _i2sConfig.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    _i2sConfig.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    _i2sConfig.dma_buf_count = 4;
    _i2sConfig.dma_buf_len = _bufferSize;
    _i2sConfig.use_apll = false;
    _i2sConfig.tx_desc_auto_clear = false;
    _i2sConfig.fixed_mclk = 0;

    _pinConfig.mck_io_num = I2S_PIN_NO_CHANGE;
    _pinConfig.bck_io_num = I2S_PIN_NO_CHANGE;
    _pinConfig.ws_io_num = I2S_MIC_SERIAL_CLOCK;
    _pinConfig.data_out_num = I2S_PIN_NO_CHANGE;
    _pinConfig.data_in_num = I2S_MIC_SERIAL_DATA;

    return true;
}

bool AudioCapture::begin() {
    Serial.println("Audio: Initializing PDM microphone...");
    
    if (!configureI2S()) {
        Serial.println("Audio: I2S config failed");
        return false;
    }
    
    esp_err_t err = i2s_driver_install(I2S_MIC_PORT, &_i2sConfig, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("Audio: Driver install failed (0x%x)\n", err);
        return false;
    }
    
    err = i2s_set_pin(I2S_MIC_PORT, &_pinConfig);
    if (err != ESP_OK) {
        Serial.println("Audio: Pin config failed");
        i2s_driver_uninstall(I2S_MIC_PORT);
        return false;
    }
    
    // Clear DMA buffers
    i2s_zero_dma_buffer(I2S_MIC_PORT);
    
    Serial.printf("Audio: Initialized at %d Hz\n", _sampleRate);
    return true;
}

void AudioCapture::end() {
    i2s_driver_uninstall(I2S_MIC_PORT);
    Serial.println("Audio: Deinitialized");
}

size_t AudioCapture::read(int16_t* buffer, size_t samples) {
    if (!buffer) {
        return 0;
    }
    
    size_t bytesRead = 0;
    esp_err_t err = i2s_read(I2S_MIC_PORT, buffer, samples * sizeof(int16_t), 
                             &bytesRead, portMAX_DELAY);
    
    if (err != ESP_OK) {
        Serial.printf("Audio: Read error (0x%x)\n", err);
        return 0;
    }
    
    // Apply software gain
    if (_gain != 1.0f) {
        size_t samplesRead = bytesRead / sizeof(int16_t);
        for (size_t i = 0; i < samplesRead; i++) {
            int32_t sample = buffer[i] * _gain;
            buffer[i] = constrain(sample, INT16_MIN, INT16_MAX);
        }
    }
    
    return bytesRead / sizeof(int16_t);
}

bool AudioCapture::available() {
    size_t bytesAvailable = 0;
    esp_err_t err = i2s_read(I2S_MIC_PORT, nullptr, 0, &bytesAvailable, 0);
    return (err == ESP_OK && bytesAvailable > 0);
}
