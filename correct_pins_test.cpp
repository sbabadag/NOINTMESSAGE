#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// ============================================
// CORRECT PINS for Wio SX1262 with XIAO ESP32S3
// From Meshtastic firmware variant definition
// ============================================
#define LORA_MISO  8
#define LORA_SCK   7
#define LORA_MOSI  9
#define LORA_CS    41  // NSS

#define LORA_RESET 42
#define LORA_DIO1  39
#define LORA_BUSY  40

// Create radio instance with CORRECT pins
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("🎯 CORRECT PIN TEST - Wio SX1262 + XIAO ESP32S3");
  Serial.println("========================================");
  Serial.println("📌 Using CORRECT Meshtastic-verified pins:");
  Serial.printf("   SPI: SCK=%d, MISO=%d, MOSI=%d\n", LORA_SCK, LORA_MISO, LORA_MOSI);
  Serial.printf("   CS (NSS)  = GPIO %d\n", LORA_CS);
  Serial.printf("   DIO1      = GPIO %d\n", LORA_DIO1);
  Serial.printf("   BUSY      = GPIO %d\n", LORA_BUSY);
  Serial.printf("   RESET     = GPIO %d\n", LORA_RESET);
  Serial.println();
  
  // Initialize SPI with CORRECT pins
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  Serial.println("✅ SPI initialized with correct pins");
  
  // Initialize LoRa
  Serial.print("🔧 Initializing SX1262... ");
  
  int state = radio.begin(915.0);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("SUCCESS! 🎉🎉🎉");
    Serial.println();
    Serial.println("✅ LoRa module initialized successfully!");
    Serial.println("✅ Pins are CORRECT!");
    Serial.println();
    
    // Configure LoRa settings
    radio.setFrequency(915.0);
    radio.setBandwidth(125.0);
    radio.setSpreadingFactor(7);
    radio.setCodingRate(5);
    radio.setOutputPower(10);
    
    Serial.println("📡 LoRa Configuration:");
    Serial.println("   Frequency: 915.0 MHz");
    Serial.println("   Bandwidth: 125.0 kHz");
    Serial.println("   Spreading Factor: 7");
    Serial.println("   Coding Rate: 4/5");
    Serial.println("   TX Power: 10 dBm");
    Serial.println();
    Serial.println("🚀 Ready for communication!");
    
  } else {
    Serial.println("FAILED ❌");
    Serial.printf("   Error code: %d\n", state);
    
    switch(state) {
      case RADIOLIB_ERR_INVALID_PARAMETER:
        Serial.println("   RADIOLIB_ERR_INVALID_PARAMETER");
        break;
      case RADIOLIB_ERR_CHIP_NOT_FOUND:
        Serial.println("   RADIOLIB_ERR_CHIP_NOT_FOUND");
        break;
      case RADIOLIB_ERR_UNKNOWN:
        Serial.println("   RADIOLIB_ERR_UNKNOWN");
        break;
      default:
        Serial.printf("   Unknown error: %d\n", state);
        break;
    }
    
    Serial.println();
    Serial.println("❌ If this still fails, there may be a hardware issue.");
    while(1);
  }
}

void loop() {
  static unsigned long lastTX = 0;
  static int msgCount = 0;
  
  // Transmit every 5 seconds
  if (millis() - lastTX > 5000) {
    String message = "Test message #" + String(msgCount++);
    
    Serial.print("📤 Transmitting: ");
    Serial.println(message);
    
    int state = radio.transmit(message);
    
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("   ✅ Transmission successful!");
    } else {
      Serial.printf("   ❌ Transmission failed: %d\n", state);
    }
    
    lastTX = millis();
  }
  
  delay(100);
}
