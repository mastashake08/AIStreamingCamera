#ifndef RTMP_CLIENT_H
#define RTMP_CLIENT_H

#include <WiFiClient.h>
#include <Arduino.h>
#include "esp_camera.h"

enum class RTMPState {
    DISCONNECTED,
    CONNECTING,
    HANDSHAKING,
    CONNECTED,
    STREAMING,
    ERROR
};

class RTMPClient {
public:
    RTMPClient();
    ~RTMPClient();
    
    // Connect to RTMP server
    bool connect(const String& url, const String& streamKey);
    
    // Disconnect from server
    void disconnect();
    
    // Send video frame
    bool sendVideoFrame(camera_fb_t* fb, uint32_t timestamp);
    
    // Send audio samples
    bool sendAudioSamples(int16_t* samples, size_t count, uint32_t timestamp);
    
    // Connection management
    bool isConnected() { return _state == RTMPState::STREAMING; }
    RTMPState getState() { return _state; }
    
    // Statistics
    uint32_t getBytesSent() { return _bytesSent; }
    uint32_t getFramesSent() { return _framesSent; }
    uint32_t getDroppedFrames() { return _droppedFrames; }
    
    // Keepalive (call periodically)
    void handle();
    
private:
    WiFiClient _client;
    RTMPState _state;
    
    String _serverHost;
    uint16_t _serverPort;
    String _appName;
    String _streamName;
    String _streamKey;
    
    uint32_t _bytesSent;
    uint32_t _framesSent;
    uint32_t _droppedFrames;
    uint32_t _lastKeepalive;
    
    // RTMP protocol implementation (placeholder)
    bool parseURL(const String& url);
    bool performHandshake();
    bool sendConnect();
    bool sendCreateStream();
    bool sendPublish();
    
    // FLV muxing (placeholder)
    bool sendFLVHeader();
    bool sendVideoData(const uint8_t* data, size_t len, uint32_t timestamp);
    bool sendAudioData(const uint8_t* data, size_t len, uint32_t timestamp);
    
    void setState(RTMPState newState);
};

#endif // RTMP_CLIENT_H
