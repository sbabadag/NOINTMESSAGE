/*
 * Fixed LoRa BLE Bridge - Compatible with Mobile App
 * Ensures proper Nordic UART Service UUIDs
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <RadioLib.h>

// Pin definitions for XIAO ESP32-S3
#define LORA_NSS_PIN    D7  // SPI Chip Select  
#define LORA_DIO1_PIN   D3  // DIO1
#define LORA_NRST_PIN   D4  // Reset
#define LORA_BUSY_PIN   D5  // Busy

// BLE Service UUIDs (Nordic UART Service) - MUST match mobile app exactly
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Mobile → ESP32
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // ESP32 → Mobile

// LoRa module 
SX1262 radio = new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_NRST_PIN, LORA_BUSY_PIN);

// BLE variables
bool deviceConnected = false;
NimBLECharacteristic* pTxCharacteristic;
NimBLEServer* pServer = nullptr;
String deviceID = "";

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Serial.println("📱 Mobile app connected via BLE!");
        Serial.println("✅ Ready to receive messages from mobile app");
    };

    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        Serial.println("📱 Mobile app disconnected");
        Serial.println("🔵 Restarting BLE advertising...");
        
        // Small delay before restarting advertising
        delay(500);
        pServer->startAdvertising();
        Serial.println("🔵 BLE advertising restarted - ready for new connections");
    }
};

class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pCharacteristic) {
        String message = pCharacteristic->getValue().c_str();
        
        if (message.length() > 0) {
            // Clean up the message (remove newlines, etc.)
            message.trim();
            
            Serial.println("📱→📡 Received from mobile: '" + message + "'");
            
            // Send via LoRa with device ID
            String loraMsg = "[" + deviceID + "] " + message;
            Serial.println("📡 Transmitting via LoRa: '" + loraMsg + "'");
            
            int state = radio.transmit(loraMsg);
            
            // Send response back to mobile
            String response;
            if (state == RADIOLIB_ERR_NONE) {
                response = "✅ Sent via LoRa: " + message;
                Serial.println("📡 LoRa transmission successful!");
            } else {
                response = "❌ LoRa error " + String(state) + ": " + message;
                Serial.println("📡 LoRa transmission failed with error: " + String(state));
            }
            
            // Send response to mobile app
            if (deviceConnected && pTxCharacteristic) {
                pTxCharacteristic->setValue(response);
                pTxCharacteristic->notify();
                Serial.println("📱 Response sent to mobile: '" + response + "'");
            }
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("🚀 LoRa BLE Bridge v2.0 - Fixed Compatibility");
    Serial.println("==============================================");
    
    // Generate unique device ID from MAC
    uint64_t chipid = ESP.getEfuseMac();
    deviceID = String((uint32_t)(chipid >> 16), HEX);
    deviceID.toUpperCase();
    
    // Initialize LoRa first
    Serial.println("📡 Initializing LoRa module...");
    int state = radio.begin(915.0, 125.0, 7, 5, 0x34, 14, 8, 1.6, false);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("📡 LoRa initialized successfully!");
        Serial.println("   📊 Frequency: 915.0 MHz");
        Serial.println("   📊 Bandwidth: 125.0 kHz"); 
        Serial.println("   📊 Spreading Factor: 7");
        Serial.println("   📊 Power: 14 dBm");
        Serial.println("   📊 Sync Word: 0x34");
    } else {
        Serial.println("📡 LoRa initialization failed with error: " + String(state));
        Serial.println("❌ Check wiring and try again");
    }
    
    // Start receiving LoRa messages
    radio.startReceive();
    Serial.println("📡 LoRa receiver started");
    
    // Initialize BLE with specific device name
    String deviceName = "LoRa_ESP32_" + deviceID;
    Serial.println("🔵 Initializing BLE as: " + deviceName);
    
    NimBLEDevice::init(deviceName);
    
    // Create BLE Server
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    // Create BLE Service with exact UUID from mobile app
    Serial.println("🔵 Creating Nordic UART Service: " + String(SERVICE_UUID));
    NimBLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Create TX Characteristic (ESP32 → Mobile)
    Serial.println("🔵 Creating TX characteristic: " + String(CHARACTERISTIC_UUID_TX));
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
    );
    
    // Create RX Characteristic (Mobile → ESP32)  
    Serial.println("🔵 Creating RX characteristic: " + String(CHARACTERISTIC_UUID_RX));
    NimBLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new CharacteristicCallbacks());
    
    // Start the service
    pService->start();
    Serial.println("🔵 BLE service started successfully");
    
    // Configure and start advertising
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // Functions that help with iPhone connections issue
    pAdvertising->setMaxPreferred(0x12);
    
    pAdvertising->start();
    
    Serial.println("🔵 BLE advertising started successfully");
    Serial.println("📱 Device discoverable as: " + deviceName);
    Serial.println("🆔 Service UUID: " + String(SERVICE_UUID));
    Serial.println("==============================================");
    Serial.println("✅ System ready! Mobile app can now connect.");
    Serial.println("💡 Look for device: " + deviceName);
    Serial.println("==============================================");
}

void loop() {
    // Check for incoming LoRa messages
    String receivedMessage;
    int state = radio.readData(receivedMessage);
    
    if (state == RADIOLIB_ERR_NONE && receivedMessage.length() > 0) {
        receivedMessage.trim(); // Clean up message
        Serial.println("📡→📱 LoRa message received: '" + receivedMessage + "'");
        
        // Forward to mobile app if connected
        if (deviceConnected && pTxCharacteristic) {
            String notification = "📡 " + receivedMessage;
            pTxCharacteristic->setValue(notification);
            pTxCharacteristic->notify();
            Serial.println("📱 LoRa message forwarded to mobile app");
        } else {
            Serial.println("📱 No mobile app connected - message not forwarded");
        }
        
        // Start receiving again
        radio.startReceive();
    }
    
    // Send periodic heartbeat if connected
    static unsigned long lastHeartbeat = 0;
    if (deviceConnected && millis() - lastHeartbeat > 60000) { // Every 60 seconds
        String heartbeat = "💚 " + deviceID + " online - " + String(millis()/1000) + "s uptime";
        pTxCharacteristic->setValue(heartbeat);
        pTxCharacteristic->notify();
        Serial.println("💚 Heartbeat sent to mobile app");
        lastHeartbeat = millis();
    }
    
    // Connection status indicator
    static bool lastConnectionState = false;
    if (deviceConnected != lastConnectionState) {
        if (deviceConnected) {
            Serial.println("🟢 Mobile app connection established");
        } else {
            Serial.println("🔴 Mobile app connection lost");
        }
        lastConnectionState = deviceConnected;
    }
    
    delay(100);
}