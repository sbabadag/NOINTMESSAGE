#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <RadioLib.h>
#include <SPI.h>
#include <ArduinoJson.h>

// Station ID
#define STATION_ID 1
#define STATION_NAME "M1"

// Function declarations
void sendLoRaMessage(String message);
void sendBLEMessage(String message);
void checkLoRaMessages();

// BLE Configuration
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// LoRa pins - OFFICIAL DATASHEET CONFIGURATION
#define LORA_CS     44  // D7 - NSS (Chip Select)
#define LORA_DIO1   2   // D1 - DIO1 (Interrupt)
#define LORA_RESET  1   // D0 - RESET (Reset pin)
#define LORA_BUSY   3   // D2 - BUSY (Status pin)
#define LORA_SCK    9   // D10 - SCK (SPI Clock)
#define LORA_MISO   8   // D9 - MISO (SPI Data In)
#define LORA_MOSI   7   // D8 - MOSI (SPI Data Out)

// Global objects
BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);
bool loraInitialized = false;

// LoRa interrupt handling
volatile bool receivedFlag = false;
IRAM_ATTR void setFlag(void) {
    receivedFlag = true;
}

// Message queue
String pendingMessage = "";
bool messageReceived = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("📱 Phone connected to M1");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("📱 Phone disconnected from M1");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        String message = String(rxValue.c_str());
        Serial.println("📱➡️ Received from phone: " + message);
        
        // Send via LoRa to other station
        sendLoRaMessage(message);
      }
    }
};

void sendBLEMessage(String message) {
  if (deviceConnected) {
    pTxCharacteristic->setValue(message.c_str());
    pTxCharacteristic->notify();
    Serial.println("📱⬅️ Sent to phone: " + message);
  }
}

void sendLoRaMessage(String message) {
  if (!loraInitialized) {
    Serial.println("❌ LoRa not initialized, message dropped");
    return;
  }
  
  // Create JSON message
  JsonDocument doc;
  doc["from"] = STATION_ID;
  doc["to"] = (STATION_ID == 1) ? 2 : 1;
  doc["msg"] = message;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("📡➡️ Sending via LoRa: " + jsonString);
  int state = radio.transmit(jsonString);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✅ LoRa transmission successful");
  } else {
    Serial.printf("❌ LoRa transmission failed: %d\n", state);
  }
  
  // IMPORTANT: Put radio back in receive mode after transmission
  radio.startReceive();
}

void checkLoRaMessages() {
  if (!loraInitialized) return;
  
  // Check if we received a packet via interrupt
  if (receivedFlag) {
    receivedFlag = false; // Reset flag
    
    // Use String for more reliable reception
    String receivedMessage;
    int state = radio.readData(receivedMessage);
    
    if (state == RADIOLIB_ERR_NONE) {
      if (receivedMessage.length() > 0) {
        Serial.print("📡⬅️ Received via LoRa (");
        Serial.print(receivedMessage.length());
        Serial.print(" bytes): ");
        Serial.println(receivedMessage);
        
        // Parse JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, receivedMessage);
        
        if (!error) {
          int from = doc["from"];
          int to = doc["to"];
          String msg = doc["msg"];
          
          // Check if message is for this station
          if (to == STATION_ID) {
            Serial.println("✅ Message for M1, forwarding to phone");
            sendBLEMessage(msg);
          } else {
            Serial.println("⚠️ Message not for this station");
          }
        } else {
          Serial.println("❌ Failed to parse LoRa JSON message");
        }
      }
    } else {
      Serial.print("❌ LoRa read error: ");
      Serial.println(state);
    }
    
    // Restart reception
    radio.startReceive();
  }
}

void handleSerialInput() {
  if (Serial.available()) {
    String message = Serial.readString();
    message.trim(); // Remove newline characters
    
    if (message.length() > 0) {
      Serial.println("🔧 TEST MESSAGE from M1: " + message);
      sendLoRaMessage(message);
    }
  }
}

void initBLE() {
  BLEDevice::init("M1-LoRa-Bridge");
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE
                      );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("✅ BLE service started - M1 ready for phone connection");
}

void initLoRa() {
  Serial.print("📡 Initializing LoRa... ");
  
  // Initialize SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  
  // Initialize LoRa like working test - simple begin first
  int state = radio.begin();
  
  if (state == RADIOLIB_ERR_NONE) {
    // Configure step by step like working test
    radio.setFrequency(915.0);
    radio.setBandwidth(125.0);
    radio.setSpreadingFactor(7);
    radio.setCodingRate(5);
    radio.setOutputPower(14);
  }
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("SUCCESS ✅");
    loraInitialized = true;
    
    // Set interrupt handler for DIO1
    radio.setDio1Action(setFlag);
    
    // Set radio to receive mode
    radio.startReceive();
    
  } else {
    Serial.printf("FAILED ❌ (Error: %d)\n", state);
    Serial.println("⚠️ M1 running in BLE-only mode");
    loraInitialized = false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║              STATION M1                ║");
  Serial.println("║        Phone ↔ BLE ↔ LoRa ↔ M2        ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  
  Serial.println("🚀 Starting M1 Station...");
  
  // Initialize BLE
  initBLE();
  
  // Initialize LoRa
  initLoRa();
  
  Serial.println();
  Serial.println("✅ M1 Station ready!");
  Serial.println("📱 Connect phone to 'M1-LoRa-Bridge'");
  if (loraInitialized) {
    Serial.println("📡 LoRa ready for M2 communication");
  }
  Serial.println();
}

void loop() {
  // Handle BLE connection status
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("📱 Restarting BLE advertising");
    oldDeviceConnected = deviceConnected;
  }
  
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  // Check for incoming LoRa messages
  if (loraInitialized) {
    checkLoRaMessages();
  }
  
  // Handle serial input for testing
  handleSerialInput();
  
  // Heartbeat
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 5000) {
    Serial.printf("💓 M1: BLE=%s, LoRa=%s (Type message + Enter to test)\n", 
                  deviceConnected ? "Connected" : "Waiting", 
                  loraInitialized ? "Ready" : "Failed");
    lastHeartbeat = millis();
  }
  
  delay(100);
}