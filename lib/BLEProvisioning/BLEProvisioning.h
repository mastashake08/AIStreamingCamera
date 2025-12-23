#ifndef BLE_PROVISIONING_H
#define BLE_PROVISIONING_H

#include <NimBLEDevice.h>
#include <Preferences.h>
#include <functional>

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define WIFI_SSID_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define WIFI_PASS_UUID      "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"
#define RTMP_URL_UUID       "d8e3c9f1-4a2b-4c5e-a3f7-8e9d1b2c3d4e"
#define RTMP_KEY_UUID       "a1b2c3d4-e5f6-4728-9a0b-1c2d3e4f5a6b"
#define STATUS_UUID         "e7f8a9b0-c1d2-4e3f-5a6b-7c8d9e0f1a2b"

class BLEProvisioning {
public:
    BLEProvisioning();
    ~BLEProvisioning();
    
    // Initialize BLE server and provisioning service
    bool begin(const char* deviceName);
    
    // Stop BLE to save power after provisioning
    void end();
    
    // Check if credentials are already stored
    bool hasStoredCredentials();
    
    // Load stored credentials
    bool loadWiFiCredentials(String& ssid, String& password);
    bool loadRTMPCredentials(String& url, String& streamKey);
    
    // Clear stored credentials (for reset)
    void clearCredentials();
    
    // Get provisioning status
    bool isProvisioned() { return _provisioned; }
    
    // Set callback for when new credentials are received
    void onCredentialsReceived(std::function<void()> callback);

private:
    NimBLEServer* _pServer;
    NimBLEService* _pService;
    NimBLECharacteristic* _pWiFiSSIDChar;
    NimBLECharacteristic* _pWiFiPassChar;
    NimBLECharacteristic* _pRTMPURLChar;
    NimBLECharacteristic* _pRTMPKeyChar;
    NimBLECharacteristic* _pStatusChar;
    
    Preferences _prefs;
    bool _provisioned;
    std::function<void()> _onCredentialsCallback;
    
    // Internal callback handlers
    class ServerCallbacks;
    class CharacteristicCallbacks;
    
    friend class ServerCallbacks;
    friend class CharacteristicCallbacks;
    
    void saveWiFiCredentials(const String& ssid, const String& password);
    void saveRTMPCredentials(const String& url, const String& streamKey);
    void updateStatus(const char* status);
};

#endif // BLE_PROVISIONING_H
