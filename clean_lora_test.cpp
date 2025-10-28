#include <Arduino.h>
#include <RadioLib.h>

// Exact pin configuration from our working debug discovery
// SX1262 connections:
// NSS pin:   3
// DIO1 pin:  5
// NRST pin:  6
// BUSY pin:  4
SX1262 radio = new Module(3, 5, 6, 4);

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("🚀 Clean LoRa Test - Following RadioLib Examples Exactly");
    
    // Initialize SX1262 with default settings - EXACTLY like RadioLib examples
    Serial.print("[SX1262] Initializing ... ");
    int state = radio.begin();
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("success!");
        Serial.println("✅ LoRa is working!");
        Serial.println("📡 Ready to communicate");
        
        // Start receiving
        radio.startReceive();
        
    } else {
        Serial.print("failed, code ");
        Serial.println(state);
        
        switch(state) {
            case -2: Serial.println("   RADIOLIB_ERR_INVALID_PARAMETER"); break;
            case -3: Serial.println("   RADIOLIB_ERR_UNSUPPORTED"); break;
            case -4: Serial.println("   RADIOLIB_ERR_UNKNOWN"); break;
            case -5: Serial.println("   RADIOLIB_ERR_CHIP_NOT_FOUND"); break;
            default: Serial.printf("   Error code: %d\n", state); break;
        }
    }
}

void loop() {
    static unsigned long lastSend = 0;
    
    // Check for received packets
    String received;
    int state = radio.readData(received);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.print("📨 Received: ");
        Serial.println(received);
        Serial.print("   RSSI: ");
        Serial.print(radio.getRSSI());
        Serial.println(" dBm");
        Serial.print("   SNR: ");
        Serial.print(radio.getSNR());
        Serial.println(" dB");
    }
    
    // Send test message every 10 seconds
    if (millis() - lastSend > 10000) {
        String message = "Hello LoRa! " + String(millis());
        Serial.print("📤 Sending: ");
        Serial.println(message);
        
        state = radio.transmit(message);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println("✅ Sent successfully");
        } else {
            Serial.print("❌ Send failed: ");
            Serial.println(state);
        }
        
        // Start receiving again
        radio.startReceive();
        
        lastSend = millis();
    }
    
    delay(100);
}
