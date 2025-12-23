#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <functional>

enum class WiFiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED
};

class WiFiManager {
public:
    WiFiManager();
    
    // Connect to WiFi using provided credentials
    bool connect(const String& ssid, const String& password, uint32_t timeoutMs = 10000);
    
    // Disconnect from WiFi
    void disconnect();
    
    // Check connection status
    bool isConnected();
    WiFiState getState() { return _state; }
    
    // Get connection info
    String getIPAddress();
    int8_t getRSSI();
    
    // Auto-reconnect handling (call in loop)
    void handle();
    
    // Set callbacks
    void onConnected(std::function<void()> callback);
    void onDisconnected(std::function<void()> callback);
    
    // Enable/disable auto-reconnect
    void setAutoReconnect(bool enable) { _autoReconnect = enable; }
    
private:
    WiFiState _state;
    String _ssid;
    String _password;
    bool _autoReconnect;
    uint32_t _lastReconnectAttempt;
    uint8_t _reconnectAttempts;
    
    std::function<void()> _onConnectedCallback;
    std::function<void()> _onDisconnectedCallback;
    
    void setState(WiFiState newState);
    bool attemptConnection();
    
    // Static WiFi event handler
    static void WiFiEvent(WiFiEvent_t event);
    static WiFiManager* _instance;
};

#endif // WIFI_MANAGER_H
