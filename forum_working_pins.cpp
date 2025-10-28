#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

// EXACT pins from working forum example by Jer
// https://github.com/hpssjellis/maker100-xiao-esp32s3-sense/blob/main/LoRa-module/pingpong01.ino
#define LORA_MISO 8
#define LORA_SCK 7
#define LORA_MOSI 9
#define LORA_CS 41    //NSS
#define LORA_DIO2 38
#define LORA_DIO1 39  // irq
#define LORA_RESET 42
#define LORA_BUSY 40

// SX1262 has the following pinout:
// NSS pin:   41
// DIO1 pin:  39
// RESET pin: 42
// BUSY pin:  40
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n╔════════════════════════════════════════╗");
  Serial.println("║  WORKING FORUM PINS TEST               ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  Serial.println("Using EXACT pins from working forum example:");
  Serial.println("Source: https://github.com/hpssjellis/maker100-xiao-esp32s3-sense");
  Serial.println("Forum: https://forum.seeedstudio.com/t/284419");
  Serial.println();
  Serial.printf("   CS     = GPIO %d\n", LORA_CS);
  Serial.printf("   DIO1   = GPIO %d\n", LORA_DIO1);
  Serial.printf("   RESET  = GPIO %d\n", LORA_RESET);
  Serial.printf("   BUSY   = GPIO %d\n", LORA_BUSY);
  Serial.printf("   DIO2   = GPIO %d\n", LORA_DIO2);
  Serial.printf("   SCK    = GPIO %d\n", LORA_SCK);
  Serial.printf("   MISO   = GPIO %d\n", LORA_MISO);
  Serial.printf("   MOSI   = GPIO %d\n", LORA_MOSI);
  Serial.println();
  
  // Initialize SPI with EXACT pin order from working example
  Serial.println("🔧 Initializing SPI...");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  delay(100);
  
  Serial.print("📡 Initializing SX1262... ");
  
  // Use same parameters as working example
  //  frequency, bandwidth, spreadingFactor, codingRate, syncWord, outputPower, preambleLength
  int state = radio.begin(915.0, 125.0, 7, 5, 0x12, 14, 8);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("SUCCESS! ✅✅✅");
    Serial.println();
    Serial.println("🎉🎉🎉 IT FINALLY WORKS!!! 🎉🎉🎉");
    Serial.println();
    Serial.println("LoRa Configuration:");
    Serial.println("   Frequency: 915.0 MHz");
    Serial.println("   Bandwidth: 125 kHz");
    Serial.println("   Spreading Factor: 7");
    Serial.println("   Coding Rate: 5");
    Serial.println("   Sync Word: 0x12");
    Serial.println("   TX Power: 14 dBm");
    Serial.println("   Preamble Length: 8");
    Serial.println();
    Serial.println("🚀 LoRa ready for communication!");
    Serial.println("✅ Forum pins are CORRECT!");
    
  } else {
    Serial.println("FAILED ❌");
    Serial.printf("   Error code: %d\n", state);
    
    if (state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
      Serial.println("   RADIOLIB_ERR_CHIP_NOT_FOUND");
    } else if (state == -2) {
      Serial.println("   Error -2");
    }
    Serial.println();
    Serial.println("⚠️  Even forum pins failed!");
    Serial.println("   Possible hardware issue");
  }
}

void loop() {
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 3000) {
    Serial.println("💓 Heart");
    lastBlink = millis();
  }
  delay(100);
}
