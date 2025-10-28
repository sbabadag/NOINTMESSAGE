#include <Arduino.h>
#include <SPI.h>

// Test SX1262 SPI communication directly
#define NSS_PIN  3
#define BUSY_PIN 4
#define NRST_PIN 6

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("🔍 Direct SPI Communication Test");
    Serial.println("=====================================");
    
    // Initialize pins
    pinMode(NSS_PIN, OUTPUT);
    pinMode(NRST_PIN, OUTPUT);
    pinMode(BUSY_PIN, INPUT);
    
    digitalWrite(NSS_PIN, HIGH);
    digitalWrite(NRST_PIN, HIGH);
    
    Serial.printf("NSS: %d, BUSY: %d, NRST: %d\n", NSS_PIN, BUSY_PIN, NRST_PIN);
    
    // Initialize SPI
    SPI.begin();
    
    Serial.println("\n📌 Initial Pin States:");
    Serial.printf("   NSS: %s\n", digitalRead(NSS_PIN) ? "HIGH" : "LOW");
    Serial.printf("   BUSY: %s\n", digitalRead(BUSY_PIN) ? "HIGH" : "LOW");
    Serial.printf("   RESET: %s\n", digitalRead(NRST_PIN) ? "HIGH" : "LOW");
    
    // Hardware reset
    Serial.println("\n🔄 Performing hardware reset...");
    digitalWrite(NRST_PIN, LOW);
    delay(10);
    digitalWrite(NRST_PIN, HIGH);
    delay(100);
    
    Serial.println("📌 After Reset:");
    Serial.printf("   BUSY: %s\n", digitalRead(BUSY_PIN) ? "HIGH" : "LOW");
    
    // Try to read SX1262 register
    Serial.println("\n📡 Attempting SPI communication...");
    
    // Wait for BUSY to go LOW
    int timeout = 1000;
    while(digitalRead(BUSY_PIN) && timeout > 0) {
        delay(1);
        timeout--;
    }
    
    if (timeout == 0) {
        Serial.println("⚠️  BUSY pin stuck HIGH!");
    } else {
        Serial.println("✅ BUSY pin is LOW");
    }
    
    // Try to read a register (GetStatus command)
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(NSS_PIN, LOW);
    delay(1);
    
    uint8_t status = SPI.transfer(0xC0);  // GetStatus command
    SPI.transfer(0x00);  // NOP
    
    digitalWrite(NSS_PIN, HIGH);
    SPI.endTransaction();
    
    Serial.printf("📊 Status response: 0x%02X\n", status);
    
    if (status == 0x00 || status == 0xFF) {
        Serial.println("❌ No valid response - possible hardware issue");
        Serial.println("\n💡 Troubleshooting:");
        Serial.println("   1. Check if Wio SX1262 is properly connected");
        Serial.println("   2. Verify power supply (3.3V)");
        Serial.println("   3. Try reseating the module");
        Serial.println("   4. Check for physical damage");
    } else {
        Serial.println("✅ Valid SPI response received!");
        Serial.println("   Hardware communication is working");
    }
}

void loop() {
    delay(5000);
    Serial.println("💓 Diagnostic running...");
    Serial.printf("   BUSY: %s\n", digitalRead(BUSY_PIN) ? "HIGH" : "LOW");
}
