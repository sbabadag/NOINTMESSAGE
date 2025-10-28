// BIDIRECTIONAL TRANSCEIVER WITH BLE - Each device has unique BLE name
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

// Set unique device ID for each board
// Change this to 1 for first device, 2 for second device
#define DEVICE_ID 1  // *** CHANGE THIS TO 2 FOR THE OTHER DEVICE ***

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID        "12345678-1234-5678-9abc-def012345678"
#define CHAR_RX_MESSAGE_UUID "12345678-1234-5678-9abc-def012345789"
#define CHAR_TX_MESSAGE_UUID "12345678-1234-5678-9abc-def012345790"

SX1262 lora = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RESET, PIN_LORA_BUSY);
bool loraReady = false;
uint32_t messageCount = 0;
String inputBuffer = "";

// BLE variables
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharRxMessage = nullptr;
NimBLECharacteristic* pCharTxMessage = nullptr;
bool deviceConnected = false;

// Device names based on ID
const char* deviceNames[] = {"", "LoRa_Station_1", "LoRa_Station_2"};
const char* getDeviceName() {
  return deviceNames[DEVICE_ID];
}

struct MessagePacket {
  uint8_t fromDevice;    // Which device sent this
  uint8_t toDevice;      // Which device this is for (0 = broadcast)
  uint32_t messageId;    // Unique message ID
  uint16_t messageLen;   // Length of message
  char message[180];     // Message text
} __attribute__((packed));

class ServerCallbacks: public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    deviceConnected = true;
    Serial.printf("📱 Phone connected to %s via BLE!\n", getDeviceName());
  }

  void onDisconnect(NimBLEServer* pServer) {
    deviceConnected = false;
    Serial.printf("📱 Phone disconnected from %s\n", getDeviceName());
    // Restart advertising
    NimBLEDevice::startAdvertising();
  }
};

class MessageCallbacks: public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic) {
    String value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.printf("📱 Message from phone: \"%s\"\n", value.c_str());
      // Send via LoRa
      sendMessage(value);
    }
  }
};

void setupBLE() {
  Serial.printf("🔵 Initializing Bluetooth as '%s'...\n", getDeviceName());
  
  NimBLEDevice::init(getDeviceName());
  
  // Create BLE Server
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  // Create BLE Service
  NimBLEService *pService = pServer->createService(SERVICE_UUID);
  
  // RX Characteristic - Device sends messages to phone
  pCharRxMessage = pService->createCharacteristic(
    CHAR_RX_MESSAGE_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  
  // TX Characteristic - Phone sends messages to device
  pCharTxMessage = pService->createCharacteristic(
    CHAR_TX_MESSAGE_UUID,
    NIMBLE_PROPERTY::WRITE
  );
  pCharTxMessage->setCallbacks(new MessageCallbacks());
  
  // Start the service
  pService->start();
  
  // Start advertising
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  NimBLEDevice::startAdvertising();
  
  Serial.printf("✅ BLE ready - Broadcasting as '%s'\n", getDeviceName());
}

void forwardToPhone(const String& message, uint8_t fromDevice, float rssi, float snr) {
  if (deviceConnected && pCharRxMessage) {
    // Create formatted message with device info
    String formattedMsg = "From Dev" + String(fromDevice) + ": " + message + 
                         " [RSSI:" + String(rssi, 1) + "dBm SNR:" + String(snr, 1) + "dB]";
    
    pCharRxMessage->setValue(formattedMsg.c_str());
    pCharRxMessage->notify();
    
    Serial.println("📱 Message forwarded to phone via BLE");
  }
}

void sendMessage(const String& text, uint8_t toDevice = 0) {  // 0 = broadcast
  if (!loraReady) {
    Serial.println("✗ LoRa not ready");
    return;
  }
  
  if (text.length() == 0 || text.length() > 180) {
    Serial.println("✗ Message empty or too long (max 180 chars)");
    return;
  }
  
  MessagePacket packet;
  packet.fromDevice = DEVICE_ID;
  packet.toDevice = toDevice;
  packet.messageId = millis();
  packet.messageLen = text.length();
  strncpy(packet.message, text.c_str(), 180);
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.printf("║  SENDING FROM DEVICE %d            ║\n", DEVICE_ID);
  Serial.println("╚═══════════════════════════════════╝");
  Serial.printf("📤 Message: \"%s\"\n", text.c_str());
  Serial.printf("📍 To: Device %d %s\n", toDevice, (toDevice == 0) ? "(Broadcast)" : "");
  Serial.printf("📏 Length: %d bytes\n", text.length());
  
  // Transmit
  int16_t state = lora.transmit((uint8_t*)&packet, sizeof(MessagePacket));
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✅ Message sent successfully!");
  } else {
    Serial.printf("❌ Send failed (error %d)\n", state);
  }
  
  Serial.println("═══════════════════════════════════\n");
  
  // Return to receive mode
  lora.startReceive();
}

void checkForMessages() {
  if (!loraReady) return;
  
  if (lora.getPacketLength() > 0) {
    MessagePacket packet;
    int state = lora.readData((uint8_t*)&packet, sizeof(MessagePacket));
    
    if (state == RADIOLIB_ERR_NONE) {
      // Check if message is for us or broadcast
      if (packet.toDevice == 0 || packet.toDevice == DEVICE_ID) {
        // Don't show our own messages
        if (packet.fromDevice != DEVICE_ID) {
          messageCount++;
          float rssi = lora.getRSSI();
          float snr = lora.getSNR();
          
          Serial.println("\n┌─────────────────────────────────────┐");
          Serial.printf("│   MESSAGE #%d RECEIVED FROM DEV %d   │\n", messageCount, packet.fromDevice);
          Serial.println("└─────────────────────────────────────┘");
          Serial.printf("📨 Message: \"%s\"\n", packet.message);
          Serial.printf("📊 RSSI: %.1f dBm\n", rssi);
          Serial.printf("📊 SNR: %.1f dB\n", snr);
          Serial.printf("🆔 Msg ID: %u\n", packet.messageId);
          
          // Forward to phone via BLE
          forwardToPhone(String(packet.message), packet.fromDevice, rssi, snr);
          
          Serial.println("═══════════════════════════════════════");
          Serial.print("> "); // Show prompt again
        }
      }
    } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
      Serial.printf("📡 Receive error (%d)\n", state);
    }
    
    // Restart receive
    lora.startReceive();
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔══════════════════════════════════════════╗");
  Serial.printf("║      BIDIRECTIONAL TRANSCEIVER + BLE    ║\n");
  Serial.printf("║           DEVICE: %s          ║\n", getDeviceName());
  Serial.println("╚══════════════════════════════════════════╝");
  
  pinMode(LED_BUILTIN, OUTPUT);
  
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
    
    // Start in receive mode
    Serial.print("📡 Starting receive mode... ");
    state = lora.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("OK!");
    } else {
      Serial.printf("Failed (%d)\n", state);
    }
    
  } else {
    Serial.printf("FAILED (error %d)\n", state);
    // Try simpler initialization
    Serial.print("📡 Trying alternative... ");
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
    Serial.println("\n🎯 Transceiver ready!");
    Serial.println("📡 Listening for LoRa messages...");
    Serial.printf("🔵 Broadcasting '%s' via Bluetooth\n", getDeviceName());
    Serial.println("📱 Connect with nRF Connect app to send/receive messages");
    Serial.println("⌨️  Type messages and press Enter to send");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.print("> ");
  } else {
    Serial.println("\n❌ LoRa failed - check wiring");
  }
}

void loop() {
  static uint32_t lastBlink = 0;
  static bool ledState = false;
  
  // LED heartbeat
  if (millis() - lastBlink >= 1000) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    lastBlink = millis();
  }
  
  // Check for incoming messages
  checkForMessages();
  
  // Handle serial input for sending messages
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        // Send the message as broadcast
        sendMessage(inputBuffer);
        inputBuffer = "";
        Serial.print("> "); // New prompt
      }
    } else if (c == 8 || c == 127) { // Backspace
      if (inputBuffer.length() > 0) {
        inputBuffer.remove(inputBuffer.length() - 1);
        Serial.print("\b \b"); // Erase character on screen
      }
    } else if (c >= 32 && c <= 126) { // Printable characters
      if (inputBuffer.length() < 180) {
        inputBuffer += c;
        Serial.print(c); // Echo character
      }
    }
  }
  
  delay(10);
}