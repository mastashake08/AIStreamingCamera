#include "RTMPClient.h"
#include "../../include/config.h"

RTMPClient::RTMPClient() 
    : _state(RTMPState::DISCONNECTED),
      _serverPort(1935),
      _bytesSent(0),
      _framesSent(0),
      _droppedFrames(0),
      _lastKeepalive(0),
      _streamId(0),
      _transactionId(1),
      _videoTimestamp(0),
      _audioTimestamp(0) {
}

RTMPClient::~RTMPClient() {
    disconnect();
}

bool RTMPClient::parseURL(const String& url) {
    // Parse RTMP URL: rtmp://server:port/app/stream
    // Example: rtmp://a.rtmp.youtube.com/live2/xxxx-xxxx-xxxx-xxxx
    
    if (!url.startsWith("rtmp://")) {
        Serial.println("RTMP: Invalid URL scheme");
        return false;
    }
    
    String remainder = url.substring(7);  // Remove "rtmp://"
    
    int slashPos = remainder.indexOf('/');
    if (slashPos == -1) {
        Serial.println("RTMP: Invalid URL format");
        return false;
    }
    
    String hostPort = remainder.substring(0, slashPos);
    String path = remainder.substring(slashPos + 1);
    
    // Parse host and port
    int colonPos = hostPort.indexOf(':');
    if (colonPos != -1) {
        _serverHost = hostPort.substring(0, colonPos);
        _serverPort = hostPort.substring(colonPos + 1).toInt();
    } else {
        _serverHost = hostPort;
        _serverPort = 1935;  // Default RTMP port
    }
    
    // Parse app and stream name
    int pathSlash = path.indexOf('/');
    if (pathSlash != -1) {
        _appName = path.substring(0, pathSlash);
        _streamName = path.substring(pathSlash + 1);
    } else {
        _appName = path;
        _streamName = "";
    }
    
    Serial.printf("RTMP: Parsed URL - Host: %s, Port: %d, App: %s, Stream: %s\n",
                 _serverHost.c_str(), _serverPort, _appName.c_str(), _streamName.c_str());
    
    return true;
}

bool RTMPClient::connect(const String& url, const String& streamKey) {
    Serial.println("RTMP: Connecting...");
    
    _streamKey = streamKey;
    
    if (!parseURL(url)) {
        setState(RTMPState::ERROR);
        return false;
    }
    
    setState(RTMPState::CONNECTING);
    
    // Connect TCP socket
    if (!_client.connect(_serverHost.c_str(), _serverPort)) {
        Serial.println("RTMP: TCP connection failed");
        setState(RTMPState::ERROR);
        return false;
    }
    
    Serial.println("RTMP: TCP connected");
    setState(RTMPState::HANDSHAKING);
    
    // Perform RTMP handshake
    if (!performHandshake()) {
        Serial.println("RTMP: Handshake failed");
        setState(RTMPState::ERROR);
        return false;
    }
    
    Serial.println("RTMP: Handshake complete");
    
    // Send connect command
    if (!sendConnect()) {
        Serial.println("RTMP: Connect command failed");
        setState(RTMPState::ERROR);
        return false;
    }
    
    Serial.println("RTMP: Connected");
    
    // Create stream
    if (!sendCreateStream()) {
        Serial.println("RTMP: CreateStream failed");
        setState(RTMPState::ERROR);
        return false;
    }
    
    Serial.println("RTMP: Stream created");
    
    // Publish stream
    if (!sendPublish()) {
        Serial.println("RTMP: Publish failed");
        setState(RTMPState::ERROR);
        return false;
    }
    
    Serial.println("RTMP: Now streaming!");
    setState(RTMPState::STREAMING);
    _lastKeepalive = millis();
    
    return true;
}

void RTMPClient::disconnect() {
    if (_client.connected()) {
        _client.stop();
    }
    setState(RTMPState::DISCONNECTED);
    Serial.println("RTMP: Disconnected");
}

bool RTMPClient::sendVideoFrame(camera_fb_t* fb, uint32_t timestamp) {
    if (!isConnected() || !fb) {
        _droppedFrames++;
        return false;
    }
    
    return sendVideoData(fb->buf, fb->len, timestamp);
}

bool RTMPClient::sendAudioSamples(int16_t* samples, size_t count, uint32_t timestamp) {
    if (!isConnected() || !samples || count == 0) {
        return false;
    }
    
    // Convert to PCM byte array
    uint8_t* audioData = (uint8_t*)samples;
    size_t audioSize = count * sizeof(int16_t);
    
    return sendAudioData(audioData, audioSize, timestamp);
}

void RTMPClient::handle() {
    if (!isConnected()) {
        return;
    }
    
    // Send keepalive ping
    uint32_t now = millis();
    if (now - _lastKeepalive >= RTMP_KEEPALIVE_INTERVAL_MS) {
        _lastKeepalive = now;
        
        // Send RTMP ping (Type 6 User Control Message)
        uint8_t pingData[10];
        pingData[0] = 0x02;  // Chunk stream ID 2
        pingData[1] = 0x00;  // Timestamp
        pingData[2] = 0x00;
        pingData[3] = 0x00;
        pingData[4] = 0x00;  // Message length
        pingData[5] = 0x06;
        pingData[6] = 0x04;  // Message type 4 (User Control)
        pingData[7] = 0x00;  // Stream ID
        pingData[8] = 0x00;
        pingData[9] = 0x06;  // Ping event
        
        _client.write(pingData, 10);
        Serial.println("RTMP: Keepalive ping sent");
    }
    
    // Check connection
    if (!_client.connected()) {
        Serial.println("RTMP: Connection lost");
        setState(RTMPState::DISCONNECTED);
    }
}

void RTMPClient::setState(RTMPState newState) {
    if (_state != newState) {
        _state = newState;
        Serial.printf("RTMP: State changed to %d\n", (int)newState);
    }
}

bool RTMPClient::performHandshake() {
    // C0: Version (0x03)
    uint8_t c0 = 0x03;
    if (_client.write(&c0, 1) != 1) {
        return false;
    }
    
    // C1: 1536 bytes (timestamp + zero + random data)
    uint8_t c1[1536];
    memset(c1, 0, 1536);
    
    // Timestamp (4 bytes) - current milliseconds
    uint32_t timestamp = millis();
    c1[0] = (timestamp >> 24) & 0xFF;
    c1[1] = (timestamp >> 16) & 0xFF;
    c1[2] = (timestamp >> 8) & 0xFF;
    c1[3] = timestamp & 0xFF;
    
    // Zero (4 bytes)
    c1[4] = c1[5] = c1[6] = c1[7] = 0;
    
    // Random data (1528 bytes)
    for (int i = 8; i < 1536; i++) {
        c1[i] = random(0, 256);
    }
    
    if (_client.write(c1, 1536) != 1536) {
        return false;
    }
    
    _bytesSent += 1537;
    
    // Read S0
    unsigned long startTime = millis();
    while (_client.available() < 1) {
        if (millis() - startTime > 5000) {
            Serial.println("RTMP: Timeout waiting for S0");
            return false;
        }
        delay(10);
    }
    
    uint8_t s0;
    _client.read(&s0, 1);
    if (s0 != 0x03) {
        Serial.printf("RTMP: Invalid S0 version: 0x%02X\n", s0);
        return false;
    }
    
    // Read S1 (1536 bytes)
    startTime = millis();
    while (_client.available() < 1536) {
        if (millis() - startTime > 5000) {
            Serial.println("RTMP: Timeout waiting for S1");
            return false;
        }
        delay(10);
    }
    
    uint8_t s1[1536];
    _client.read(s1, 1536);
    
    // Send C2 (echo S1)
    if (_client.write(s1, 1536) != 1536) {
        return false;
    }
    
    _bytesSent += 1536;
    
    // Read S2 (can ignore - it's echo of C1)
    startTime = millis();
    while (_client.available() < 1536) {
        if (millis() - startTime > 5000) {
            Serial.println("RTMP: Timeout waiting for S2");
            return false;
        }
        delay(10);
    }
    
    uint8_t s2[1536];
    _client.read(s2, 1536);
    
    return true;
}

bool RTMPClient::sendConnect() {
    // Build connect command using AMF0
    uint8_t packet[1024];
    int pos = 0;
    
    // Command name: "connect"
    writeAMFString(packet, pos, "connect");
    
    // Transaction ID: 1
    writeAMFNumber(packet, pos, 1.0);
    
    // Command object
    writeAMFObject(packet, pos);
    writeAMFPropertyString(packet, pos, "app", _appName);
    writeAMFPropertyString(packet, pos, "type", "nonprivate");
    writeAMFPropertyString(packet, pos, "flashVer", "FMLE/3.0");
    writeAMFPropertyString(packet, pos, "tcUrl", String("rtmp://") + _serverHost + "/" + _appName);
    writeAMFObjectEnd(packet, pos);
    
    // Send via chunk stream ID 3 (control channel)
    return sendChunk(3, 0, 0x14, packet, pos);
}

bool RTMPClient::sendCreateStream() {
    uint8_t packet[256];
    int pos = 0;
    
    // Command name: "createStream"
    writeAMFString(packet, pos, "createStream");
    
    // Transaction ID
    _transactionId++;
    writeAMFNumber(packet, pos, (double)_transactionId);
    
    // Command object (null)
    writeAMFNull(packet, pos);
    
    // Send and wait for response
    if (!sendChunk(3, 0, 0x14, packet, pos)) {
        return false;
    }
    
    // Read response to get stream ID
    delay(100);
    if (_client.available() > 0) {
        uint8_t response[128];
        size_t len = _client.readBytes(response, min((int)_client.available(), 128));
        // Parse stream ID from response (simplified - assumes it's 1)
        _streamId = 1;
    } else {
        _streamId = 1;  // Default
    }
    
    return true;
}

bool RTMPClient::sendPublish() {
    uint8_t packet[512];
    int pos = 0;
    
    // Command name: "publish"
    writeAMFString(packet, pos, "publish");
    
    // Transaction ID (0 for publish)
    writeAMFNumber(packet, pos, 0.0);
    
    // Command object (null)
    writeAMFNull(packet, pos);
    
    // Publishing name (stream key)
    writeAMFString(packet, pos, _streamKey);
    
    // Publishing type: "live"
    writeAMFString(packet, pos, "live");
    
    return sendChunk(4, 0, 0x14, packet, pos);
}

bool RTMPClient::sendFLVHeader() {
    // TODO: Send FLV file header
    return false;
}

bool RTMPClient::sendVideoData(const uint8_t* data, size_t len, uint32_t timestamp) {
    // FLV Video Tag format for JPEG frames
    // Since ESP32-CAM provides JPEG, we'll send as video frame
    
    uint8_t* packet = (uint8_t*)malloc(len + 5);
    if (!packet) {
        Serial.println("RTMP: Failed to allocate video packet");
        _droppedFrames++;
        return false;
    }
    
    int pos = 0;
    
    // FLV VideoTagHeader
    // Frame type (1 = keyframe, 2 = inter) + Codec ID (7 = AVC/H.264, but we use custom for JPEG)
    // For JPEG streaming, we'll use a simplified approach
    packet[pos++] = 0x17;  // Keyframe + AVC (we'll treat JPEG as keyframe)
    
    // AVC packet type (0 = sequence header, 1 = NALU)
    packet[pos++] = 0x01;
    
    // Composition time (3 bytes, 0 for now)
    packet[pos++] = 0x00;
    packet[pos++] = 0x00;
    packet[pos++] = 0x00;
    
    // Copy video data
    memcpy(packet + pos, data, len);
    pos += len;
    
    // Send via RTMP chunk stream 6 (video)
    bool success = sendChunk(6, timestamp, 0x09, packet, pos);
    
    free(packet);
    
    if (success) {
        _framesSent++;
        _videoTimestamp = timestamp;
    } else {
        _droppedFrames++;
    }
    
    return success;
}

bool RTMPClient::sendAudioData(const uint8_t* data, size_t len, uint32_t timestamp) {
    // FLV Audio Tag format for PCM
    
    uint8_t* packet = (uint8_t*)malloc(len + 1);
    if (!packet) {
        Serial.println("RTMP: Failed to allocate audio packet");
        return false;
    }
    
    int pos = 0;
    
    // FLV AudioTagHeader
    // Format (3 = PCM) | Sample rate (3 = 44kHz) | Size (1 = 16-bit) | Type (1 = stereo, 0 = mono)
    // 0011 | 11 | 1 | 0 = 0x3E for 16-bit 44kHz PCM mono
    // For 16kHz: 0011 | 00 | 1 | 0 = 0x32
    packet[pos++] = 0x32;  // PCM, 16kHz, 16-bit, mono
    
    // Copy audio data
    memcpy(packet + pos, data, len);
    pos += len;
    
    // Send via RTMP chunk stream 5 (audio)
    bool success = sendChunk(5, timestamp, 0x08, packet, pos);
    
    free(packet);
    
    if (success) {
        _audioTimestamp = timestamp;
    }
    
    return success;
}

// AMF Encoding Functions
void RTMPClient::writeAMFString(uint8_t* buf, int& pos, const String& str) {
    buf[pos++] = 0x02;  // AMF0 String type
    
    uint16_t len = str.length();
    buf[pos++] = (len >> 8) & 0xFF;
    buf[pos++] = len & 0xFF;
    
    memcpy(buf + pos, str.c_str(), len);
    pos += len;
}

void RTMPClient::writeAMFNumber(uint8_t* buf, int& pos, double num) {
    buf[pos++] = 0x00;  // AMF0 Number type
    
    uint64_t* numPtr = (uint64_t*)&num;
    uint64_t numBits = *numPtr;
    
    // Big-endian encoding
    buf[pos++] = (numBits >> 56) & 0xFF;
    buf[pos++] = (numBits >> 48) & 0xFF;
    buf[pos++] = (numBits >> 40) & 0xFF;
    buf[pos++] = (numBits >> 32) & 0xFF;
    buf[pos++] = (numBits >> 24) & 0xFF;
    buf[pos++] = (numBits >> 16) & 0xFF;
    buf[pos++] = (numBits >> 8) & 0xFF;
    buf[pos++] = numBits & 0xFF;
}

void RTMPClient::writeAMFBoolean(uint8_t* buf, int& pos, bool val) {
    buf[pos++] = 0x01;  // AMF0 Boolean type
    buf[pos++] = val ? 0x01 : 0x00;
}

void RTMPClient::writeAMFNull(uint8_t* buf, int& pos) {
    buf[pos++] = 0x05;  // AMF0 Null type
}

void RTMPClient::writeAMFObject(uint8_t* buf, int& pos) {
    buf[pos++] = 0x03;  // AMF0 Object type
}

void RTMPClient::writeAMFObjectEnd(uint8_t* buf, int& pos) {
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = 0x09;  // Object end marker
}

void RTMPClient::writeAMFProperty(uint8_t* buf, int& pos, const String& name, double value) {
    uint16_t len = name.length();
    buf[pos++] = (len >> 8) & 0xFF;
    buf[pos++] = len & 0xFF;
    memcpy(buf + pos, name.c_str(), len);
    pos += len;
    
    writeAMFNumber(buf, pos, value);
}

void RTMPClient::writeAMFPropertyString(uint8_t* buf, int& pos, const String& name, const String& value) {
    uint16_t len = name.length();
    buf[pos++] = (len >> 8) & 0xFF;
    buf[pos++] = len & 0xFF;
    memcpy(buf + pos, name.c_str(), len);
    pos += len;
    
    writeAMFString(buf, pos, value);
}

void RTMPClient::writeAMFPropertyBool(uint8_t* buf, int& pos, const String& name, bool value) {
    uint16_t len = name.length();
    buf[pos++] = (len >> 8) & 0xFF;
    buf[pos++] = len & 0xFF;
    memcpy(buf + pos, name.c_str(), len);
    pos += len;
    
    writeAMFBoolean(buf, pos, value);
}

// RTMP Chunking
bool RTMPClient::sendChunk(uint8_t chunkStreamId, uint32_t timestamp, uint8_t messageType, 
                           const uint8_t* data, size_t len) {
    // Write chunk header (Type 0 - full header)
    if (!writeChunkHeader(chunkStreamId, timestamp, len, messageType, _streamId)) {
        return false;
    }
    
    // Write data (chunk it if > 128 bytes)
    const size_t chunkSize = 128;
    size_t sent = 0;
    
    while (sent < len) {
        size_t toSend = min(len - sent, chunkSize);
        
        if (sent > 0) {
            // Type 3 header for continuation chunks
            uint8_t contHeader = 0xC0 | (chunkStreamId & 0x3F);
            if (_client.write(&contHeader, 1) != 1) {
                return false;
            }
            _bytesSent++;
        }
        
        if (_client.write(data + sent, toSend) != toSend) {
            return false;
        }
        
        _bytesSent += toSend;
        sent += toSend;
    }
    
    return true;
}

bool RTMPClient::writeChunkHeader(uint8_t chunkStreamId, uint32_t timestamp, 
                                  size_t messageLength, uint8_t messageType, uint32_t streamId) {
    uint8_t header[12];
    int pos = 0;
    
    // Chunk basic header (Type 0)
    header[pos++] = chunkStreamId & 0x3F;
    
    // Timestamp (3 bytes)
    header[pos++] = (timestamp >> 16) & 0xFF;
    header[pos++] = (timestamp >> 8) & 0xFF;
    header[pos++] = timestamp & 0xFF;
    
    // Message length (3 bytes)
    header[pos++] = (messageLength >> 16) & 0xFF;
    header[pos++] = (messageLength >> 8) & 0xFF;
    header[pos++] = messageLength & 0xFF;
    
    // Message type
    header[pos++] = messageType;
    
    // Message stream ID (4 bytes, little-endian)
    header[pos++] = streamId & 0xFF;
    header[pos++] = (streamId >> 8) & 0xFF;
    header[pos++] = (streamId >> 16) & 0xFF;
    header[pos++] = (streamId >> 24) & 0xFF;
    
    if (_client.write(header, pos) != pos) {
        return false;
    }
    
    _bytesSent += pos;
    return true;
}
