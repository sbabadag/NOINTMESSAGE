#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

// Try using the D-pin macros directly
// According to wio_sx1262_pins.h:
// D0 = GPIO1 (RESET), D1 = GPIO2 (DIO1), D2 = GPIO3 (BUSY), D7 = GPIO44 (CS)
// D8 = GPIO7 (MOSI), D9 = GPIO8 (MISO), D10 = GPIO9 (SCK)

// Let's use the actual GPIO numbers from the wio_sx1262_pins.h mapping
#define LORA_CS     44    // D7
#define LORA_DIO1   2     // D1
#define LORA_RESET  1     // D0
#define LORA_BUSY   3     // D2

// SPI pins
#define LORA_SCK    9     // D10
#define LORA_MISO   8     // D9
#define LORA_MOSI   7     // D8

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n╔════════════════════════════════════════╗");
  Serial.println("║  Wio SX1262 D-Pin Test                ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  Serial.println("📌 Using D-pin GPIO mapping from wio_sx1262_pins.h:");
  Serial.printf("   CS (D7)    = GPIO %d\n", LORA_CS);
  Serial.printf("   DIO1 (D1)  = GPIO %d\n", LORA_DIO1);
  Serial.printf("   RESET (D0) = GPIO %d\n", LORA_RESET);
  Serial.printf("   BUSY (D2)  = GPIO %d\n", LORA_BUSY);
  Serial.printf("   SCK (D10)  = GPIO %d\n", LORA_SCK);
  Serial.printf("   MISO (D9)  = GPIO %d\n", LORA_MISO);
  Serial.printf("   MOSI (D8)  = GPIO %d\n", LORA_MOSI);
  Serial.println();
  
  // Initialize custom SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  
  Serial.print("📡 Initializing SX1262... ");
  
  int state = radio.begin(915.0);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("SUCCESS! ✅");
    Serial.println();
    Serial.println("🎉 LoRa initialization succeeded with D-pin mapping!");
    Serial.println("   Frequency: 915.0 MHz");
    Serial.println("   Bandwidth: 125 kHz");
    Serial.println("   Spreading Factor: 9");
    Serial.println("   Coding Rate: 7");
    Serial.println("   TX Power: 10 dBm");
    Serial.println();
    Serial.println("🚀 Ready for communication!");
    
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
    Serial.println("❌ D-pin mapping test failed!");
  }
}

void loop() {
  // Heartbeat
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    Serial.println("💓 Heartbeat...");
    lastBlink = millis();
  }
  delay(100);
}
