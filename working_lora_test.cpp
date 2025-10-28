#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// Working configuration (discovered by debug tool)
#define NSS_PIN     3
#define DIO1_PIN    5  
#define NRST_PIN    6
#define BUSY_PIN    4

SX1262* radio = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("🚀 LoRa Communication Test - Based on Working Debug");
    
    // Use exact same initialization as debug tool
    radio = new SX1262(new Module(NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN));
    
    // Hardware reset (like debug tool)
    pinMode(NRST_PIN, OUTPUT);
    digitalWrite(NRST_PIN, LOW);
    delay(10);
    digitalWrite(NRST_PIN, HIGH);
    delay(100);
    
    // Initialize LoRa (like debug tool)
    int state = radio->begin();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("✅ LoRa initialized successfully!");
        
        // Configure (like debug tool)
        radio->setFrequency(915.0);
        radio->setBandwidth(125.0);
        radio->setSpreadingFactor(7);
        
        Serial.println("📡 LoRa ready for communication");
        Serial.println("💬 Type messages in serial monitor to send via LoRa");
        
        // Start receiving
        radio->startReceive();
        
    } else {
        Serial.printf("❌ LoRa init failed: %d\n", state);
    }
}

void loop() {
    static unsigned long lastSend = 0;
    
    // Check for incoming messages
    if (radio != nullptr) {
        String received;
        int state = radio->readData(received);
        
        if (state == RADIOLIB_ERR_NONE) {
            Serial.printf("📨 Received: %s\n", received.c_str());
            Serial.printf("   RSSI: %.1f dBm\n", radio->getRSSI());
            Serial.printf("   SNR: %.1f dB\n", radio->getSNR());
            
            // Start receiving again
            radio->startReceive();
        }
    }
    
    // Send test message every 10 seconds
    if (millis() - lastSend > 10000) {
        if (radio != nullptr) {
            String message = "Hello LoRa! " + String(millis());
            Serial.printf("📤 Sending: %s\n", message.c_str());
            
            int state = radio->transmit(message);
            if (state == RADIOLIB_ERR_NONE) {
                Serial.println("✅ Message sent successfully");
            } else {
                Serial.printf("❌ Send failed: %d\n", state);
            }
            
            // Return to receive mode
            radio->startReceive();
        }
        lastSend = millis();
    }
    
    delay(100);
}