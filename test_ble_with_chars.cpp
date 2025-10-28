#include <Arduino.h>
#include <NimBLEDevice.h>

NimBLECharacteristic* pTxCharacteristic = nullptr;
NimBLECharacteristic* pRxCharacteristic = nullptr;
bool deviceConnected = false;

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Serial.println("📱 Device connected!");
    }

    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        Serial.println("📱 Device disconnected - restarting advertising");
        NimBLEDevice::startAdvertising();
    }
};

class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            Serial.printf("📥 Received: %s\n", value.c_str());
            
            // Echo back with timestamp
            if (pTxCharacteristic && deviceConnected) {
                String echo = "Echo[" + String(millis()) + "]: " + String(value.c_str());
                pTxCharacteristic->setValue(echo.c_str());
                pTxCharacteristic->notify();
                Serial.printf("📤 Sent: %s\n", echo.c_str());
            }
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("===========================");
    Serial.println("🚀 BLE Tunnel with Characteristics");
    Serial.println("===========================");
    
    // Initialize BLE
    Serial.println("🔧 Initializing BLE...");
    NimBLEDevice::init("LORA_TUNNEL");
    Serial.println("✅ BLE initialized");
    
    // Create server
    Serial.println("🔧 Creating server...");
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    Serial.println("✅ Server created");
    
    // Create service
    Serial.println("🔧 Creating service FFE0...");
    NimBLEService* pService = pServer->createService("FFE0");
    Serial.println("✅ Service created");
    
    // Create TX characteristic (for sending data to phone)
    Serial.println("🔧 Creating TX characteristic FFE1...");
    pTxCharacteristic = pService->createCharacteristic(
        "FFE1", 
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    Serial.println("✅ TX characteristic created");
    
    // Create RX characteristic (for receiving data from phone)
    Serial.println("🔧 Creating RX characteristic FFE2...");
    pRxCharacteristic = pService->createCharacteristic(
        "FFE2",
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new CharacteristicCallbacks());
    Serial.println("✅ RX characteristic created");
    
    // Start service
    Serial.println("🔧 Starting service...");
    pService->start();
    Serial.println("✅ Service started");
    
    // Start advertising
    Serial.println("🔧 Starting advertising...");
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("FFE0");
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    Serial.println("✅ Advertising started");
    
    Serial.println("");
    Serial.println("🎯 BLE Tunnel Ready!");
    Serial.println("📱 Device: LORA_TUNNEL");
    Serial.println("📋 Service: FFE0");
    Serial.println("📋 TX (notify): FFE1");
    Serial.println("📋 RX (write): FFE2");
    Serial.println("");
    Serial.println("Test: Write to FFE2, receive notifications on FFE1");
}

void loop() {
    static unsigned long lastHeartbeat = 0;
    static int counter = 0;
    
    // Send periodic heartbeat if connected
    if (deviceConnected && pTxCharacteristic && millis() - lastHeartbeat > 10000) {
        String heartbeat = "Heartbeat #" + String(++counter) + " [" + String(millis()) + "]";
        pTxCharacteristic->setValue(heartbeat.c_str());
        pTxCharacteristic->notify();
        Serial.println("💓 Heartbeat sent: " + heartbeat);
        lastHeartbeat = millis();
    }
    
    // Status update
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 5000) {
        Serial.printf("📡 Status: %s\n", deviceConnected ? "CONNECTED" : "Advertising...");
        lastStatus = millis();
    }
    
    delay(100);
}