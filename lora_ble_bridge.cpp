/*
 * LoRa BLE Bridge for Mobile App
 * ESP32-S3 + SX1262 LoRa Module
 * 
 * This firmware creates a BLE UART service that bridges
 * mobile app messages to LoRa radio transmission
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <RadioLib.h>

// LoRa Module Pin Definitions (XIAO ESP32-S3)
#define LORA_NSS_PIN    D7   // SPI Chip Select
#define LORA_DIO1_PIN   D3   // DIO1 
#define LORA_NRST_PIN   D4   // Reset
#define LORA_BUSY_PIN   D5   // Busy

// BLE UUIDs for UART Service (Nordic UART Service compatible)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// LoRa Configuration
#define LORA_FREQUENCY    915.0  // MHz (adjust for your region)
#define LORA_BANDWIDTH    125.0  // kHz
#define LORA_SPREADING    7      // SF7
#define LORA_CODING_RATE  5      // 4/5
#define LORA_POWER        14     // dBm

// Initialize LoRa module
SX1262 radio = new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_NRST_PIN, LORA_BUSY_PIN);

// BLE Variables
BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
String messageBuffer = "";

// Device Info
String deviceName = "LoRa_ESP32_Bridge";
String deviceID = "";

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("BLE Client Connected");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("BLE Client Disconnected");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            Serial.println("Received from mobile app: " + rxValue);
            
            // Process the received message
            processMessage(rxValue);
        }
    }
};

void processMessage(String message) {
    // Add timestamp and device ID
    String loraMessage = "[" + deviceID + "] " + message;
    
    Serial.println("Sending via LoRa: " + loraMessage);
    
    // Transmit via LoRa
    int state = radio.transmit(loraMessage);
    
    String response = "";
    if (state == RADIOLIB_ERR_NONE) {
        response = "✓ Sent: " + message;
        Serial.println("LoRa transmission successful");
    } else {
        response = "✗ Error: " + String(state);
        Serial.println("LoRa transmission failed: " + String(state));
    }
    
    // Send response back to mobile app
    if (deviceConnected) {
        pTxCharacteristic->setValue(response.c_str());
        pTxCharacteristic->notify();
    }
}

void checkForLoRaMessages() {
    // Check for incoming LoRa messages
    String receivedMessage = "";
    int state = radio.readData(receivedMessage);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Received LoRa message: " + receivedMessage);
        
        // Format for mobile app
        String notification = "📡 " + receivedMessage;
        
        // Send to mobile app if connected
        if (deviceConnected) {
            pTxCharacteristic->setValue(notification.c_str());
            pTxCharacteristic->notify();
        }
    }
}

void initializeLoRa() {
    Serial.println("Initializing LoRa module...");
    
    // Initialize LoRa module
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("LoRa initialization successful");
    } else {
        Serial.println("LoRa initialization failed: " + String(state));
        return;
    }
    
    // Set LoRa parameters
    radio.setFrequency(LORA_FREQUENCY);
    radio.setBandwidth(LORA_BANDWIDTH);
    radio.setSpreadingFactor(LORA_SPREADING);
    radio.setCodingRate(LORA_CODING_RATE);
    radio.setOutputPower(LORA_POWER);
    
    // Set to receive mode
    radio.startReceive();
    
    Serial.println("LoRa configuration complete");
    Serial.println("Frequency: " + String(LORA_FREQUENCY) + " MHz");
    Serial.println("Bandwidth: " + String(LORA_BANDWIDTH) + " kHz");
    Serial.println("Spreading Factor: " + String(LORA_SPREADING));
}

void initializeBLE() {
    // Generate unique device ID
    deviceID = String((uint32_t)ESP.getEfuseMac(), HEX);
    deviceID.toUpperCase();
    String fullDeviceName = deviceName + "_" + deviceID.substring(0, 4);
    
    Serial.println("Initializing BLE: " + fullDeviceName);
    
    // Initialize BLE
    BLEDevice::init(fullDeviceName);
    
    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // Create BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Create TX Characteristic (for sending to mobile app)
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->addDescriptor(new BLE2902());
    
    // Create RX Characteristic (for receiving from mobile app)
    BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE
    );
    pRxCharacteristic->setCallbacks(new MyCallbacks());
    
    // Start the service
    pService->start();
    
    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    
    Serial.println("BLE UART Service started");
    Serial.println("Device is now discoverable as: " + fullDeviceName);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=================================");
    Serial.println("LoRa BLE Bridge - Starting...");
    Serial.println("=================================");
    
    // Initialize LoRa first
    initializeLoRa();
    
    // Initialize BLE
    initializeBLE();
    
    Serial.println("Setup complete - Ready for connections!");
    Serial.println("Mobile app can now scan and connect via BLE");
}

void loop() {
    // Handle BLE connection state changes
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Give the bluetooth stack time to get things ready
        pServer->startAdvertising(); // Restart advertising
        Serial.println("Start advertising again");
        oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        Serial.println("Device connected - ready for messaging");
    }
    
    // Check for incoming LoRa messages
    checkForLoRaMessages();
    
    // Send periodic status if connected
    static unsigned long lastStatus = 0;
    if (deviceConnected && millis() - lastStatus > 30000) { // Every 30 seconds
        String status = "Status: Online, RSSI: " + String(radio.getRSSI()) + " dBm";
        pTxCharacteristic->setValue(status.c_str());
        pTxCharacteristic->notify();
        lastStatus = millis();
    }
    
    delay(100);
}