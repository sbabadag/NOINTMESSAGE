#include <Arduino.h>
#include <NimBLEDevice.h>

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("===========================");
    Serial.println("🚀 Minimal BLE Test");
    Serial.println("===========================");
    
    // Initialize NimBLE device
    Serial.println("🔧 Initializing BLE...");
    NimBLEDevice::init("LoRa_Tunnel");
    Serial.println("✅ BLE initialized");
    
    // Create BLE Server
    Serial.println("🔧 Creating BLE server...");
    NimBLEServer* pServer = NimBLEDevice::createServer();
    Serial.println("✅ BLE server created");
    
    // Create BLE Service using 16-bit UUID
    Serial.println("🔧 Creating BLE service 0xFFE0...");
    NimBLEService *pService = pServer->createService("FFE0");
    Serial.println("✅ BLE service created");
    
    // Start the service
    Serial.println("🔧 Starting BLE service...");
    pService->start();
    Serial.println("✅ BLE service started");
    
    // Start advertising
    Serial.println("🔧 Starting BLE advertising...");
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("FFE0");
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    pAdvertising->start();
    Serial.println("✅ BLE advertising started");
    
    Serial.println("");
    Serial.println("🎯 BLE Test Ready!");
    Serial.println("📱 Look for 'LoRa_Tunnel' in nRF Connect");
    Serial.println("📋 Service UUID: FFE0");
    Serial.println("");
}

void loop() {
    static unsigned long lastPrint = 0;
    
    if (millis() - lastPrint > 5000) {
        Serial.println("💓 BLE device running...");
        lastPrint = millis();
    }
    
    delay(1000);
}