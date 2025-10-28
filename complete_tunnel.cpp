#include <Arduino.h>
#include <NimBLEDevice.h>
#include <RadioLib.h>
#include <ArduinoJson.h>

// =================== HARDWARE CONFIGURATION ===================
#define NSS_PIN     D7    // SPI NSS
#define DIO1_PIN    D1    // DIO1  
#define NRST_PIN    D0    // RESET
#define BUSY_PIN    D2    // BUSY

// =================== COMMUNICATION SETTINGS ===================
#define LORA_FREQUENCY  915.0    // MHz (US band)
#define LORA_BANDWIDTH  125.0    // kHz
#define LORA_SF         7        // Spreading Factor (7-12, lower = faster)
#define LORA_CR         5        // Coding Rate (5-8, lower = faster)
#define LORA_POWER      14       // dBm (max 22)
#define SYNC_WORD       0x34     // Private network sync word

// =================== DEVICE SETTINGS ===================
#define DEVICE_NAME     "LORA_TUNNEL"
#define MAX_MESSAGE_SIZE 200
#define PAIRING_TIMEOUT  30000   // 30 seconds to find partner
#define HEARTBEAT_INTERVAL 5000  // 5 seconds

// =================== GLOBAL OBJECTS ===================
SX1262 radio = new Module(NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN);
NimBLECharacteristic* pTxCharacteristic = nullptr;
NimBLECharacteristic* pRxCharacteristic = nullptr;

// =================== STATE VARIABLES ===================
bool deviceConnected = false;
bool loraReady = false;
bool devicePaired = false;
String partnerDeviceId = "";
String myDeviceId = "";
unsigned long lastHeartbeat = 0;
unsigned long pairingStartTime = 0;

// =================== MESSAGE QUEUES ===================
struct Message {
    String content;
    String fromDevice;
    String toDevice;
    unsigned long timestamp;
};

#define QUEUE_SIZE 10
Message bleToLoraQueue[QUEUE_SIZE];
Message loraToBleQueue[QUEUE_SIZE];
int bleToLoraHead = 0, bleToLoraTail = 0;
int loraToBleHead = 0, loraToBLETail = 0;

// =================== BLE CALLBACKS ===================
class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Serial.println("📱 Phone connected!");
        
        if (pTxCharacteristic) {
            String welcome = "Connected to " + String(DEVICE_NAME) + " [" + myDeviceId + "]";
            if (devicePaired) {
                welcome += " ↔ Paired with [" + partnerDeviceId + "]";
            } else {
                welcome += " - Searching for partner...";
            }
            pTxCharacteristic->setValue(welcome.c_str());
            pTxCharacteristic->notify();
        }
    }

    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        Serial.println("📱 Phone disconnected - restarting advertising");
        delay(100);
        NimBLEDevice::startAdvertising();
    }
};

class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0 && value.length() <= MAX_MESSAGE_SIZE) {
            String message = String(value.c_str());
            Serial.printf("📱➡️ From phone: %s\n", message.c_str());
            
            // Add to BLE->LoRa queue
            addToBleLoraQueue(message);
            
            // Send confirmation back to phone
            if (pTxCharacteristic && deviceConnected) {
                String confirm = "Sent[" + String(millis()) + "]: " + message;
                pTxCharacteristic->setValue(confirm.c_str());
                pTxCharacteristic->notify();
            }
        }
    }
};

// =================== QUEUE MANAGEMENT ===================
void addToBleLoraQueue(String message) {
    int next = (bleToLoraHead + 1) % QUEUE_SIZE;
    if (next != bleToLoraTail) {
        bleToLoraQueue[bleToLoraHead].content = message;
        bleToLoraQueue[bleToLoraHead].fromDevice = myDeviceId;
        bleToLoraQueue[bleToLoraHead].toDevice = partnerDeviceId;
        bleToLoraQueue[bleToLoraHead].timestamp = millis();
        bleToLoraHead = next;
        Serial.println("✅ Added to BLE->LoRa queue");
    } else {
        Serial.println("❌ BLE->LoRa queue full!");
    }
}

void addToLoraBleQueue(String message, String fromDevice) {
    int next = (loraToBleHead + 1) % QUEUE_SIZE;
    if (next != loraToBLETail) {
        loraToBleQueue[loraToBleHead].content = message;
        loraToBleQueue[loraToBleHead].fromDevice = fromDevice;
        loraToBleQueue[loraToBleHead].toDevice = myDeviceId;
        loraToBleQueue[loraToBleHead].timestamp = millis();
        loraToBleHead = next;
        Serial.println("✅ Added to LoRa->BLE queue");
    } else {
        Serial.println("❌ LoRa->BLE queue full!");
    }
}

bool getBleLoraMessage(Message& msg) {
    if (bleToLoraTail != bleToLoraHead) {
        msg = bleToLoraQueue[bleToLoraTail];
        bleToLoraTail = (bleToLoraTail + 1) % QUEUE_SIZE;
        return true;
    }
    return false;
}

bool getLoraBleMessage(Message& msg) {
    if (loraToBLETail != loraToBleHead) {
        msg = loraToBleQueue[loraToBLETail];
        loraToBLETail = (loraToBLETail + 1) % QUEUE_SIZE;
        return true;
    }
    return false;
}

// =================== LORA FUNCTIONS ===================
bool initLoRa() {
    Serial.println("🔧 Initializing LoRa SX1262...");
    
    // Initialize with step-by-step configuration
    int state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("❌ LoRa init failed: %d\n", state);
        return false;
    }
    
    // Configure frequency
    state = radio.setFrequency(LORA_FREQUENCY);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("❌ LoRa frequency failed: %d\n", state);
        return false;
    }
    
    // Configure bandwidth
    state = radio.setBandwidth(LORA_BANDWIDTH);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("❌ LoRa bandwidth failed: %d\n", state);
        return false;
    }
    
    // Configure spreading factor
    state = radio.setSpreadingFactor(LORA_SF);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("❌ LoRa SF failed: %d\n", state);
        return false;
    }
    
    // Configure coding rate  
    state = radio.setCodingRate(LORA_CR);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("❌ LoRa CR failed: %d\n", state);
        return false;
    }
    
    // Configure output power
    state = radio.setOutputPower(LORA_POWER);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("❌ LoRa power failed: %d\n", state);
        return false;
    }
    
    // Set sync word
    state = radio.setSyncWord(SYNC_WORD);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("❌ LoRa sync word failed: %d\n", state);
        return false;
    }
    
    Serial.println("✅ LoRa initialized successfully");
    Serial.printf("📡 Freq: %.1f MHz, BW: %.1f kHz, SF: %d, CR: 4/%d, Power: %d dBm\n", 
                  LORA_FREQUENCY, LORA_BANDWIDTH, LORA_SF, LORA_CR, LORA_POWER);
    
    return true;
}

void sendLoRaMessage(String message, String messageType = "DATA") {
    if (!loraReady) return;
    
    // Create JSON message
    JsonDocument doc;
    doc["type"] = messageType;
    doc["from"] = myDeviceId;
    doc["to"] = (messageType == "PAIR") ? "BROADCAST" : partnerDeviceId;
    doc["data"] = message;
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    if (jsonString.length() > MAX_MESSAGE_SIZE) {
        Serial.println("❌ Message too long for LoRa");
        return;
    }
    
    Serial.printf("📡➡️ Sending LoRa: %s\n", jsonString.c_str());
    int state = radio.transmit(jsonString);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("✅ LoRa message sent");
    } else {
        Serial.printf("❌ LoRa send failed: %d\n", state);
    }
    
    // Return to receive mode
    radio.startReceive();
}

void handleLoRaReceive() {
    if (!loraReady) return;
    
    String received;
    int state = radio.readData(received);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("📡⬅️ Received LoRa: %s\n", received.c_str());
        
        // Parse JSON message
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, received);
        
        if (error) {
            Serial.println("❌ Invalid JSON received");
            return;
        }
        
        String messageType = doc["type"];
        String fromDevice = doc["from"];
        String toDevice = doc["to"];
        String data = doc["data"];
        
        if (messageType == "PAIR") {
            handlePairingMessage(fromDevice, data);
        } else if (messageType == "DATA" && fromDevice == partnerDeviceId) {
            // Route to phone via BLE
            addToLoraBleQueue(data, fromDevice);
        }
    }
}

// =================== PAIRING SYSTEM ===================
void generateDeviceId() {
    // Create unique ID from MAC address
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    myDeviceId = String(mac[4], HEX) + String(mac[5], HEX);
    myDeviceId.toUpperCase();
    Serial.println("🆔 Device ID: " + myDeviceId);
}

void startPairing() {
    Serial.println("🤝 Starting device pairing...");
    devicePaired = false;
    partnerDeviceId = "";
    pairingStartTime = millis();
    
    // Send pairing broadcast
    sendLoRaMessage("HELLO_" + myDeviceId, "PAIR");
}

void handlePairingMessage(String fromDevice, String data) {
    if (devicePaired && partnerDeviceId == fromDevice) return;
    
    if (data.startsWith("HELLO_") && fromDevice != myDeviceId) {
        Serial.println("🤝 Received pairing request from: " + fromDevice);
        
        if (!devicePaired) {
            partnerDeviceId = fromDevice;
            devicePaired = true;
            Serial.println("✅ Paired with device: " + partnerDeviceId);
            
            // Send pairing confirmation
            sendLoRaMessage("PAIRED_" + myDeviceId, "PAIR");
            
            // Notify phone
            if (pTxCharacteristic && deviceConnected) {
                String pairMsg = "🤝 Paired with device [" + partnerDeviceId + "] - Ready for messaging!";
                pTxCharacteristic->setValue(pairMsg.c_str());
                pTxCharacteristic->notify();
            }
        }
    } else if (data.startsWith("PAIRED_") && fromDevice == partnerDeviceId) {
        Serial.println("✅ Pairing confirmed by: " + partnerDeviceId);
    }
}

// =================== MAIN SETUP ===================
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("===============================");
    Serial.println("🚀 LoRa-Bluetooth Tunnel v1.0");
    Serial.println("===============================");
    
    // Generate unique device ID
    generateDeviceId();
    
    // Initialize LoRa
    loraReady = initLoRa();
    if (loraReady) {
        radio.startReceive(); // Start listening
    }
    
    // Initialize BLE
    Serial.println("🔧 Initializing BLE...");
    NimBLEDevice::init(DEVICE_NAME);
    
    // Create BLE server
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    // Create BLE service
    NimBLEService* pService = pServer->createService("FFE0");
    
    // Create characteristics
    pTxCharacteristic = pService->createCharacteristic("FFE1", 
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    
    pRxCharacteristic = pService->createCharacteristic("FFE2",
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    pRxCharacteristic->setCallbacks(new CharacteristicCallbacks());
    
    // Start BLE service
    pService->start();
    
    // Start advertising
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("FFE0");
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    
    Serial.println("✅ BLE tunnel ready!");
    Serial.println("");
    Serial.println("📱 Connect phone to: " + String(DEVICE_NAME));
    Serial.println("🆔 Device ID: " + myDeviceId);
    Serial.println("📋 Write to FFE2, receive on FFE1");
    Serial.println("");
    
    // Start pairing process
    if (loraReady) {
        startPairing();
    }
    
    Serial.println("🎯 System ready - waiting for partner device and phone connection");
}

// =================== MAIN LOOP ===================
void loop() {
    // Handle LoRa communication
    if (loraReady) {
        handleLoRaReceive();
        
        // Process BLE->LoRa queue
        Message msg;
        if (getBleLoraMessage(msg) && devicePaired) {
            sendLoRaMessage(msg.content, "DATA");
        }
        
        // Handle pairing timeout
        if (!devicePaired && (millis() - pairingStartTime > PAIRING_TIMEOUT)) {
            Serial.println("⏰ Pairing timeout - retrying...");
            startPairing();
        }
    }
    
    // Process LoRa->BLE queue
    Message msg;
    if (getLoraBleMessage(msg) && deviceConnected && pTxCharacteristic) {
        String notification = "📨[" + msg.fromDevice + "]: " + msg.content;
        pTxCharacteristic->setValue(notification.c_str());
        pTxCharacteristic->notify();
        Serial.printf("📱⬅️ To phone: %s\n", notification.c_str());
    }
    
    // Send heartbeat
    if (deviceConnected && pTxCharacteristic && (millis() - lastHeartbeat > HEARTBEAT_INTERVAL)) {
        String status = "💓[" + myDeviceId + "] ";
        status += loraReady ? "LoRa✅ " : "LoRa❌ ";
        status += devicePaired ? ("Paired:" + partnerDeviceId) : "Searching...";
        
        pTxCharacteristic->setValue(status.c_str());
        pTxCharacteristic->notify();
        lastHeartbeat = millis();
    }
    
    // Status logging
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 10000) {
        Serial.printf("📊 Status: Phone:%s LoRa:%s Paired:%s Partner:%s\n",
                     deviceConnected ? "✅" : "❌",
                     loraReady ? "✅" : "❌", 
                     devicePaired ? "✅" : "❌",
                     partnerDeviceId.c_str());
        lastStatus = millis();
    }
    
    delay(100);
}