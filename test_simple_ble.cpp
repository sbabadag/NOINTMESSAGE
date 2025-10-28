#include <Arduino.h>
#include <NimBLEDevice.h>

// Simple BLE test - no LoRa, no complex features
#define BLE_SERVER_NAME "LoRa_Test"

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharacteristicTX = nullptr;
NimBLECharacteristic* pCharacteristicRX = nullptr;
bool deviceConnected = false;

class MyServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Serial.println("📱 Device connected!");
        digitalWrite(LED_BUILTIN, HIGH);
    };

    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        Serial.println("📱 Device disconnected!");
        digitalWrite(LED_BUILTIN, LOW);
        
        delay(500);
        NimBLEDevice::startAdvertising();
        Serial.println("📡 Restarted advertising");
    }
};

class MyCharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        
        if (value.length() > 0) {
            Serial.printf("📱 Received: %s\n", value.c_str());
            
            // Echo back the message
            if (deviceConnected && pCharacteristicTX) {
                String echo = "Echo: " + String(value.c_str());
                pCharacteristicTX->setValue(echo.c_str());
                pCharacteristicTX->notify();
                Serial.printf("📱 Sent back: %s\n", echo.c_str());
            }
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    
    Serial.println("🚀 Starting BLE Test");
    Serial.println("====================");
    
    // Initialize BLE
    Serial.println("🔧 Initializing BLE...");
    NimBLEDevice::init(BLE_SERVER_NAME);
    
    // Create server
    Serial.println("🔧 Creating server...");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    // Create service with simple UUID
    Serial.println("🔧 Creating service FFE0...");
    NimBLEService *pService = pServer->createService("FFE0");
    
    if (!pService) {
        Serial.println("❌ FAILED to create service!");
        while(1) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(200);
            digitalWrite(LED_BUILTIN, LOW);
            delay(200);
        }
    }
    Serial.println("✅ Service created successfully!");
    
    // Create TX characteristic (notify)
    Serial.println("🔧 Creating TX characteristic FFE1...");
    pCharacteristicTX = pService->createCharacteristic(
                          "FFE1",
                          NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
                        );
    
    if (!pCharacteristicTX) {
        Serial.println("❌ FAILED to create TX characteristic!");
        while(1) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(500);
            digitalWrite(LED_BUILTIN, LOW);
            delay(500);
        }
    }
    Serial.println("✅ TX characteristic created!");
    
    // Create RX characteristic (write)
    Serial.println("🔧 Creating RX characteristic FFE2...");
    pCharacteristicRX = pService->createCharacteristic(
                          "FFE2",
                          NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
                        );
    
    if (!pCharacteristicRX) {
        Serial.println("❌ FAILED to create RX characteristic!");
        while(1) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
        }
    }
    Serial.println("✅ RX characteristic created!");
    
    pCharacteristicRX->setCallbacks(new MyCharacteristicCallbacks());
    
    // Start service
    Serial.println("🔧 Starting service...");
    pService->start();
    Serial.println("✅ Service started!");
    
    // Start server
    Serial.println("🔧 Starting server...");
    pServer->start();
    Serial.println("✅ Server started!");
    
    // Start advertising
    Serial.println("🔧 Starting advertising...");
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("FFE0");
    pAdvertising->start();
    Serial.println("✅ Advertising started!");
    
    Serial.println("");
    Serial.println("🎯 BLE Test Ready!");
    Serial.println("📱 Connect with nRF Connect and look for service FFE0");
    Serial.println("📋 TX: FFE1 (enable notifications to receive)");
    Serial.println("📋 RX: FFE2 (write to this to send messages)");
    Serial.println("");
    
    // Success blink pattern
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
    }
}

void loop() {
    // Handle connection status
    static unsigned long lastHeartbeat = 0;
    
    if (deviceConnected && millis() - lastHeartbeat > 10000) {
        // Send heartbeat every 10 seconds
        String heartbeat = "Heartbeat: " + String(millis());
        pCharacteristicTX->setValue(heartbeat.c_str());
        pCharacteristicTX->notify();
        Serial.println("💓 Heartbeat sent");
        lastHeartbeat = millis();
    }
    
    delay(100);
}