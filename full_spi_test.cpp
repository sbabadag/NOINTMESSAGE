#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

// From photo, the module has these pin labels:
// D0, D1(D1), RST, BUSY, NSS, RF_SW, MISO(A), MOSI(B), GND, VIN, SCK
// Let's try ALL possible SPI pin combinations

// D-pins from previous test:
// D0=1, D1=2, D2=3, D7=44

// Try the SPI pins we saw from Meshtastic (7,8,9)
#define LORA_SCK    9     // D10 from wio_sx1262_pins.h
#define LORA_MISO   8     // D9
#define LORA_MOSI   7     // D8

#define LORA_CS     44    // D7 = NSS
#define LORA_DIO1   2     // D1  
#define LORA_RESET  1     // D0 = RST
#define LORA_BUSY   3     // D2

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n╔════════════════════════════════════════╗");
  Serial.println("║  Full SPI Configuration Test          ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  Serial.println("📌 Pin Configuration:");
  Serial.printf("   CS/NSS (D7)  = GPIO %d\n", LORA_CS);
  Serial.printf("   DIO1 (D1)    = GPIO %d\n", LORA_DIO1);
  Serial.printf("   RESET (D0)   = GPIO %d\n", LORA_RESET);
  Serial.printf("   BUSY (D2)    = GPIO %d\n", LORA_BUSY);
  Serial.printf("   SCK (D10)    = GPIO %d\n", LORA_SCK);
  Serial.printf("   MISO (D9)    = GPIO %d\n", LORA_MISO);
  Serial.printf("   MOSI (D8)    = GPIO %d\n", LORA_MOSI);
  Serial.println();
  
  // IMPORTANT: Initialize SPI with explicit pins BEFORE RadioLib
  Serial.println("🔧 Initializing SPI with explicit pins...");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  delay(100);
  Serial.println("   SPI initialized");
  
  Serial.print("📡 Initializing SX1262... ");
  
  int state = radio.begin(915.0);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("SUCCESS! ✅");
    Serial.println();
    Serial.println("🎉🎉🎉 LORA WORKS! 🎉🎉🎉");
    Serial.println();
    Serial.println("Configuration:");
    Serial.println("   Frequency: 915.0 MHz");
    Serial.println("   Bandwidth: 125 kHz");
    Serial.println("   Spreading Factor: 9");
    Serial.println("   Coding Rate: 7");
    Serial.println("   TX Power: 10 dBm");
    Serial.println();
    Serial.println("🚀 Ready for LoRa communication!");
    
  } else {
    Serial.println("FAILED ❌");
    Serial.printf("   Error code: %d\n", state);
    
    if (state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
      Serial.println("   RADIOLIB_ERR_CHIP_NOT_FOUND");
    } else if (state == -2) {
      Serial.println("   Error -2: Parameter/SPI issue");
    }
    Serial.println();
    Serial.println("⚠️  Possible issues:");
    Serial.println("   1. Hardware not connected properly");
    Serial.println("   2. Module power issue");
    Serial.println("   3. Defective module");
    Serial.println("   4. Wrong pin configuration");
  }
}

void loop() {
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 2000) {
    Serial.println("💓");
    lastBlink = millis();
  }
  delay(100);
}
