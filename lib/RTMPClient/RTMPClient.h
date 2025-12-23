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
    uint32_t _streamId;
    uint32_t _transactionId;
    uint32_t _videoTimestamp;
    uint32_t _audioTimestamp;
    
    // RTMP protocol implementation
    bool parseURL(const String& url);
    bool performHandshake();
    bool sendConnect();
    bool sendCreateStream();
    bool sendPublish();
    
    // RTMP chunking
    bool sendChunk(uint8_t chunkType, uint32_t timestamp, uint8_t messageType, 
                   const uint8_t* data, size_t len);
    bool writeChunkHeader(uint8_t chunkStreamId, uint32_t timestamp, 
                          size_t messageLength, uint8_t messageType, uint32_t streamId);
    
    // AMF encoding
    void writeAMFString(uint8_t* buf, int& pos, const String& str);
    void writeAMFNumber(uint8_t* buf, int& pos, double num);
    void writeAMFBoolean(uint8_t* buf, int& pos, bool val);
    void writeAMFNull(uint8_t* buf, int& pos);
    void writeAMFObject(uint8_t* buf, int& pos);
    void writeAMFObjectEnd(uint8_t* buf, int& pos);
    void writeAMFProperty(uint8_t* buf, int& pos, const String& name, double value);
    void writeAMFPropertyString(uint8_t* buf, int& pos, const String& name, const String& value);
    void writeAMFPropertyBool(uint8_t* buf, int& pos, const String& name, bool value);
    
    // FLV muxing
    bool sendFLVHeader();
    bool sendVideoData(const uint8_t* data, size_t len, uint32_t timestamp);
    bool sendAudioData(const uint8_t* data, size_t len, uint32_t timestamp);
    
    void setState(RTMPState newState);
};

#endif // RTMP_CLIENT_H
