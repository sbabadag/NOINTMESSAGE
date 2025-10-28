#include <Arduino.h>
#include <SPI.h>
#include <SX126XLT.h>  // Stuart Robinson's library

// Pin definitions for our Wio SX1262
#define NSS    3
#define NRESET 6
#define RFBUSY 4
#define DIO1   5
#define LED1   -1  // No LED on XIAO

#define LORA_DEVICE DEVICE_SX1262

// LoRa settings
#define Frequency 915000000           // Frequency in Hz
#define Offset 0                       // Offset frequency
#define Bandwidth LORA_BW_125         // LoRa bandwidth
#define SpreadingFactor LORA_SF7      // LoRa spreading factor
#define CodeRate LORA_CR_4_5          // LoRa coding rate
#define Optimisation LDRO_AUTO        // Low data rate optimize

SX126XLT LT;  // Create instance

void setup()
{
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("🚀 SX12XX-LoRa Library Test");
  Serial.println("Using Stuart Robinson's Library");
  Serial.println("================================");
  
  // Initialize SPI FIRST (critical!)
  SPI.begin();
  Serial.println("✅ SPI initialized");
  
  // Setup hardware pins and check if device is found
  Serial.print("🔧 Initializing LoRa... ");
  if (LT.begin(NSS, NRESET, RFBUSY, DIO1, LORA_DEVICE))
  {
    Serial.println("SUCCESS!");
    Serial.println("✅ LoRa Device found and initialized!");
    
    // Setup LoRa parameters
    LT.setupLoRa(Frequency, Offset, SpreadingFactor, Bandwidth, CodeRate, Optimisation);
    
    Serial.println("\n📡 LoRa Configuration:");
    Serial.printf("   Frequency: %d Hz (%.1f MHz)\n", Frequency, Frequency/1000000.0);
    Serial.printf("   Bandwidth: 125 kHz\n");
    Serial.printf("   Spreading Factor: SF7\n");
    Serial.printf("   Coding Rate: 4/5\n");
    Serial.println("\n🎉 LoRa is ready for communication!");
    
  }
  else
  {
    Serial.println("FAILED!");
    Serial.println("❌ No LoRa device responding");
    Serial.println("\n💡 Troubleshooting:");
    Serial.println("   1. Check Wio SX1262 is properly connected");
    Serial.println("   2. Verify pin connections");
    Serial.println("   3. Check power supply");
    while (1);  // Halt
  }
}

void loop()
{
  static unsigned long lastTX = 0;
  static int count = 0;
  
  // Transmit every 10 seconds
  if (millis() - lastTX > 10000)
  {
    String message = "Hello LoRa " + String(count++);
    
    Serial.print("📤 Sending: ");
    Serial.println(message);
    
    // Transmit the message
    uint8_t TXPacketL = LT.transmit((uint8_t*)message.c_str(), message.length(), 10000, 10, WAIT_TX);
    
    if (TXPacketL > 0)
    {
      Serial.print("✅ Sent ");
      Serial.print(TXPacketL);
      Serial.println(" bytes successfully");
    }
    else
    {
      Serial.print("❌ Send failed: ");
      Serial.println(LT.getReliableStatusString());
    }
    
    lastTX = millis();
  }
  
  // Check for received packets
  uint8_t RXPacketL = LT.receive((uint8_t*)0, 0, 2000, WAIT_RX);
  
  if (RXPacketL > 0)
  {
    uint8_t buffer[256];
    LT.startReadSXBuffer(0);
    for (uint8_t i = 0; i < RXPacketL; i++)
    {
      buffer[i] = LT.readUint8();
    }
    LT.endReadSXBuffer();
    
    Serial.print("📨 Received: ");
    Serial.write(buffer, RXPacketL);
    Serial.println();
    Serial.printf("   RSSI: %d dBm, SNR: %d dB\n", LT.readPacketRSSI(), LT.readPacketSNR());
  }
  
  delay(100);
}
