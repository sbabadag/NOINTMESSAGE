#include <Arduino.h>
#include <NimBLEDevice.h>

// =================== DEVICE SETTINGS ===================
#define DEVICE_NAME     "LORA_TUNNEL"
#define MAX_MESSAGE_SIZE 200
#define HEARTBEAT_INTERVAL 5000  // 5 seconds

// =================== GLOBAL OBJECTS ===================
NimBLECharacteristic* pTxCharacteristic = nullptr;
NimBLECharacteristic* pRxCharacteristic = nullptr;

// =================== STATE VARIABLES ===================
bool deviceConnected = false;
String myDeviceId = "";
unsigned long lastHeartbeat = 0;

// =================== MESSAGE QUEUE ===================
struct Message {
    String content;
    unsigned long timestamp;
};

#define QUEUE_SIZE 5
Message messageQueue[QUEUE_SIZE];
int queueHead = 0, queueTail = 0;

// =================== BLE CALLBACKS ===================
class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Serial.println("📱 Phone connected!");
        
        if (pTxCharacteristic) {
            String welcome = "🎯 Connected to TUNNEL [" + myDeviceId + "]";
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
            
            // Add to message queue for processing
            addToQueue(message);
            
            // Send immediate confirmation back to phone
            if (pTxCharacteristic && deviceConnected) {
                String confirm = "✅ Sent[" + String(millis()) + "]: " + message;
                pTxCharacteristic->setValue(confirm.c_str());
                pTxCharacteristic->notify();
            }
        }
    }
};

// =================== QUEUE MANAGEMENT ===================
void addToQueue(String message) {
    int next = (queueHead + 1) % QUEUE_SIZE;
    if (next != queueTail) {
        messageQueue[queueHead].content = message;
        messageQueue[queueHead].timestamp = millis();
        queueHead = next;
        Serial.println("✅ Added to message queue");
    } else {
        Serial.println("❌ Message queue full!");
    }
}

bool getFromQueue(Message& msg) {
    if (queueTail != queueHead) {
        msg = messageQueue[queueTail];
        queueTail = (queueTail + 1) % QUEUE_SIZE;
        return true;
    }
    return false;
}

void generateDeviceId() {
    // Create unique ID from MAC address
    WiFi.mode(WIFI_STA); // Initialize WiFi to get MAC
    uint8_t mac[6];
    WiFi.macAddress(mac);
    myDeviceId = String(mac[4], HEX) + String(mac[5], HEX);
    myDeviceId.toUpperCase();
    Serial.println("🆔 Device ID: " + myDeviceId);
}

void processMessages() {
    Message msg;
    if (getFromQueue(msg) && deviceConnected && pTxCharacteristic) {
        // Simulate message from "partner device"
        delay(100);
        String relayMsg = "📡[PARTNER➡️YOU]: " + msg.content;
        pTxCharacteristic->setValue(relayMsg.c_str());
        pTxCharacteristic->notify();
        Serial.printf("📡⬅️ Relayed: %s\n", relayMsg.c_str());
    }
}

// =================== MAIN SETUP ===================
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("===============================");
    Serial.println("🚀 BLE Tunnel Demo v1.0");  
    Serial.println("===============================");
    Serial.println("📡 LoRa: DISABLED (Demo Mode)");
    Serial.println("🔵 BLE: Local Message Echo");
    
    // Generate unique device ID
    generateDeviceId();
    
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
    Serial.println("🧪 DEMO MODE: Messages will echo back as if from partner");
    Serial.println("🎯 Ready for testing!");
}

// =================== MAIN LOOP ===================
void loop() {
    // Process any queued messages
    processMessages();
    
    // Send heartbeat with system status
    if (deviceConnected && pTxCharacteristic && (millis() - lastHeartbeat > HEARTBEAT_INTERVAL)) {
        String status = "💓[" + myDeviceId + "] ";
        status += "BLE:✅ LoRa:🧪DEMO ";
        status += "Mode:LOCAL_ECHO";
        
        pTxCharacteristic->setValue(status.c_str());
        pTxCharacteristic->notify();
        lastHeartbeat = millis();
    }
    
    // Status logging
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 10000) {
        Serial.printf("📊 Demo Status: Phone:%s Messages:%d\n",
                     deviceConnected ? "✅" : "❌",
                     (queueHead - queueTail + QUEUE_SIZE) % QUEUE_SIZE);
        lastStatus = millis();
    }
    
    delay(100);
}