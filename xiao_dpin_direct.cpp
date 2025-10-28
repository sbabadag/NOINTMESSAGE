#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

// Use XIAO ESP32S3 D-pin definitions directly
// The Wio SX1262 module connects to XIAO expansion pins D0-D10
// Arduino should have these defined for the XIAO board

// From the module labels visible in photo:
// NSS, BUSY, RST, D1, MISO, MOSI, SCK
// These map to XIAO expansion connector pins

SX1262 radio = new Module(D7, D1, D0, D2);  // CS=D7(NSS), DIO1=D1, RESET=D0(RST), BUSY=D2

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n╔════════════════════════════════════════╗");
  Serial.println("║  XIAO D-Pin Direct Test               ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  Serial.println("📌 Using D-pin Arduino constants:");
  Serial.printf("   CS = D7 (NSS pin on module)\n");
  Serial.printf("   DIO1 = D1\n");
  Serial.printf("   RESET = D0 (RST pin on module)\n");
  Serial.printf("   BUSY = D2\n");
  Serial.println("   SPI = default XIAO SPI pins");
  Serial.println();
  
  // Use default SPI (should auto-configure for XIAO)
  SPI.begin();
  
  Serial.print("📡 Initializing SX1262... ");
  
  int state = radio.begin(915.0);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("SUCCESS! ✅");
    Serial.println();
    Serial.println("🎉🎉🎉 IT WORKS! 🎉🎉🎉");
    Serial.println();
    Serial.println("✅ D-pin constants are correct!");
    Serial.println("Configuration:");
    Serial.println("   Frequency: 915.0 MHz");
    Serial.println("   Bandwidth: 125 kHz");
    Serial.println("   Spreading Factor: 9");
    Serial.println("   Coding Rate: 7");
    Serial.println("   TX Power: 10 dBm");
    Serial.println();
    Serial.println("🚀 LoRa ready for communication!");
    
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
    Serial.println("⚠️  D-pin test failed");
    
    // Print what D-pins resolve to
    Serial.println("\nD-pin GPIO mapping:");
    Serial.printf("   D0 = GPIO %d\n", D0);
    Serial.printf("   D1 = GPIO %d\n", D1);
    Serial.printf("   D2 = GPIO %d\n", D2);
    Serial.printf("   D7 = GPIO %d\n", D7);
  }
}

void loop() {
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 2000) {
    Serial.println("💓 Heartbeat...");
    lastBlink = millis();
  }
  delay(100);
}
