// MESSAGE TUNNEL RECEIVER WITH BLE - Forward LoRa messages to phone
#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <NimBLEDevice.h>

// Pin mapping for XIAO ESP32S3 + Wio SX1262
#define PIN_LORA_NSS   44  // D7
#define PIN_LORA_DIO1  2   // D1  
#define PIN_LORA_RESET 1   // D0
#define PIN_LORA_BUSY  3   // D2
#define PIN_LORA_SCK   9   // D10
#define PIN_LORA_MISO  8   // D9
#define PIN_LORA_MOSI  7   // D8

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID        "12345678-1234-5678-9abc-def012345678"
#define CHAR_RX_MESSAGE_UUID "12345678-1234-5678-9abc-def012345789"

SX1262 lora = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RESET, PIN_LORA_BUSY);
bool loraReady = false;

// BLE variables
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharRxMessage = nullptr;
bool deviceConnected = false;
uint32_t messageCount = 0;

class ServerCallbacks: public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    deviceConnected = true;
    Serial.println("📱 Phone connected via BLE!");
  }

  void onDisconnect(NimBLEServer* pServer) {
    deviceConnected = false;
    Serial.println("📱 Phone disconnected");
    // Restart advertising
    NimBLEDevice::startAdvertising();
  }
};

void setupBLE() {
  Serial.println("🔵 Initializing Bluetooth...");
  
  NimBLEDevice::init("MessageTunnel");
  
  // Create BLE Server
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  // Create BLE Service
  NimBLEService *pService = pServer->createService(SERVICE_UUID);
  
  // RX Characteristic - Receiver sends messages to phone via this
  pCharRxMessage = pService->createCharacteristic(
    CHAR_RX_MESSAGE_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  
  // Start the service
  pService->start();
  
  // Start advertising
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  NimBLEDevice::startAdvertising();
  
  Serial.println("✅ BLE ready - Device name: 'MessageTunnel'");
  Serial.println("📱 Use nRF Connect app to connect and subscribe to notifications");
}

void forwardToPhone(const String& message, float rssi, float snr) {
  if (deviceConnected && pCharRxMessage) {
    // Create formatted message with timestamp and signal info
    String formattedMsg = String(millis()) + "|" + message + "|" + String(rssi, 1) + "|" + String(snr, 1);
    
    pCharRxMessage->setValue(formattedMsg.c_str());
    pCharRxMessage->notify();
    
    Serial.println("📱 Message forwarded to phone via BLE");
  } else {
    Serial.println("📱 No phone connected - message not forwarded");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔══════════════════════════════════════╗");
  Serial.println("║   MESSAGE TUNNEL RECEIVER + BLE      ║");
  Serial.println("╚══════════════════════════════════════╝");
  
  // Initialize BLE
  setupBLE();
  
  // Initialize SPI
  Serial.print("📡 Initializing SPI... ");
  SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI);
  delay(100);
  Serial.println("OK");
  
  // Initialize LoRa
  Serial.print("📡 Initializing LoRa... ");
  int state = lora.begin(915.0, 125.0, 7, 5, 0x12, 22);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("OK!");
    loraReady = true;
    
    // Start listening
    Serial.print("📡 Starting LoRa receive... ");
    state = lora.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("OK!");
    } else {
      Serial.printf("Failed (%d)\n", state);
    }
    
  } else {
    Serial.printf("FAILED (error %d)\n", state);
    // Try simpler initialization
    Serial.print("📡 Trying alternative LoRa init... ");
    state = lora.begin(915.0);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("OK!");
      loraReady = true;
      lora.startReceive();
    } else {
      Serial.printf("Still failed (%d)\n", state);
    }
  }
  
  if (loraReady) {
    Serial.println("\n🎯 System ready!");
    Serial.println("📡 Listening for LoRa messages...");
    Serial.println("🔵 Broadcasting 'MessageTunnel' via Bluetooth");
    Serial.println("📱 Connect with nRF Connect app to receive messages");
  } else {
    Serial.println("\n❌ LoRa not working - check wiring");
  }
  
  Serial.println("═══════════════════════════════════════════\n");
}

void loop() {
  static uint32_t lastHeartbeat = 0;
  
  // Heartbeat LED
  if (millis() - lastHeartbeat >= 1000) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    lastHeartbeat = millis();
  }
  
  if (loraReady) {
    // Check for received messages
    if (lora.getPacketLength() > 0) {
      String message;
      int state = lora.readData(message);
      
      if (state == RADIOLIB_ERR_NONE && message.length() > 0) {
        messageCount++;
        float rssi = lora.getRSSI();
        float snr = lora.getSNR();
        
        Serial.println("\n┌─────────────────────────────────────┐");
        Serial.printf("│     MESSAGE #%-4d RECEIVED          │\n", messageCount);
        Serial.println("└─────────────────────────────────────┘");
        Serial.printf("📨 Message: \"%s\"\n", message.c_str());
        Serial.printf("📊 RSSI: %.1f dBm\n", rssi);
        Serial.printf("📊 SNR: %.1f dB\n", snr);
        Serial.printf("📏 Length: %d bytes\n", message.length());
        
        // Forward to phone via BLE
        forwardToPhone(message, rssi, snr);
        
        Serial.println("═══════════════════════════════════════\n");
        
        // Restart listening
        lora.startReceive();
        
      } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
        Serial.printf("📡 Read failed (%d)\n", state);
        lora.startReceive();
      }
    }
  }
  
  delay(10);
}