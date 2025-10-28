#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

// Using EXACT pins from Meshtastic firmware variants/esp32s3/seeed_xiao_s3/variant.h
// These are for the XIAO ESP32S3 with Wio-SX1262 module
#define LORA_CS     41    // LORA_CS from Meshtastic
#define LORA_DIO1   39    // LORA_DIO1 from Meshtastic
#define LORA_RESET  42    // LORA_RESET from Meshtastic
#define LORA_BUSY   40    // SX126X_BUSY from Meshtastic

// SPI pins from Meshtastic
#define LORA_SCK    7     // LORA_SCK
#define LORA_MISO   8     // LORA_MISO
#define LORA_MOSI   9     // LORA_MOSI

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n╔════════════════════════════════════════╗");
  Serial.println("║  MESHTASTIC Pin Configuration Test    ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  Serial.println("📌 Using EXACT pins from Meshtastic firmware:");
  Serial.printf("   CS (LORA_CS)       = GPIO %d\n", LORA_CS);
  Serial.printf("   DIO1 (LORA_DIO1)   = GPIO %d\n", LORA_DIO1);
  Serial.printf("   RESET (LORA_RESET) = GPIO %d\n", LORA_RESET);
  Serial.printf("   BUSY (SX126X_BUSY) = GPIO %d\n", LORA_BUSY);
  Serial.printf("   SCK (LORA_SCK)     = GPIO %d\n", LORA_SCK);
  Serial.printf("   MISO (LORA_MISO)   = GPIO %d\n", LORA_MISO);
  Serial.printf("   MOSI (LORA_MOSI)   = GPIO %d\n", LORA_MOSI);
  Serial.println();
  Serial.println("Source: meshtastic/firmware");
  Serial.println("File: variants/esp32s3/seeed_xiao_s3/variant.h");
  Serial.println();
  
  // Initialize custom SPI with Meshtastic pins
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  
  Serial.print("📡 Initializing SX1262... ");
  
  int state = radio.begin(915.0);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("SUCCESS! ✅");
    Serial.println();
    Serial.println("🎉🎉🎉 LoRa initialization SUCCEEDED! 🎉🎉🎉");
    Serial.println();
    Serial.println("Configuration:");
    Serial.println("   Frequency: 915.0 MHz");
    Serial.println("   Bandwidth: 125 kHz");
    Serial.println("   Spreading Factor: 9");
    Serial.println("   Coding Rate: 7");
    Serial.println("   TX Power: 10 dBm");
    Serial.println();
    Serial.println("🚀 Ready for LoRa communication!");
    Serial.println("✅ Meshtastic pins are CORRECT!");
    
  } else {
    Serial.println("FAILED ❌");
    Serial.printf("   Error code: %d\n", state);
    
    if (state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
      Serial.println("   RADIOLIB_ERR_CHIP_NOT_FOUND - No SX1262 detected");
    } else if (state == RADIOLIB_ERR_UNKNOWN) {
      Serial.println("   RADIOLIB_ERR_UNKNOWN");
    } else if (state == -2) {
      Serial.println("   Error -2: Invalid parameter or SPI communication issue");
    } else {
      Serial.printf("   Unknown error code\n");
    }
    Serial.println();
    Serial.println("❌ Meshtastic pins test failed!");
    Serial.println("⚠️  Check hardware connection!");
  }
}

void loop() {
  // Heartbeat every 2 seconds
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 2000) {
    Serial.println("💓 Heartbeat...");
    lastBlink = millis();
  }
  delay(100);
}
