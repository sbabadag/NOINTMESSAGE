#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// Working configuration from debug tool
#define NSS_PIN     3
#define DIO1_PIN    5  
#define NRST_PIN    6
#define BUSY_PIN    4

SX1262* radio = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("🔧 Minimal LoRa Test - Configuration 1");
    
    // Exact same sequence as working debug tool
    Serial.printf("📌 Pins: NSS=%d, DIO1=%d, RESET=%d, BUSY=%d\n", NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN);
    
    // Create radio instance
    radio = new SX1262(new Module(NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN));
    
    // Test pin states
    pinMode(NSS_PIN, OUTPUT);
    digitalWrite(NSS_PIN, HIGH);
    Serial.printf("📌 NSS (GPIO %d): Set HIGH\n", NSS_PIN);
    
    pinMode(NRST_PIN, OUTPUT);
    digitalWrite(NRST_PIN, HIGH);
    Serial.printf("📌 RESET (GPIO %d): Set HIGH (inactive)\n", NRST_PIN);
    
    pinMode(BUSY_PIN, INPUT);
    Serial.printf("📌 BUSY (GPIO %d): Input mode, current state: %s\n", 
                  BUSY_PIN, digitalRead(BUSY_PIN) ? "HIGH" : "LOW");
    
    // Perform hardware reset
    Serial.println("🔧 Hardware reset sequence...");
    digitalWrite(NRST_PIN, LOW);
    delay(10);
    digitalWrite(NRST_PIN, HIGH);
    delay(100);
    
    // Try to initialize LoRa
    Serial.println("🔧 Attempting radio.begin()...");
    int state = radio->begin();
    Serial.printf("📡 radio.begin() returned: %d\n", state);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("✅ SUCCESS! Configuration works!\n");
        
        // Try basic configuration
        state = radio->setFrequency(915.0);
        Serial.printf("📡 setFrequency(915.0): %d\n", state);
        
        state = radio->setBandwidth(125.0);
        Serial.printf("📡 setBandwidth(125.0): %d\n", state);
        
        state = radio->setSpreadingFactor(7);
        Serial.printf("📡 setSpreadingFactor(7): %d\n", state);
        
        Serial.println("🎉 LoRa is working perfectly!");
        
    } else {
        Serial.printf("❌ Failed with error %d\n", state);
        switch(state) {
            case -2: Serial.println("   RADIOLIB_ERR_INVALID_PARAMETER"); break;
            case -3: Serial.println("   RADIOLIB_ERR_UNSUPPORTED"); break;
            case -4: Serial.println("   RADIOLIB_ERR_UNKNOWN"); break;
            case -5: Serial.println("   RADIOLIB_ERR_CHIP_NOT_FOUND"); break;
            default: Serial.printf("   Unknown error code: %d\n", state); break;
        }
    }
}

void loop() {
    delay(5000);
    if (radio) {
        Serial.println("💓 LoRa heartbeat - ready for messages");
    } else {
        Serial.println("❌ LoRa not initialized");
    }
}