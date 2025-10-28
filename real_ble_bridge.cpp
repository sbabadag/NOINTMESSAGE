/*
 * Real LoRa BLE Bridge - Simplified for Quick Deployment
 * Based on your existing working setup
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <RadioLib.h>

// Pin definitions for XIAO ESP32-S3 (matching your working setup)
#define LORA_NSS_PIN    D7  // SPI Chip Select  
#define LORA_DIO1_PIN   D3  // DIO1
#define LORA_NRST_PIN   D4  // Reset
#define LORA_BUSY_PIN   D5  // Busy

// BLE Service UUIDs (Nordic UART Service)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// LoRa module using your working configuration
SX1262 radio = new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_NRST_PIN, LORA_BUSY_PIN);

// BLE variables
bool deviceConnected = false;
NimBLECharacteristic* pTxCharacteristic;
String deviceID = "";

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Serial.println("📱 Mobile app connected via BLE!");
    };

    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        Serial.println("📱 Mobile app disconnected");
        pServer->startAdvertising(); // Restart advertising
    }
};

class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pCharacteristic) {
        String message = pCharacteristic->getValue().c_str();
        
        if (message.length() > 0) {
            Serial.println("📱→📡 Received from mobile: " + message);
            
            // Send via LoRa
            String loraMsg = "[" + deviceID + "] " + message;
            int state = radio.transmit(loraMsg);
            
            // Send response back to mobile
            String response;
            if (state == RADIOLIB_ERR_NONE) {
                response = "✅ Sent via LoRa: " + message;
                Serial.println("📡 LoRa transmission successful");
            } else {
                response = "❌ LoRa error: " + String(state);
                Serial.println("📡 LoRa transmission failed: " + String(state));
            }
            
            // Notify mobile app
            pTxCharacteristic->setValue(response);
            pTxCharacteristic->notify();
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("🚀 Real LoRa BLE Bridge Starting...");
    Serial.println("=====================================");
    
    // Generate device ID
    uint64_t chipid = ESP.getEfuseMac();
    deviceID = String((uint32_t)(chipid >> 16), HEX);
    deviceID.toUpperCase();
    
    // Initialize LoRa
    Serial.println("📡 Initializing LoRa...");
    int state = radio.begin(915.0, 125.0, 7, 5, 0x34, 14, 8, 1.6, false);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("📡 LoRa initialized successfully!");
        Serial.println("   Frequency: 915.0 MHz");
        Serial.println("   Bandwidth: 125.0 kHz"); 
        Serial.println("   Spreading Factor: 7");
        Serial.println("   Power: 14 dBm");
    } else {
        Serial.println("📡 LoRa initialization failed: " + String(state));
    }
    
    // Start receiving
    radio.startReceive();
    
    // Initialize BLE
    String deviceName = "LoRa_ESP32_" + deviceID;
    Serial.println("🔵 Starting BLE: " + deviceName);
    
    NimBLEDevice::init(deviceName);
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    NimBLEService *pService = pServer->createService(SERVICE_UUID);
    
    // TX Characteristic (ESP32 → Mobile)
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        NIMBLE_PROPERTY::NOTIFY
    );
    
    // RX Characteristic (Mobile → ESP32)
    NimBLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE
    );
    pRxCharacteristic->setCallbacks(new CharacteristicCallbacks());
    
    pService->start();
    
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    
    Serial.println("🔵 BLE advertising started");
    Serial.println("📱 Mobile apps can now discover: " + deviceName);
    Serial.println("=====================================");
    Serial.println("✅ System ready for connections!");
}

void loop() {
    // Check for incoming LoRa messages
    String receivedMessage;
    int state = radio.readData(receivedMessage);
    
    if (state == RADIOLIB_ERR_NONE && receivedMessage.length() > 0) {
        Serial.println("📡→📱 Received LoRa: " + receivedMessage);
        
        // Forward to mobile app if connected
        if (deviceConnected) {
            String notification = "📡 " + receivedMessage;
            pTxCharacteristic->setValue(notification);
            pTxCharacteristic->notify();
            Serial.println("📱 Forwarded to mobile app");
        }
        
        // Start receiving again
        radio.startReceive();
    }
    
    // Send periodic status updates
    static unsigned long lastStatus = 0;
    if (deviceConnected && millis() - lastStatus > 30000) {
        String status = "💚 Device " + deviceID + " online";
        pTxCharacteristic->setValue(status);
        pTxCharacteristic->notify();
        lastStatus = millis();
    }
    
    delay(100);
}