/*
 * ESP32 LoRa BLE Bridge - Enhanced Version with Diagnostics
 * This version includes additional debugging and monitoring features
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <RadioLib.h>
#include <WiFi.h>  // For MAC address

// ==================== ENHANCED CONFIGURATION ====================

// Station Configuration - CHANGE THIS FOR EACH DEVICE
#define STATION_TYPE "M1"  // "M1" or "M2"
#define DEVICE_NAME STATION_TYPE
#define FIRMWARE_VERSION "1.1.0"

// Enhanced debugging
#define DEBUG_LEVEL 2  // 0=None, 1=Basic, 2=Verbose
#define ENABLE_DIAGNOSTICS true
#define ENABLE_HEARTBEAT true
#define HEARTBEAT_INTERVAL 30000  // 30 seconds

// BLE Configuration (Nordic UART Service)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// LoRa Configuration with better defaults
#define LORA_SCK     5
#define LORA_MISO    19
#define LORA_MOSI    27
#define LORA_CS      18
#define LORA_RST     14
#define LORA_DIO1    26
#define LORA_DIO2    35
#define LORA_DIO3    34

// Adaptive LoRa Settings
#define LORA_FREQ    868.0  // MHz
#define LORA_SF      7      // Spreading Factor (7-12)
#define LORA_BW      125.0  // Bandwidth in kHz
#define LORA_CR      5      // Coding Rate 4/5
#define LORA_POWER   14     // TX Power in dBm (2-20)
#define LORA_PREAMBLE 8     // Preamble length
#define LORA_SYNC_WORD 0x12 // Sync word

// Enhanced buffer management
#define MAX_MESSAGE_SIZE 200
#define JSON_BUFFER_SIZE 1024
#define LORA_BUFFER_SIZE 256

// Performance monitoring
struct SystemStats {
  unsigned long messagesReceived = 0;
  unsigned long messagesSent = 0;
  unsigned long loraPacketsSent = 0;
  unsigned long loraPacketsReceived = 0;
  unsigned long bleConnections = 0;
  unsigned long lastResetTime = 0;
  float avgRSSI = 0;
  float avgSNR = 0;
};

SystemStats stats;

// ==================== ENHANCED GLOBAL VARIABLES ====================

// BLE
BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic = NULL;
BLECharacteristic* pRxCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long connectionStartTime = 0;

// LoRa
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_DIO2);
volatile bool receivedFlag = false;
unsigned long lastLoRaActivity = 0;

// Enhanced Message Structure
struct EnhancedMessage {
  String messageId;
  String text;
  String timestamp;
  String sender;
  String stationType;
  String targetStation;
  unsigned long createdAt;
  int retryCount;
  bool acknowledged;
};

// Message queue for reliability
std::vector<EnhancedMessage> messageQueue;

// Station Info
String myStation = STATION_TYPE;
String remoteStation = (String(STATION_TYPE) == "M1") ? "M2" : "M1";
String deviceMAC;

// ==================== ENHANCED BLE CALLBACKS ====================

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      connectionStartTime = millis();
      stats.bleConnections++;
      
      debugPrint("📱 Phone connected via BLE", 1);
      if (DEBUG_LEVEL >= 2) {
        Serial.printf("   Connection #%lu at %lu ms\n", stats.bleConnections, connectionStartTime);
      }
      
      // Send connection confirmation
      sendSystemMessage("Connected to " + myStation + " station");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      unsigned long connectionDuration = millis() - connectionStartTime;
      
      debugPrint("📱 Phone disconnected from BLE", 1);
      if (DEBUG_LEVEL >= 2) {
        Serial.printf("   Connection lasted %lu ms\n", connectionDuration);
      }
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String receivedData = pCharacteristic->getValue().c_str();
      
      if (receivedData.length() > 0) {
        stats.messagesReceived++;
        debugPrint("📨 Received from phone (" + String(receivedData.length()) + " bytes): " + receivedData, 1);
        processIncomingBLEMessage(receivedData);
      }
    }
};

// ==================== ENHANCED SETUP FUNCTIONS ====================

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // Get MAC address for unique identification
  WiFi.mode(WIFI_MODE_STA);
  deviceMAC = WiFi.macAddress();
  
  Serial.println("\n🚀 ESP32 LoRa BLE Bridge Starting...");
  Serial.println("📡 Station: " + myStation);
  Serial.println("🎯 Target: " + remoteStation);
  Serial.println("🔧 Firmware: v" + String(FIRMWARE_VERSION));
  Serial.println("🆔 MAC: " + deviceMAC);
  Serial.printf("💾 Free Heap: %d bytes\n", ESP.getFreeHeap());
  
  stats.lastResetTime = millis();
  
  // Initialize LoRa with enhanced error handling
  if (!setupLoRa()) {
    Serial.println("❌ LoRa initialization failed - entering safe mode");
    while(true) {
      Serial.println("💔 System halted - check LoRa wiring");
      delay(5000);
    }
  }
  
  // Initialize BLE
  setupBLE();
  
  Serial.println("✅ System ready for bidirectional messaging!");
  Serial.println("🔍 Debug level: " + String(DEBUG_LEVEL));
  printSystemInfo();
}

bool setupLoRa() {
  debugPrint("📡 Initializing LoRa module...", 1);
  
  // Initialize SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  
  // Initialize LoRa module with enhanced settings
  int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, LORA_SYNC_WORD, LORA_POWER, LORA_PREAMBLE, 1.6);
  
  if (state == RADIOLIB_ERR_NONE) {
    debugPrint("✅ LoRa module initialized successfully", 1);
    if (DEBUG_LEVEL >= 2) {
      Serial.printf("📊 Config: %.1fMHz, SF%d, BW%.1fkHz, CR4/%d, Power:%ddBm\n", 
                    LORA_FREQ, LORA_SF, LORA_BW, LORA_CR, LORA_POWER);
    }
    
    // Set interrupt for incoming LoRa messages
    radio.setDio1Action(onLoRaReceive);
    
    // Start listening
    radio.startReceive();
    debugPrint("👂 LoRa listening mode activated", 1);
    return true;
  } else {
    Serial.printf("❌ LoRa initialization failed, code: %d\n", state);
    return false;
  }
}

void setupBLE() {
  debugPrint("📱 Initializing BLE server...", 1);
  
  // Create BLE Device with enhanced name
  String deviceNameWithMAC = String(DEVICE_NAME) + "_" + deviceMAC.substring(9);
  BLEDevice::init(deviceNameWithMAC.c_str());
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create characteristics
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());
  
  pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE
                      );
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  
  // Start service
  pService->start();
  
  // Configure advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();
  
  debugPrint("✅ BLE server ready - advertising as: " + deviceNameWithMAC, 1);
}

// ==================== ENHANCED MESSAGE PROCESSING ====================

void processIncomingBLEMessage(String jsonMessage) {
  debugPrint("🔄 Processing BLE message for LoRa relay...", 2);
  
  // Enhanced JSON parsing with error recovery
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  DeserializationError error = deserializeJson(doc, jsonMessage);
  
  if (error) {
    debugPrint("❌ JSON parsing failed: " + String(error.c_str()), 1);
    sendSystemMessage("Error: Invalid message format");
    return;
  }
  
  // Validate required fields
  if (!doc.containsKey("text") || !doc.containsKey("messageId")) {
    debugPrint("❌ Missing required fields in message", 1);
    sendSystemMessage("Error: Missing message fields");
    return;
  }
  
  // Create enhanced message
  EnhancedMessage msg;
  msg.messageId = doc["messageId"].as<String>();
  msg.text = doc["text"].as<String>();
  msg.timestamp = doc["timestamp"].as<String>();
  msg.sender = doc["sender"].as<String>();
  msg.stationType = myStation;
  msg.targetStation = remoteStation;
  msg.createdAt = millis();
  msg.retryCount = 0;
  msg.acknowledged = false;
  
  // Validate message content
  if (msg.text.length() == 0 || msg.text.length() > MAX_MESSAGE_SIZE) {
    debugPrint("❌ Invalid message length: " + String(msg.text.length()), 1);
    sendSystemMessage("Error: Message too long or empty");
    return;
  }
  
  debugPrint("📤 Relaying to " + remoteStation + ": " + msg.text, 1);
  
  // Send via LoRa with enhanced error handling
  if (sendLoRaMessage(msg)) {
    stats.messagesSent++;
    sendSystemMessage("Message sent via LoRa");
  } else {
    sendSystemMessage("Error: LoRa transmission failed");
  }
}

bool sendLoRaMessage(EnhancedMessage msg) {
  // Create comprehensive LoRa packet
  DynamicJsonDocument loraDoc(JSON_BUFFER_SIZE);
  loraDoc["messageId"] = msg.messageId;
  loraDoc["text"] = msg.text;
  loraDoc["timestamp"] = msg.timestamp;
  loraDoc["sender"] = msg.sender;
  loraDoc["fromStation"] = msg.stationType;
  loraDoc["toStation"] = msg.targetStation;
  loraDoc["route"] = msg.stationType + "→" + msg.targetStation;
  loraDoc["createdAt"] = msg.createdAt;
  loraDoc["deviceMAC"] = deviceMAC.substring(9);  // Last 6 chars
  
  String loraPacket;
  serializeJson(loraDoc, loraPacket);
  
  if (loraPacket.length() > LORA_BUFFER_SIZE) {
    debugPrint("❌ LoRa packet too large: " + String(loraPacket.length()) + " bytes", 1);
    return false;
  }
  
  debugPrint("📡 Sending LoRa packet (" + String(loraPacket.length()) + " bytes)", 1);
  if (DEBUG_LEVEL >= 2) {
    Serial.println("   Packet: " + loraPacket);
  }
  
  // Send LoRa packet
  int state = radio.transmit(loraPacket);
  
  if (state == RADIOLIB_ERR_NONE) {
    stats.loraPacketsSent++;
    lastLoRaActivity = millis();
    debugPrint("✅ LoRa message sent successfully", 1);
    
    if (DEBUG_LEVEL >= 2) {
      Serial.printf("   TX Stats: Packets sent: %lu\n", stats.loraPacketsSent);
    }
    
    // Return to receive mode
    radio.startReceive();
    return true;
  } else {
    debugPrint("❌ LoRa transmission failed, code: " + String(state), 1);
    radio.startReceive();  // Ensure we return to receive mode
    return false;
  }
}

// ==================== ENHANCED LORA RECEIVE HANDLER ====================

void onLoRaReceive(void) {
  receivedFlag = true;
}

void handleLoRaMessage() {
  if (!receivedFlag) return;
  receivedFlag = false;
  
  String receivedPacket;
  int state = radio.readData(receivedPacket);
  
  if (state != RADIOLIB_ERR_NONE) {
    debugPrint("❌ LoRa receive failed, code: " + String(state), 1);
    radio.startReceive();
    return;
  }
  
  lastLoRaActivity = millis();
  stats.loraPacketsReceived++;
  
  // Get signal quality
  float rssi = radio.getRSSI();
  float snr = radio.getSNR();
  
  // Update running averages
  stats.avgRSSI = (stats.avgRSSI * 0.9) + (rssi * 0.1);
  stats.avgSNR = (stats.avgSNR * 0.9) + (snr * 0.1);
  
  debugPrint("📡 Received LoRa packet (" + String(receivedPacket.length()) + " bytes)", 1);
  if (DEBUG_LEVEL >= 2) {
    Serial.printf("📊 RSSI: %.1fdBm, SNR: %.1fdB, Avg RSSI: %.1fdBm\n", rssi, snr, stats.avgRSSI);
    Serial.println("   Packet: " + receivedPacket);
  }
  
  // Parse LoRa packet with enhanced validation
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  DeserializationError error = deserializeJson(doc, receivedPacket);
  
  if (error) {
    debugPrint("❌ LoRa packet JSON parsing failed: " + String(error.c_str()), 1);
    radio.startReceive();
    return;
  }
  
  // Validate packet structure
  if (!doc.containsKey("toStation") || !doc.containsKey("text") || !doc.containsKey("messageId")) {
    debugPrint("❌ Invalid LoRa packet structure", 1);
    radio.startReceive();
    return;
  }
  
  // Check if message is for this station
  String toStation = doc["toStation"].as<String>();
  if (toStation != myStation) {
    debugPrint("📭 Message not for this station (" + toStation + " != " + myStation + ")", 2);
    radio.startReceive();
    return;
  }
  
  debugPrint("📬 Message is for this station - relaying to phone", 1);
  
  // Relay to connected phone via BLE
  relayLoRaMessageToBLE(doc, rssi, snr);
  
  // Return to receive mode
  radio.startReceive();
}

void relayLoRaMessageToBLE(DynamicJsonDocument& loraMsg, float rssi, float snr) {
  if (!deviceConnected) {
    debugPrint("📱 No phone connected - message lost", 1);
    return;
  }
  
  // Create enhanced message for phone
  DynamicJsonDocument phoneMsg(JSON_BUFFER_SIZE);
  phoneMsg["text"] = loraMsg["text"];
  phoneMsg["sender"] = "remote";
  phoneMsg["timestamp"] = loraMsg["timestamp"];
  phoneMsg["deviceId"] = (loraMsg["fromStation"] == "M1") ? 1 : 2;
  phoneMsg["stationType"] = loraMsg["fromStation"];
  phoneMsg["route"] = loraMsg["route"];
  phoneMsg["rssi"] = rssi;
  phoneMsg["snr"] = snr;
  phoneMsg["messageId"] = loraMsg["messageId"];
  
  // Add system info
  phoneMsg["receivedAt"] = millis();
  phoneMsg["stationMAC"] = deviceMAC.substring(9);
  
  String phoneMessage;
  serializeJson(phoneMsg, phoneMessage);
  
  debugPrint("📱 Sending to phone (" + String(phoneMessage.length()) + " bytes)", 1);
  if (DEBUG_LEVEL >= 2) {
    Serial.println("   Message: " + phoneMessage);
  }
  
  // Send via BLE notification
  pTxCharacteristic->setValue(phoneMessage.c_str());
  pTxCharacteristic->notify();
  
  debugPrint("✅ Message relayed to phone successfully", 1);
}

// ==================== ENHANCED DIAGNOSTIC FUNCTIONS ====================

void sendSystemMessage(String message) {
  if (!deviceConnected) return;
  
  DynamicJsonDocument sysMsg(512);
  sysMsg["text"] = message;
  sysMsg["sender"] = "system";
  sysMsg["timestamp"] = millis();
  sysMsg["type"] = "system";
  sysMsg["station"] = myStation;
  
  String systemMessage;
  serializeJson(sysMsg, systemMessage);
  
  pTxCharacteristic->setValue(systemMessage.c_str());
  pTxCharacteristic->notify();
}

void sendHeartbeat() {
  if (!deviceConnected || !ENABLE_HEARTBEAT) return;
  
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat < HEARTBEAT_INTERVAL) return;
  
  DynamicJsonDocument heartbeat(512);
  heartbeat["type"] = "heartbeat";
  heartbeat["station"] = myStation;
  heartbeat["timestamp"] = millis();
  heartbeat["uptime"] = millis() - stats.lastResetTime;
  heartbeat["freeHeap"] = ESP.getFreeHeap();
  heartbeat["avgRSSI"] = stats.avgRSSI;
  heartbeat["avgSNR"] = stats.avgSNR;
  heartbeat["messagesSent"] = stats.messagesSent;
  heartbeat["messagesReceived"] = stats.messagesReceived;
  heartbeat["loraPacketsSent"] = stats.loraPacketsSent;
  heartbeat["loraPacketsReceived"] = stats.loraPacketsReceived;
  
  String msg;
  serializeJson(heartbeat, msg);
  
  pTxCharacteristic->setValue(msg.c_str());
  pTxCharacteristic->notify();
  
  lastHeartbeat = millis();
  debugPrint("💓 Heartbeat sent", 2);
}

void printSystemInfo() {
  if (!ENABLE_DIAGNOSTICS) return;
  
  Serial.println("\n=== SYSTEM INFORMATION ===");
  Serial.println("Station: " + myStation + " → " + remoteStation);
  Serial.println("MAC Address: " + deviceMAC);
  Serial.println("Firmware: v" + String(FIRMWARE_VERSION));
  Serial.printf("Uptime: %lu ms\n", millis() - stats.lastResetTime);
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("BLE Connected: %s\n", deviceConnected ? "Yes" : "No");
  Serial.printf("BLE Connections: %lu\n", stats.bleConnections);
  Serial.printf("Messages Sent: %lu\n", stats.messagesSent);
  Serial.printf("Messages Received: %lu\n", stats.messagesReceived);
  Serial.printf("LoRa Packets Sent: %lu\n", stats.loraPacketsSent);
  Serial.printf("LoRa Packets Received: %lu\n", stats.loraPacketsReceived);
  Serial.printf("Average RSSI: %.1f dBm\n", stats.avgRSSI);
  Serial.printf("Average SNR: %.1f dB\n", stats.avgSNR);
  Serial.printf("Last LoRa Activity: %lu ms ago\n", millis() - lastLoRaActivity);
  Serial.println("=========================\n");
}

void debugPrint(String message, int level) {
  if (DEBUG_LEVEL >= level) {
    Serial.println(message);
  }
}

// ==================== ENHANCED MAIN LOOP ====================

void loop() {
  // Handle LoRa messages (highest priority)
  handleLoRaMessage();
  
  // Send periodic heartbeat
  sendHeartbeat();
  
  // Handle BLE connection changes
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    debugPrint("📱 Started advertising for new connection", 1);
    oldDeviceConnected = deviceConnected;
  }
  
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  // Periodic diagnostics
  static unsigned long lastDiagnostics = 0;
  if (ENABLE_DIAGNOSTICS && millis() - lastDiagnostics > 60000) {  // Every minute
    printSystemInfo();
    lastDiagnostics = millis();
  }
  
  // Watchdog prevention
  delay(10);
}