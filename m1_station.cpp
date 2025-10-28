// BIDIRECTIONAL MESSAGE TUNNEL - M1 STATION  
// BLE + LoRa bidirectional messaging

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <NimBLEDevice.h>

// Pin mapping for XIAO ESP32S3 + Wio SX1262
constexpr uint8_t PIN_LORA_NSS   = D7;  // GPIO44
constexpr uint8_t PIN_LORA_DIO1  = D1;  // GPIO2  
constexpr uint8_t PIN_LORA_RESET = D0;  // GPIO1
constexpr uint8_t PIN_LORA_BUSY  = D2;  // GPIO3
constexpr uint8_t PIN_LORA_SCK   = D10; // GPIO9
constexpr uint8_t PIN_LORA_MISO  = D9;  // GPIO8
constexpr uint8_t PIN_LORA_MOSI  = D8;  // GPIO7

// LoRa Configuration (must match M2)
constexpr float LORA_FREQ = 915.0;
constexpr float LORA_BW = 125.0;
constexpr uint8_t LORA_SF = 7;
constexpr uint8_t LORA_CR = 5;
constexpr int8_t LORA_POWER = 22;

// BLE Configuration for M1
const char* BLE_DEVICE_NAME = "M1";
const char* SERVICE_UUID = "12345678-1234-1234-1234-123456789abc";
const char* CHAR_RX_MESSAGE_UUID = "12345678-1234-1234-1234-123456789abd"; // Phone receives messages here
const char* CHAR_TX_MESSAGE_UUID = "12345678-1234-1234-1234-123456789abe"; // Phone sends messages here
const char* CHAR_STATUS_UUID = "12345678-1234-1234-1234-123456789abf";     // Status updates

// Message settings
constexpr uint16_t MAX_MESSAGE_LEN = 200;

SX1262 lora = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RESET, PIN_LORA_BUSY);
bool loraReady = false;
uint32_t messageCount = 0;

// Message packet structure (must match M2)
struct MessagePacket {
  uint32_t timestamp;
  uint16_t messageLen;
  char message[MAX_MESSAGE_LEN];
} __attribute__((packed));

// BLE objects
NimBLEServer* pServer = nullptr;
NimBLEService* pService = nullptr;
NimBLECharacteristic* pCharRxMessage = nullptr;  // Messages TO phone
NimBLECharacteristic* pCharTxMessage = nullptr;  // Messages FROM phone
NimBLECharacteristic* pCharStatus = nullptr;
bool deviceConnected = false;

// BLE Server Callbacks
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    deviceConnected = true;
    Serial.println("\n📱 Phone connected to M1");
    
    if (pCharStatus) {
      pCharStatus->setValue("M1_CONNECTED");
      pCharStatus->notify();
    }
  }
  
  void onDisconnect(NimBLEServer* pServer) {
    deviceConnected = false;
    Serial.println("📱 Phone disconnected from M1");
    pServer->startAdvertising();
  }
};

// Callback for messages FROM phone
class MessageCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    if (value.length() > 0) {
      Serial.println("\n╔═══════════════════════════════════╗");
      Serial.println("║  📱 MESSAGE FROM PHONE (M1)       ║");
      Serial.println("╚═══════════════════════════════════╝");
      Serial.printf("Message: \"%s\"\n", value.c_str());
      Serial.println("═══════════════════════════════════\n");
      
      // Forward to LoRa (to M2)
      sendMessageViaLoRa(value.c_str());
    }
  }
};

// Initialize BLE
void initBLE() {
  Serial.print("Initializing BLE as M1... ");
  
  NimBLEDevice::init(BLE_DEVICE_NAME);
  NimBLEDevice::setMTU(512);
  
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  pService = pServer->createService(SERVICE_UUID);
  
  // RX Characteristic - Phone receives messages from LoRa here
  pCharRxMessage = pService->createCharacteristic(
    CHAR_RX_MESSAGE_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  
  // TX Characteristic - Phone sends messages via this
  pCharTxMessage = pService->createCharacteristic(
    CHAR_TX_MESSAGE_UUID,
    NIMBLE_PROPERTY::WRITE
  );
  pCharTxMessage->setCallbacks(new MessageCallbacks());
  
  // Status Characteristic
  pCharStatus = pService->createCharacteristic(
    CHAR_STATUS_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  
  pService->start();
  
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
  
  Serial.println("✓ OK");
  Serial.printf("BLE Name: %s\n", BLE_DEVICE_NAME);
}

// Send message to phone via BLE
void sendMessageToPhone(const char* message, uint32_t timestamp) {
  if (!deviceConnected) {
    Serial.println("⚠ No phone connected to M1");
    return;
  }
  
  if (pCharRxMessage) {
    // Create formatted message with timestamp
    char formatted[MAX_MESSAGE_LEN + 50];
    unsigned long seconds = timestamp / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    snprintf(formatted, sizeof(formatted), "[M2→M1 %02lu:%02lu:%02lu] %s", 
             hours % 24, minutes % 60, seconds % 60, message);
    
    pCharRxMessage->setValue(formatted);
    pCharRxMessage->notify();
    
    Serial.println("📱 Message forwarded to phone (M1)");
  }
  
  // Update status
  if (pCharStatus) {
    char status[64];
    snprintf(status, sizeof(status), "M1_MESSAGES:%u", messageCount);
    pCharStatus->setValue(status);
    pCharStatus->notify();
  }
}

// Send message FROM phone via LoRa (M1 → M2)
void sendMessageViaLoRa(const char* text) {
  if (!loraReady) {
    Serial.println("✗ LoRa not ready for transmission");
    return;
  }
  
  uint16_t len = strlen(text);
  if (len == 0 || len > MAX_MESSAGE_LEN) {
    Serial.println("✗ Message too long or empty");
    return;
  }
  
  MessagePacket packet;
  packet.timestamp = millis();
  packet.messageLen = len;
  strncpy(packet.message, text, MAX_MESSAGE_LEN);
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  📤 M1 → M2 VIA LORA             ║");
  Serial.println("╚═══════════════════════════════════╝");
  Serial.printf("Message: \"%s\"\n", text);
  Serial.printf("Length: %d bytes\n", len);
  
  // Stop receiving to transmit
  lora.standby();
  
  // Transmit
  int16_t state = lora.transmit((uint8_t*)&packet, sizeof(uint32_t) + sizeof(uint16_t) + len);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✓ Message sent successfully to M2!");
    Serial.printf("Time on air: %d ms\n", lora.getTimeOnAir(len + 6));
  } else {
    Serial.printf("✗ LoRa transmission failed, error: %d\n", state);
  }
  
  Serial.println("═══════════════════════════════════\n");
  
  // Resume receiving
  lora.startReceive();
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  BIDIRECTIONAL MESSAGE TUNNEL     ║");
  Serial.println("║             M1 STATION             ║");
  Serial.println("╚═══════════════════════════════════╝\n");
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Initialize BLE first
  initBLE();
  
  // Initialize LoRa
  Serial.print("Initializing SX1262... ");
  int16_t state = lora.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, 0x12, LORA_POWER);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✓ OK");
    loraReady = true;
    
    lora.setCurrentLimit(60.0);
    lora.setCRC(true);
    
    // Start in receive mode
    lora.startReceive();
    
    Serial.println("\n┌─────────────────────────────────┐");
    Serial.printf("│ Frequency: %.1f MHz             │\n", LORA_FREQ);
    Serial.printf("│ Bandwidth: %.1f kHz             │\n", LORA_BW);
    Serial.printf("│ Spreading Factor: %d             │\n", LORA_SF);
    Serial.printf("│ TX Power: %d dBm                │\n", LORA_POWER);
    Serial.println("└─────────────────────────────────┘");
    
  } else {
    Serial.printf("✗ Failed (error %d)\n", state);
    Serial.println("Check wiring and restart");
    loraReady = false;
  }
  
  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("📱 M1: Connect phone via BLE");
  Serial.println("📥 M1: Listening for M2 messages...");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void loop() {
  static uint32_t lastBlink = 0;
  static bool ledState = false;
  
  // LED heartbeat (faster when phone connected)
  uint32_t blinkInterval = deviceConnected ? 200 : 500;
  if (millis() - lastBlink >= blinkInterval) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    lastBlink = millis();
  }
  
  // Check for incoming LoRa messages from M2
  if (loraReady) {
    int16_t state = lora.scanChannel();
    
    if (state == RADIOLIB_ERR_NONE) {
      uint8_t buffer[sizeof(MessagePacket)];
      int len = lora.getPacketLength();
      
      if (len > 0 && len <= sizeof(buffer)) {
        state = lora.readData(buffer, len);
        
        if (state == RADIOLIB_ERR_NONE && len >= sizeof(uint32_t) + sizeof(uint16_t)) {
          MessagePacket* packet = (MessagePacket*)buffer;
          
          // Validate message length
          if (packet->messageLen > 0 && packet->messageLen <= MAX_MESSAGE_LEN) {
            // Null-terminate message
            packet->message[packet->messageLen] = '\0';
            
            messageCount++;
            
            Serial.println("\n╔═══════════════════════════════════╗");
            Serial.println("║  📨 MESSAGE FROM M2               ║");
            Serial.println("╚═══════════════════════════════════╝");
            Serial.printf("Message #%u\n", messageCount);
            Serial.printf("From: M2 Station\n");
            Serial.printf("Text: \"%s\"\n", packet->message);
            Serial.printf("RSSI: %d dBm\n", lora.getRSSI());
            Serial.printf("SNR: %.2f dB\n", lora.getSNR());
            Serial.println("═══════════════════════════════════\n");
            
            // Send to phone via BLE
            sendMessageToPhone(packet->message, packet->timestamp);
            
            // Brief flash to indicate message received
            for (int i = 0; i < 3; i++) {
              digitalWrite(LED_BUILTIN, HIGH);
              delay(50);
              digitalWrite(LED_BUILTIN, LOW);
              delay(50);
            }
          }
        }
      }
      
      // Restart receive
      lora.startReceive();
    }
  }
}