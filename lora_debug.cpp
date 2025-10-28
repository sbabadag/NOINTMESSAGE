#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// =================== PIN CONFIGURATION TESTS ===================
// We'll test multiple pin configurations to find the right one

// Configuration 1: Standard Wio SX1262 mapping
#define NSS_PIN_1     3     // GPIO 3 - SPI NSS (CS)
#define DIO1_PIN_1    5     // GPIO 5 - DIO1  
#define NRST_PIN_1    6     // GPIO 6 - RESET
#define BUSY_PIN_1    4     // GPIO 4 - BUSY

// Configuration 2: Alternative mapping
#define NSS_PIN_2     7     // GPIO 7 - SPI NSS (CS)
#define DIO1_PIN_2    1     // GPIO 1 - DIO1  
#define NRST_PIN_2    0     // GPIO 0 - RESET
#define BUSY_PIN_2    2     // GPIO 2 - BUSY

// Configuration 3: Another alternative
#define NSS_PIN_3     10    // GPIO 10 - SPI NSS (CS)
#define DIO1_PIN_3    8     // GPIO 8 - DIO1  
#define NRST_PIN_3    9     // GPIO 9 - RESET
#define BUSY_PIN_3    4     // GPIO 4 - BUSY

// Current configuration to test
int NSS_PIN = NSS_PIN_1;
int DIO1_PIN = DIO1_PIN_1;
int NRST_PIN = NRST_PIN_1;
int BUSY_PIN = BUSY_PIN_1;

SX1262* radio = nullptr;

// =================== HARDWARE TEST FUNCTIONS ===================

void testPinStates() {
    Serial.println("\n🔍 Testing pin states...");
    
    // Test NSS (should be HIGH when idle)
    pinMode(NSS_PIN, OUTPUT);
    digitalWrite(NSS_PIN, HIGH);
    Serial.printf("📌 NSS (GPIO %d): Set HIGH\n", NSS_PIN);
    
    // Test RESET (active LOW)
    pinMode(NRST_PIN, OUTPUT);
    digitalWrite(NRST_PIN, HIGH);
    Serial.printf("📌 RESET (GPIO %d): Set HIGH (inactive)\n", NRST_PIN);
    
    // Test BUSY (input, should read chip busy state)
    pinMode(BUSY_PIN, INPUT);
    bool busyState = digitalRead(BUSY_PIN);
    Serial.printf("📌 BUSY (GPIO %d): Reading %s\n", BUSY_PIN, busyState ? "HIGH" : "LOW");
    
    // Test DIO1 (input, interrupt pin)
    pinMode(DIO1_PIN, INPUT);
    bool dio1State = digitalRead(DIO1_PIN);
    Serial.printf("📌 DIO1 (GPIO %d): Reading %s\n", DIO1_PIN, dio1State ? "HIGH" : "LOW");
}

void performHardReset() {
    Serial.println("🔄 Performing hardware reset...");
    
    // Pull reset low for 10ms, then high
    digitalWrite(NRST_PIN, LOW);
    delay(10);
    digitalWrite(NRST_PIN, HIGH);
    delay(100);
    
    Serial.println("✅ Hardware reset completed");
}

bool testSPICommunication() {
    Serial.println("\n🔍 Testing SPI communication...");
    
    // Initialize SPI
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV16); // Slower clock for testing
    
    // Test SPI by trying to read a register
    digitalWrite(NSS_PIN, LOW);
    delayMicroseconds(1);
    
    // Send read command for version register (usually 0x42 for SX126x)
    uint8_t cmd = 0x1D; // Read register command for SX126x
    uint8_t addr_high = 0x03; // Version register high byte
    uint8_t addr_low = 0x20;  // Version register low byte
    
    SPI.transfer(cmd);
    SPI.transfer(addr_high);
    SPI.transfer(addr_low);
    SPI.transfer(0x00); // NOP
    uint8_t result = SPI.transfer(0x00); // Read result
    
    digitalWrite(NSS_PIN, HIGH);
    delayMicroseconds(1);
    
    Serial.printf("📡 SPI Test - Register read result: 0x%02X\n", result);
    
    // Check if we got a reasonable response (not 0x00 or 0xFF)
    if (result != 0x00 && result != 0xFF) {
        Serial.println("✅ SPI communication appears to be working");
        return true;
    } else {
        Serial.println("❌ SPI communication failed or no response");
        return false;
    }
}

bool testLoRaInitialization(int config) {
    Serial.printf("\n🧪 Testing LoRa configuration %d...\n", config);
    
    // Set pins based on configuration
    switch(config) {
        case 1:
            NSS_PIN = NSS_PIN_1; DIO1_PIN = DIO1_PIN_1; NRST_PIN = NRST_PIN_1; BUSY_PIN = BUSY_PIN_1;
            break;
        case 2:
            NSS_PIN = NSS_PIN_2; DIO1_PIN = DIO1_PIN_2; NRST_PIN = NRST_PIN_2; BUSY_PIN = BUSY_PIN_2;
            break;
        case 3:
            NSS_PIN = NSS_PIN_3; DIO1_PIN = DIO1_PIN_3; NRST_PIN = NRST_PIN_3; BUSY_PIN = BUSY_PIN_3;
            break;
        default:
            return false;
    }
    
    Serial.printf("📌 Pins: NSS=%d, DIO1=%d, RESET=%d, BUSY=%d\n", NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN);
    
    // Clean up previous radio instance
    if (radio != nullptr) {
        delete radio;
        radio = nullptr;
    }
    
    // Create new radio instance
    radio = new SX1262(new Module(NSS_PIN, DIO1_PIN, NRST_PIN, BUSY_PIN));
    
    // Test pin states
    testPinStates();
    
    // Perform hardware reset
    performHardReset();
    
    // Test SPI communication
    bool spiWorking = testSPICommunication();
    if (!spiWorking) {
        Serial.println("❌ SPI test failed for this configuration");
        return false;
    }
    
    // Try to initialize LoRa
    Serial.println("🔧 Attempting radio.begin()...");
    int state = radio->begin();
    Serial.printf("📡 radio.begin() returned: %d\n", state);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("✅ SUCCESS! Configuration %d works!\n", config);
        
        // Try basic configuration
        state = radio->setFrequency(915.0);
        Serial.printf("📡 setFrequency(915.0): %d\n", state);
        
        state = radio->setBandwidth(125.0);
        Serial.printf("📡 setBandwidth(125.0): %d\n", state);
        
        state = radio->setSpreadingFactor(7);
        Serial.printf("📡 setSpreadingFactor(7): %d\n", state);
        
        return true;
    } else {
        Serial.printf("❌ Configuration %d failed with error %d\n", config, state);
        
        // Print error meaning
        switch(state) {
            case -2: Serial.println("   RADIOLIB_ERR_INVALID_PARAMETER"); break;
            case -3: Serial.println("   RADIOLIB_ERR_UNSUPPORTED"); break;
            case -4: Serial.println("   RADIOLIB_ERR_UNKNOWN"); break;
            case -5: Serial.println("   RADIOLIB_ERR_CHIP_NOT_FOUND"); break;
            default: Serial.printf("   Unknown error code: %d\n", state); break;
        }
        
        return false;
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=======================================");
    Serial.println("🔧 LoRa Hardware Debug Tool");
    Serial.println("=======================================");
    Serial.println("🎯 Goal: Find working pin configuration");
    Serial.println("📋 Testing multiple pin combinations...");
    Serial.println("");
    
    // Test each configuration
    for (int config = 1; config <= 3; config++) {
        bool success = testLoRaInitialization(config);
        
        if (success) {
            Serial.println("\n🎉 FOUND WORKING CONFIGURATION!");
            Serial.printf("✅ Use these pins in your main code:\n");
            Serial.printf("   NSS_PIN = %d\n", NSS_PIN);
            Serial.printf("   DIO1_PIN = %d\n", DIO1_PIN);
            Serial.printf("   NRST_PIN = %d\n", NRST_PIN);
            Serial.printf("   BUSY_PIN = %d\n", BUSY_PIN);
            Serial.println("\n🎯 LoRa hardware is working!");
            
            // Try a test transmission
            Serial.println("\n📡 Testing transmission...");
            String testMsg = "LoRa Test " + String(millis());
            int txState = radio->transmit(testMsg);
            if (txState == RADIOLIB_ERR_NONE) {
                Serial.println("✅ Test transmission successful!");
            } else {
                Serial.printf("⚠️  Transmission failed: %d\n", txState);
            }
            
            break;
        }
        
        Serial.println("───────────────────────────────────────");
        delay(1000);
    }
    
    if (radio == nullptr || !radio) {
        Serial.println("\n❌ NO WORKING CONFIGURATION FOUND");
        Serial.println("💡 Possible issues:");
        Serial.println("   1. Hardware not connected properly");
        Serial.println("   2. Power supply issue");
        Serial.println("   3. Defective LoRa module");
        Serial.println("   4. Incorrect pin mapping for your board");
        Serial.println("\n🔍 Manual checks needed:");
        Serial.println("   - Verify Wio SX1262 is properly seated");
        Serial.println("   - Check power LED on the module");
        Serial.println("   - Verify ESP32S3 board variant");
    }
}

void loop() {
    // Keep monitoring
    static unsigned long lastPrint = 0;
    
    if (millis() - lastPrint > 5000) {
        if (radio != nullptr) {
            Serial.println("💓 LoRa hardware test running...");
            
            // Check BUSY pin state
            bool busyState = digitalRead(BUSY_PIN);
            Serial.printf("📡 BUSY pin: %s\n", busyState ? "HIGH" : "LOW");
        } else {
            Serial.println("❌ No working LoRa configuration found");
        }
        lastPrint = millis();
    }
    
    delay(1000);
}