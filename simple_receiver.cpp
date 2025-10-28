// SIMPLE MESSAGE RECEIVER - Basic working version
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
  
  Serial.println("\n=== SIMPLE MESSAGE RECEIVER ===");
  
  // Initialize SPI with custom pins
  SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI);
  delay(100);
  
  // Initialize LoRa with minimal settings
  Serial.print("LoRa init... ");
  int state = lora.begin(915.0, 125.0, 7, 5, 0x12, 22);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("OK!");
    loraReady = true;
    
    // Start listening for messages
    Serial.print("Starting receive... ");
    state = lora.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("OK!");
      Serial.println("\nListening for messages...");
    } else {
      Serial.printf("Failed (%d)\n", state);
    }
    
  } else {
    Serial.printf("FAILED (error %d)\n", state);
    
    // Try alternative initialization
    Serial.print("Trying alternative... ");
    state = lora.begin(915.0);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("OK!");
      loraReady = true;
      lora.startReceive();
      Serial.println("\nListening for messages...");
    } else {
      Serial.printf("Still failed (%d)\n", state);
    }
  }
  
  if (!loraReady) {
    Serial.println("\nLoRa not working - check wiring");
  }
}

void loop() {
  if (loraReady) {
    // Check for received messages
    if (lora.getPacketLength() > 0) {
      String message;
      int state = lora.readData(message);
      
      if (state == RADIOLIB_ERR_NONE) {
        Serial.println("\n┌─────────────────────────────┐");
        Serial.println("│      MESSAGE RECEIVED       │");
        Serial.println("└─────────────────────────────┘");
        Serial.printf("Message: \"%s\"\n", message.c_str());
        Serial.printf("RSSI: %.2f dBm\n", lora.getRSSI());
        Serial.printf("SNR: %.2f dB\n", lora.getSNR());
        Serial.printf("Length: %d bytes\n", message.length());
        Serial.println("═══════════════════════════════\n");
        
        // Restart listening
        lora.startReceive();
        
      } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
        Serial.printf("Read failed (%d)\n", state);
        lora.startReceive();
      }
    }
  }
  
  delay(10);
}