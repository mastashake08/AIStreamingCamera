#ifndef AI_INFERENCE_H
#define AI_INFERENCE_H

#include "esp_camera.h"
#include <Arduino.h>

// AI Inference stub - ready for implementation
// See include/model_placeholder.h for integration guide

struct InferenceResult {
    uint8_t classId;
    float confidence;
    char label[32];
    uint32_t inferenceTime;
};

class AIInference {
public:
    AIInference();
    ~AIInference();
    
    // Load model from embedded data
    bool loadModel(const unsigned char* modelData, size_t modelSize);
    
    // Run inference on camera frame
    bool runInference(camera_fb_t* fb, InferenceResult& result);
    
    // Get model info
    bool isModelLoaded() { return _modelLoaded; }
    size_t getModelSize() { return _modelSize; }
    uint32_t getLastInferenceTime() { return _lastInferenceTime; }
    
    // Model configuration
    bool setInputSize(uint16_t width, uint16_t height);
    
private:
    bool _modelLoaded;
    size_t _modelSize;
    uint32_t _lastInferenceTime;
    
    uint16_t _inputWidth;
    uint16_t _inputHeight;
    
    // Placeholder for ML framework objects
    // Will be implemented when AI library is added
    void* _interpreter;
    uint8_t* _tensorArena;
    static constexpr size_t _tensorArenaSize = 60 * 1024;  // 60KB
    
    bool preprocessImage(camera_fb_t* fb, float* inputTensor);
    void postprocessOutput(float* output, size_t outputSize, InferenceResult& result);
};

#endif // AI_INFERENCE_H
