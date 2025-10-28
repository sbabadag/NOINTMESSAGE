// SIMPLE MESSAGE SENDER - Basic working version
#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// Pin mapping for XIAO ESP32S3 + Wio SX1262
#define PIN_LORA_NSS   44  // D7
#define PIN_LORA_DIO1  2   // D1  
#define PIN_LORA_RESET 1   // D0
#define PIN_LORA_BUSY  3   // D2
#define PIN_LORA_SCK   9   // D10
#define PIN_LORA_MISO  8   // D9
#define PIN_LORA_MOSI  7   // D8

SX1262 lora = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RESET, PIN_LORA_BUSY);
bool loraReady = false;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n=== SIMPLE MESSAGE SENDER ===");
  
  // Initialize SPI with custom pins
  SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI);
  delay(100);
  
  // Initialize LoRa with minimal settings
  Serial.print("LoRa init... ");
  int state = lora.begin(915.0, 125.0, 7, 5, 0x12, 22);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("OK!");
    loraReady = true;
  } else {
    Serial.printf("FAILED (error %d)\n", state);
    
    // Try alternative initialization
    Serial.print("Trying alternative... ");
    state = lora.begin(915.0);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("OK!");
      loraReady = true;
    } else {
      Serial.printf("Still failed (%d)\n", state);
    }
  }
  
  if (loraReady) {
    Serial.println("\nType messages and press Enter:");
  } else {
    Serial.println("\nLoRa not working - check wiring");
  }
}

void loop() {
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    
    if (msg.length() > 0 && loraReady) {
      Serial.printf("Sending: \"%s\"\n", msg.c_str());
      
      int state = lora.transmit(msg);
      if (state == RADIOLIB_ERR_NONE) {
        Serial.println("✓ Sent OK");
      } else {
        Serial.printf("✗ Send failed (%d)\n", state);
      }
    }
  }
  
  delay(100);
}