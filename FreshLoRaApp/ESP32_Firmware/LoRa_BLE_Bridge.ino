/*
 * ESP32 LoRa BLE Bridge Firmware
 * Compatible with React Native LoRa Chat App
 * 
 * Hardware: ESP32 + SX1262 LoRa Module
 * Purpose: Bridge BLE ↔ LoRa for bidirectional messaging
 * Stations: M1 and M2 (set via STATION_TYPE)
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <RadioLib.h>

// ==================== CONFIGURATION ====================

// Station Configuration - CHANGE THIS FOR EACH DEVICE
#define STATION_TYPE "M1"  // "M1" or "M2"
#define DEVICE_NAME STATION_TYPE  // BLE advertised name

// BLE Configuration (Nordic UART Service)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // ESP32 receives from app
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // ESP32 sends to app

// LoRa Configuration
#define LORA_SCK     5
#define LORA_MISO    19
#define LORA_MOSI    27
#define LORA_CS      18
#define LORA_RST     14
#define LORA_DIO1    26
#define LORA_DIO2    35
#define LORA_DIO3    34

// LoRa Settings
#define LORA_FREQ    868.0  // MHz (change to 915.0 for US)
#define LORA_SF      7      // Spreading Factor
#define LORA_BW      125.0  // Bandwidth in kHz
#define LORA_CR      5      // Coding Rate 4/5
#define LORA_POWER   14     // TX Power in dBm

// Message Settings
#define MAX_MESSAGE_SIZE 200
#define JSON_BUFFER_SIZE 512

// ==================== GLOBAL VARIABLES ====================

// BLE
BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic = NULL;
BLECharacteristic* pRxCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// LoRa
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_DIO2);

// Message Queue
struct Message {
  String messageId;
  String text;
  String timestamp;
  String sender;
  String stationType;
  String targetStation;
};

// Station Info
String myStation = STATION_TYPE;
String remoteStation = (String(STATION_TYPE) == "M1") ? "M2" : "M1";

// ==================== BLE CALLBACKS ====================

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("📱 Phone connected via BLE");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("📱 Phone disconnected from BLE");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String receivedData = pCharacteristic->getValue().c_str();
      
      if (receivedData.length() > 0) {
        Serial.println("📨 Received from phone: " + receivedData);
        processIncomingBLEMessage(receivedData);
      }
    }
};

// ==================== SETUP FUNCTIONS ====================

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("🚀 ESP32 LoRa BLE Bridge Starting...");
  Serial.println("📡 Station: " + myStation);
  Serial.println("🎯 Target: " + remoteStation);
  
  // Initialize LoRa
  setupLoRa();
  
  // Initialize BLE
  setupBLE();
  
  Serial.println("✅ System ready for bidirectional messaging!");
}

void setupLoRa() {
  Serial.println("📡 Initializing LoRa module...");
  
  // Initialize SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  
  // Initialize LoRa module
  int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, 0x12, LORA_POWER, 8, 1.6);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✅ LoRa module initialized successfully");
    Serial.printf("📊 Freq: %.1f MHz, SF: %d, BW: %.1f kHz, Power: %d dBm\n", 
                  LORA_FREQ, LORA_SF, LORA_BW, LORA_POWER);
  } else {
    Serial.printf("❌ LoRa initialization failed, code: %d\n", state);
    while(true) delay(1000);
  }
  
  // Set interrupt for incoming LoRa messages
  radio.setDio1Action(onLoRaReceive);
  
  // Start listening
  radio.startReceive();
  Serial.println("👂 LoRa listening mode activated");
}

void setupBLE() {
  Serial.println("📱 Initializing BLE server...");
  
  // Create BLE Device
  BLEDevice::init(DEVICE_NAME);
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create TX Characteristic (ESP32 → Phone)
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());
  
  // Create RX Characteristic (Phone → ESP32)
  pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE
                      );
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  
  // Start service
  pService->start();
  
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  
  Serial.println("✅ BLE server ready - device discoverable as: " + String(DEVICE_NAME));
}

// ==================== MESSAGE PROCESSING ====================

void processIncomingBLEMessage(String jsonMessage) {
  Serial.println("🔄 Processing BLE message for LoRa relay...");
  
  // Parse JSON
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  DeserializationError error = deserializeJson(doc, jsonMessage);
  
  if (error) {
    Serial.println("❌ JSON parsing failed: " + String(error.c_str()));
    return;
  }
  
  // Extract message data
  Message msg;
  msg.messageId = doc["messageId"].as<String>();
  msg.text = doc["text"].as<String>();
  msg.timestamp = doc["timestamp"].as<String>();
  msg.sender = doc["sender"].as<String>();
  msg.stationType = myStation;
  msg.targetStation = remoteStation;
  
  Serial.println("📤 Relaying to " + remoteStation + ": " + msg.text);
  
  // Send via LoRa
  sendLoRaMessage(msg);
}

void sendLoRaMessage(Message msg) {
  // Create LoRa packet JSON
  DynamicJsonDocument loraDoc(JSON_BUFFER_SIZE);
  loraDoc["messageId"] = msg.messageId;
  loraDoc["text"] = msg.text;
  loraDoc["timestamp"] = msg.timestamp;
  loraDoc["sender"] = msg.sender;
  loraDoc["fromStation"] = msg.stationType;
  loraDoc["toStation"] = msg.targetStation;
  loraDoc["route"] = msg.stationType + "→" + msg.targetStation;
  
  String loraPacket;
  serializeJson(loraDoc, loraPacket);
  
  Serial.println("📡 Sending LoRa packet: " + loraPacket);
  
  // Send LoRa packet
  int state = radio.transmit(loraPacket);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✅ LoRa message sent successfully");
  } else {
    Serial.printf("❌ LoRa transmission failed, code: %d\n", state);
  }
  
  // Return to receive mode
  radio.startReceive();
}

// ==================== LORA RECEIVE HANDLER ====================

volatile bool receivedFlag = false;

void onLoRaReceive(void) {
  receivedFlag = true;
}

void handleLoRaMessage() {
  if (!receivedFlag) return;
  receivedFlag = false;
  
  String receivedPacket;
  int state = radio.readData(receivedPacket);
  
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("❌ LoRa receive failed, code: %d\n", state);
    radio.startReceive();
    return;
  }
  
  Serial.println("📡 Received LoRa packet: " + receivedPacket);
  float rssi = radio.getRSSI();
  float snr = radio.getSNR();
  Serial.printf("📊 RSSI: %.2f dBm, SNR: %.2f dB\n", rssi, snr);
  
  // Parse LoRa packet
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  DeserializationError error = deserializeJson(doc, receivedPacket);
  
  if (error) {
    Serial.println("❌ LoRa packet JSON parsing failed");
    radio.startReceive();
    return;
  }
  
  // Check if message is for this station
  String toStation = doc["toStation"].as<String>();
  if (toStation != myStation) {
    Serial.println("📭 Message not for this station (" + toStation + " != " + myStation + ")");
    radio.startReceive();
    return;
  }
  
  Serial.println("📬 Message is for this station - relaying to phone");
  
  // Relay to connected phone via BLE
  relayLoRaMessageToBLE(doc);
  
  // Return to receive mode
  radio.startReceive();
}

void relayLoRaMessageToBLE(DynamicJsonDocument& loraMsg) {
  if (!deviceConnected) {
    Serial.println("📱 No phone connected - message lost");
    return;
  }
  
  // Create message for phone (compatible with app format)
  DynamicJsonDocument phoneMsg(JSON_BUFFER_SIZE);
  phoneMsg["text"] = loraMsg["text"];
  phoneMsg["sender"] = "remote";
  phoneMsg["timestamp"] = loraMsg["timestamp"];
  phoneMsg["deviceId"] = (loraMsg["fromStation"] == "M1") ? 1 : 2;
  phoneMsg["stationType"] = loraMsg["fromStation"];
  phoneMsg["route"] = loraMsg["route"];
  phoneMsg["rssi"] = radio.getRSSI();
  phoneMsg["snr"] = radio.getSNR();
  
  String phoneMessage;
  serializeJson(phoneMsg, phoneMessage);
  
  Serial.println("📱 Sending to phone: " + phoneMessage);
  
  // Send via BLE notification
  pTxCharacteristic->setValue(phoneMessage.c_str());
  pTxCharacteristic->notify();
  
  Serial.println("✅ Message relayed to phone successfully");
}

// ==================== MAIN LOOP ====================

void loop() {
  // Handle LoRa messages
  handleLoRaMessage();
  
  // Handle BLE connection changes
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give the bluetooth stack time to get ready
    pServer->startAdvertising(); // Restart advertising
    Serial.println("📱 Started advertising for new connection");
    oldDeviceConnected = deviceConnected;
  }
  
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}

// ==================== DIAGNOSTIC FUNCTIONS ====================

void printSystemStatus() {
  Serial.println("\n=== SYSTEM STATUS ===");
  Serial.println("Station: " + myStation);
  Serial.println("BLE Connected: " + String(deviceConnected ? "Yes" : "No"));
  Serial.println("LoRa Status: Active");
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("===================\n");
}

// Call this function periodically in main loop for debugging
void periodicStatusReport() {
  static unsigned long lastReport = 0;
  if (millis() - lastReport > 30000) { // Every 30 seconds
    printSystemStatus();
    lastReport = millis();
  }
}