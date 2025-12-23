#include "WiFiManager.h"
#include "../../include/config.h"

WiFiManager* WiFiManager::_instance = nullptr;

WiFiManager::WiFiManager() 
    : _state(WiFiState::DISCONNECTED),
      _autoReconnect(true),
      _lastReconnectAttempt(0),
      _reconnectAttempts(0) {
    _instance = this;
    
    // Register WiFi event handler
    WiFi.onEvent(WiFiEvent);
}

bool WiFiManager::connect(const String& ssid, const String& password, uint32_t timeoutMs) {
    _ssid = ssid;
    _password = password;
    _reconnectAttempts = 0;
    
    Serial.printf("WiFi: Connecting to '%s'...\n", ssid.c_str());
    setState(WiFiState::CONNECTING);
    
    // Disconnect if already connected
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        delay(100);
    }
    
    // Set WiFi mode and begin connection
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection with timeout
    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeoutMs) {
        delay(100);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        setState(WiFiState::CONNECTED);
        Serial.printf("WiFi: Connected! IP: %s, RSSI: %d dBm\n", 
                     WiFi.localIP().toString().c_str(), WiFi.RSSI());
        
        if (_onConnectedCallback) {
            _onConnectedCallback();
        }
        return true;
    } else {
        setState(WiFiState::FAILED);
        Serial.printf("WiFi: Connection failed (status: %d)\n", WiFi.status());
        return false;
    }
}

void WiFiManager::disconnect() {
    Serial.println("WiFi: Disconnecting...");
    _autoReconnect = false;
    WiFi.disconnect();
    setState(WiFiState::DISCONNECTED);
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED && _state == WiFiState::CONNECTED;
}

String WiFiManager::getIPAddress() {
    return WiFi.localIP().toString();
}

int8_t WiFiManager::getRSSI() {
    return WiFi.RSSI();
}

void WiFiManager::handle() {
    // Auto-reconnect logic
    if (_autoReconnect && !isConnected() && _ssid.length() > 0) {
        uint32_t now = millis();
        
        // Exponential backoff: 5s, 10s, 20s, 40s, then 60s
        uint32_t backoffTime = min(5000 * (1 << _reconnectAttempts), 60000);
        
        if (now - _lastReconnectAttempt >= backoffTime) {
            _lastReconnectAttempt = now;
            _reconnectAttempts++;
            
            if (_reconnectAttempts <= WIFI_MAX_RECONNECT_ATTEMPTS) {
                Serial.printf("WiFi: Reconnect attempt %d/%d...\n", 
                             _reconnectAttempts, WIFI_MAX_RECONNECT_ATTEMPTS);
                attemptConnection();
            } else {
                Serial.println("WiFi: Max reconnect attempts reached");
                setState(WiFiState::FAILED);
                _autoReconnect = false;  // Stop trying
            }
        }
    }
}

void WiFiManager::setState(WiFiState newState) {
    if (_state != newState) {
        _state = newState;
        
        // Reset reconnect counter on successful connection
        if (newState == WiFiState::CONNECTED) {
            _reconnectAttempts = 0;
        }
    }
}

bool WiFiManager::attemptConnection() {
    if (_ssid.length() == 0) {
        return false;
    }
    
    setState(WiFiState::CONNECTING);
    WiFi.begin(_ssid.c_str(), _password.c_str());
    
    // Quick check (non-blocking)
    delay(100);
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::onConnected(std::function<void()> callback) {
    _onConnectedCallback = callback;
}

void WiFiManager::onDisconnected(std::function<void()> callback) {
    _onDisconnectedCallback = callback;
}

void WiFiManager::WiFiEvent(WiFiEvent_t event) {
    if (!_instance) return;
    
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("WiFi: Station connected to AP");
            break;
            
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            _instance->setState(WiFiState::CONNECTED);
            Serial.printf("WiFi: Got IP: %s\n", WiFi.localIP().toString().c_str());
            if (_instance->_onConnectedCallback) {
                _instance->_onConnectedCallback();
            }
            break;
            
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("WiFi: Disconnected from AP");
            _instance->setState(WiFiState::DISCONNECTED);
            if (_instance->_onDisconnectedCallback) {
                _instance->_onDisconnectedCallback();
            }
            break;
            
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            Serial.println("WiFi: Lost IP address");
            break;
            
        default:
            break;
    }
}
