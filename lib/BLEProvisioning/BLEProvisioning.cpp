#include "BLEProvisioning.h"
#include "../../include/config.h"

// Server callbacks
class BLEProvisioning::ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        Serial.println("BLE: Client connected");
    }
    
    void onDisconnect(NimBLEServer* pServer) {
        Serial.println("BLE: Client disconnected");
        // Restart advertising
        pServer->startAdvertising();
    }
};

// Characteristic write callbacks
class BLEProvisioning::CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
    CharacteristicCallbacks(BLEProvisioning* parent) : _parent(parent) {}
    
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string uuid = pCharacteristic->getUUID().toString();
        std::string value = pCharacteristic->getValue();
        
        Serial.printf("BLE: Received data for UUID: %s\n", uuid.c_str());
        
        if (uuid == WIFI_SSID_UUID) {
            _wifiSSID = String(value.c_str());
            Serial.printf("BLE: WiFi SSID set: %s\n", _wifiSSID.c_str());
        }
        else if (uuid == WIFI_PASS_UUID) {
            _wifiPass = String(value.c_str());
            Serial.println("BLE: WiFi password set");
            
            // Save WiFi credentials when password is received
            if (_wifiSSID.length() > 0) {
                _parent->saveWiFiCredentials(_wifiSSID, _wifiPass);
                _parent->updateStatus("wifi_saved");
            }
        }
        else if (uuid == RTMP_URL_UUID) {
            _rtmpURL = String(value.c_str());
            Serial.printf("BLE: RTMP URL set: %s\n", _rtmpURL.c_str());
        }
        else if (uuid == RTMP_KEY_UUID) {
            _rtmpKey = String(value.c_str());
            Serial.println("BLE: RTMP key set");
            
            // Save RTMP credentials when key is received
            if (_rtmpURL.length() > 0) {
                _parent->saveRTMPCredentials(_rtmpURL, _rtmpKey);
                _parent->updateStatus("rtmp_saved");
                
                // All credentials received
                if (_wifiSSID.length() > 0) {
                    _parent->_provisioned = true;
                    _parent->updateStatus("provisioned");
                    if (_parent->_onCredentialsCallback) {
                        _parent->_onCredentialsCallback();
                    }
                }
            }
        }
    }
    
private:
    BLEProvisioning* _parent;
    String _wifiSSID;
    String _wifiPass;
    String _rtmpURL;
    String _rtmpKey;
};

BLEProvisioning::BLEProvisioning() 
    : _pServer(nullptr), _pService(nullptr), _provisioned(false) {
}

BLEProvisioning::~BLEProvisioning() {
    end();
}

bool BLEProvisioning::begin(const char* deviceName) {
    Serial.println("BLE: Initializing provisioning service...");
    
    // Check if already provisioned
    _provisioned = hasStoredCredentials();
    
    // Initialize NimBLE
    NimBLEDevice::init(deviceName);
    
    // Create BLE Server
    _pServer = NimBLEDevice::createServer();
    _pServer->setCallbacks(new ServerCallbacks());
    
    // Create BLE Service
    _pService = _pServer->createService(SERVICE_UUID);
    
    // Create characteristics with write permissions
    _pWiFiSSIDChar = _pService->createCharacteristic(
        WIFI_SSID_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ
    );
    _pWiFiSSIDChar->setCallbacks(new CharacteristicCallbacks(this));
    
    _pWiFiPassChar = _pService->createCharacteristic(
        WIFI_PASS_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    _pWiFiPassChar->setCallbacks(new CharacteristicCallbacks(this));
    
    _pRTMPURLChar = _pService->createCharacteristic(
        RTMP_URL_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ
    );
    _pRTMPURLChar->setCallbacks(new CharacteristicCallbacks(this));
    
    _pRTMPKeyChar = _pService->createCharacteristic(
        RTMP_KEY_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    _pRTMPKeyChar->setCallbacks(new CharacteristicCallbacks(this));
    
    _pStatusChar = _pService->createCharacteristic(
        STATUS_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    
    // Start service
    _pService->start();
    
    // Start advertising
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // Help with iPhone connections
    pAdvertising->setMaxPreferred(0x12);
    
    NimBLEDevice::startAdvertising();
    
    Serial.printf("BLE: Advertising as '%s'\n", deviceName);
    updateStatus(_provisioned ? "already_provisioned" : "awaiting_config");
    
    return true;
}

void BLEProvisioning::end() {
    if (_pServer) {
        NimBLEDevice::deinit(true);
        _pServer = nullptr;
        _pService = nullptr;
        Serial.println("BLE: Service stopped");
    }
}

bool BLEProvisioning::hasStoredCredentials() {
    _prefs.begin(NVS_NAMESPACE_WIFI, true);  // Read-only
    bool hasWiFi = _prefs.isKey("ssid") && _prefs.isKey("password");
    _prefs.end();
    
    _prefs.begin(NVS_NAMESPACE_RTMP, true);  // Read-only
    bool hasRTMP = _prefs.isKey("url") && _prefs.isKey("key");
    _prefs.end();
    
    return hasWiFi && hasRTMP;
}

bool BLEProvisioning::loadWiFiCredentials(String& ssid, String& password) {
    _prefs.begin(NVS_NAMESPACE_WIFI, true);
    
    if (_prefs.isKey("ssid") && _prefs.isKey("password")) {
        ssid = _prefs.getString("ssid", "");
        password = _prefs.getString("password", "");
        _prefs.end();
        return true;
    }
    
    _prefs.end();
    return false;
}

bool BLEProvisioning::loadRTMPCredentials(String& url, String& streamKey) {
    _prefs.begin(NVS_NAMESPACE_RTMP, true);
    
    if (_prefs.isKey("url") && _prefs.isKey("key")) {
        url = _prefs.getString("url", "");
        streamKey = _prefs.getString("key", "");
        _prefs.end();
        return true;
    }
    
    _prefs.end();
    return false;
}

void BLEProvisioning::saveWiFiCredentials(const String& ssid, const String& password) {
    _prefs.begin(NVS_NAMESPACE_WIFI, false);
    _prefs.putString("ssid", ssid);
    _prefs.putString("password", password);
    _prefs.end();
    Serial.println("BLE: WiFi credentials saved to NVS");
}

void BLEProvisioning::saveRTMPCredentials(const String& url, const String& streamKey) {
    _prefs.begin(NVS_NAMESPACE_RTMP, false);
    _prefs.putString("url", url);
    _prefs.putString("key", streamKey);
    _prefs.end();
    Serial.println("BLE: RTMP credentials saved to NVS");
}

void BLEProvisioning::clearCredentials() {
    _prefs.begin(NVS_NAMESPACE_WIFI, false);
    _prefs.clear();
    _prefs.end();
    
    _prefs.begin(NVS_NAMESPACE_RTMP, false);
    _prefs.clear();
    _prefs.end();
    
    _provisioned = false;
    Serial.println("BLE: All credentials cleared");
}

void BLEProvisioning::updateStatus(const char* status) {
    if (_pStatusChar) {
        _pStatusChar->setValue(status);
        _pStatusChar->notify();
    }
}

void BLEProvisioning::onCredentialsReceived(std::function<void()> callback) {
    _onCredentialsCallback = callback;
}
