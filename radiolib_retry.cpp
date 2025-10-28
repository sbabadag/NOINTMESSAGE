#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// Working pins from debug tool
#define NSS    3
#define DIO1   5
#define NRST   6
#define BUSY   4

SX1262 radio = new Module(NSS, DIO1, NRST, BUSY);

bool testSPICommunication() {
    Serial.println("\n🔍 Testing SPI communication manually...");
    
    pinMode(NSS, OUTPUT);
    pinMode(NRST, OUTPUT);
    pinMode(BUSY, INPUT);
    pinMode(DIO1, INPUT);
    
    // Hardware reset first
    digitalWrite(NRST, LOW);
    delay(10);
    digitalWrite(NRST, HIGH);
    delay(100);
    
    // Initialize SPI with slower clock
    SPI.begin(8, 9, 10, NSS);  // Custom pins for XIAO
    SPI.setClockDivider(SPI_CLOCK_DIV16); // Slower clock
    Serial.println("✅ SPI initialized (slower clock)");
    
    // Manual SPI test - read register
    digitalWrite(NSS, LOW);
    delayMicroseconds(1);
    
    uint8_t cmd = 0x1D; // Read register command
    uint8_t addr_high = 0x03;
    uint8_t addr_low = 0x20;
    
    SPI.transfer(cmd);
    SPI.transfer(addr_high);
    SPI.transfer(addr_low);
    SPI.transfer(0x00);
    uint8_t result = SPI.transfer(0x00);
    
    digitalWrite(NSS, HIGH);
    delayMicroseconds(1);
    
    Serial.printf("📡 SPI Test - Register read: 0x%02X\n", result);
    
    if (result != 0x00 && result != 0xFF) {
        Serial.println("✅ SPI communication working!");
        return true;
    } else {
        Serial.println("❌ SPI communication failed");
        return false;
    }
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    
    Serial.println();
    Serial.println("=================================");
    Serial.println("🔧 RadioLib Retry with Manual SPI Test");
    Serial.println("=================================");
    
    // Test SPI first (like debug tool did)
    if (!testSPICommunication()) {
        Serial.println("❌ SPI test failed - stopping");
        while(1);
    }
    
    Serial.println("\n🔧 Now trying radio.begin()...");
    
    // Now try RadioLib initialization
    int state = radio.begin();
    
    Serial.printf("📡 radio.begin() returned: %d\n", state);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("✅ LoRa initialized successfully!");
        
        // Configure
        radio.setFrequency(915.0);
        radio.setBandwidth(125.0);
        radio.setSpreadingFactor(7);
        radio.setCodingRate(5);
        radio.setOutputPower(10);
        
        Serial.println("🎉 LoRa is ready!");
    } else {
        Serial.printf("❌ radio.begin() failed with error: %d\n", state);
        
        switch(state) {
            case -2: Serial.println("   RADIOLIB_ERR_INVALID_PARAMETER"); break;
            case -3: Serial.println("   RADIOLIB_ERR_UNSUPPORTED"); break;
            case -4: Serial.println("   RADIOLIB_ERR_UNKNOWN"); break;
            case -5: Serial.println("   RADIOLIB_ERR_CHIP_NOT_FOUND"); break;
            default: Serial.printf("   Unknown error: %d\n", state); break;
        }
        
        Serial.println("\n💡 Even though SPI works, begin() fails.");
        Serial.println("   This suggests a timing or initialization sequence issue.");
        while(1);
    }
}

void loop() {
    static unsigned long lastTX = 0;
    static int count = 0;
    
    if (millis() - lastTX > 5000) {
        Serial.printf("💓 Heartbeat #%d - LoRa still running\n", count++);
        lastTX = millis();
    }
}
