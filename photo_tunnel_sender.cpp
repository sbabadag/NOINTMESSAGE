// SENDER STATION: Photo → LoRa Chunks
// This station receives a photo (via Serial or SD card) and transmits it over LoRa

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// Pin mapping for XIAO ESP32S3 + Wio SX1262
constexpr uint8_t PIN_LORA_NSS   = D7;  // GPIO44
constexpr uint8_t PIN_LORA_DIO1  = D1;  // GPIO2
constexpr uint8_t PIN_LORA_RESET = D0;  // GPIO1
constexpr uint8_t PIN_LORA_BUSY  = D2;  // GPIO3
constexpr uint8_t PIN_LORA_SCK   = D10; // GPIO9
constexpr uint8_t PIN_LORA_MISO  = D9;  // GPIO8
constexpr uint8_t PIN_LORA_MOSI  = D8;  // GPIO7

// LoRa Configuration
constexpr float LORA_FREQ = 915.0;      // Adjust for your region
constexpr float LORA_BW = 125.0;
constexpr uint8_t LORA_SF = 7;
constexpr uint8_t LORA_CR = 5;
constexpr int8_t LORA_POWER = 22;

// Photo transmission settings
constexpr uint16_t CHUNK_SIZE = 200;    // Bytes per LoRa packet (adjust based on SF)
constexpr uint8_t MAX_RETRIES = 3;
constexpr uint32_t ACK_TIMEOUT = 2000;  // ms to wait for ACK

// Packet types
enum PacketType : uint8_t {
  PKT_START = 0x01,      // Photo transmission start
  PKT_DATA = 0x02,       // Photo data chunk
  PKT_END = 0x03,        // Photo transmission end
  PKT_ACK = 0x04,        // Acknowledgment
  PKT_NACK = 0x05,       // Negative acknowledgment (request retransmit)
  PKT_PING = 0x06        // Keep-alive / connection test
};

// Packet header structure
struct PacketHeader {
  uint8_t type;
  uint32_t photoId;      // Unique ID for this photo
  uint16_t chunkIndex;   // Current chunk number
  uint16_t totalChunks;  // Total number of chunks
  uint16_t dataLen;      // Actual data length in this packet
  uint16_t crc;          // CRC16 of data
} __attribute__((packed));

SX1262 lora = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RESET, PIN_LORA_BUSY);
bool loraReady = false;

// Demo photo data (small test image)
// In real use, this would come from camera, SD card, or Serial
const uint8_t DEMO_PHOTO[] = {
  // JPEG header and small test data
  0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01,
  // Add more bytes here for a real photo...
  // For now, we'll simulate with pattern data
};

// Calculate CRC16
uint16_t calculateCRC16(const uint8_t* data, uint16_t length) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// Wait for acknowledgment
bool waitForAck(uint16_t chunkIndex) {
  uint32_t startTime = millis();
  
  while (millis() - startTime < ACK_TIMEOUT) {
    int16_t state = lora.receive();
    
    if (state == RADIOLIB_ERR_NONE) {
      uint8_t buffer[sizeof(PacketHeader)];
      int len = lora.getPacketLength();
      
      if (len >= sizeof(PacketHeader)) {
        state = lora.readData(buffer, len);
        
        if (state == RADIOLIB_ERR_NONE) {
          PacketHeader* hdr = (PacketHeader*)buffer;
          
          if (hdr->type == PKT_ACK && hdr->chunkIndex == chunkIndex) {
            Serial.printf("✓ ACK received for chunk %d\n", chunkIndex);
            return true;
          } else if (hdr->type == PKT_NACK && hdr->chunkIndex == chunkIndex) {
            Serial.printf("✗ NACK received for chunk %d (retransmit)\n", chunkIndex);
            return false;
          }
        }
      }
    }
    
    delay(10);
  }
  
  Serial.printf("⚠ ACK timeout for chunk %d\n", chunkIndex);
  return false;
}

// Send a single chunk with retries
bool sendChunkWithRetry(const uint8_t* photoData, uint32_t photoSize, 
                        uint32_t photoId, uint16_t chunkIndex, uint16_t totalChunks) {
  
  for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++) {
    if (attempt > 0) {
      Serial.printf("  Retry %d/%d for chunk %d\n", attempt, MAX_RETRIES, chunkIndex);
    }
    
    // Calculate chunk offset and size
    uint32_t offset = chunkIndex * CHUNK_SIZE;
    uint16_t dataLen = min((uint32_t)CHUNK_SIZE, photoSize - offset);
    
    // Prepare packet
    uint8_t packet[sizeof(PacketHeader) + CHUNK_SIZE];
    PacketHeader* hdr = (PacketHeader*)packet;
    
    hdr->type = PKT_DATA;
    hdr->photoId = photoId;
    hdr->chunkIndex = chunkIndex;
    hdr->totalChunks = totalChunks;
    hdr->dataLen = dataLen;
    hdr->crc = calculateCRC16(photoData + offset, dataLen);
    
    // Copy data after header
    memcpy(packet + sizeof(PacketHeader), photoData + offset, dataLen);
    
    // Transmit
    int16_t state = lora.transmit(packet, sizeof(PacketHeader) + dataLen);
    
    if (state == RADIOLIB_ERR_NONE) {
      Serial.printf("  Sent chunk %d/%d (%d bytes)\n", 
                    chunkIndex + 1, totalChunks, dataLen);
      
      // Wait for ACK
      lora.startReceive();
      if (waitForAck(chunkIndex)) {
        return true;
      }
    } else {
      Serial.printf("  TX error: %d\n", state);
    }
    
    delay(100); // Brief delay before retry
  }
  
  Serial.printf("✗ Failed to send chunk %d after %d attempts\n", chunkIndex, MAX_RETRIES);
  return false;
}

// Send entire photo
bool sendPhoto(const uint8_t* photoData, uint32_t photoSize) {
  if (photoSize == 0) {
    Serial.println("Error: Photo size is 0");
    return false;
  }
  
  uint32_t photoId = millis(); // Simple unique ID
  uint16_t totalChunks = (photoSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  STARTING PHOTO TRANSMISSION      ║");
  Serial.println("╚═══════════════════════════════════╝");
  Serial.printf("Photo ID: %u\n", photoId);
  Serial.printf("Size: %u bytes\n", photoSize);
  Serial.printf("Chunks: %u (each %u bytes)\n", totalChunks, CHUNK_SIZE);
  
  // Send START packet
  PacketHeader startPkt = {
    .type = PKT_START,
    .photoId = photoId,
    .chunkIndex = 0,
    .totalChunks = totalChunks,
    .dataLen = (uint16_t)photoSize,
    .crc = calculateCRC16(photoData, min(photoSize, (uint32_t)32)) // CRC of first 32 bytes
  };
  
  Serial.println("\n→ Sending START packet...");
  int16_t state = lora.transmit((uint8_t*)&startPkt, sizeof(startPkt));
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("✗ START packet failed: %d\n", state);
    return false;
  }
  
  delay(500); // Give receiver time to prepare
  
  // Send all chunks
  uint32_t startTime = millis();
  uint16_t successfulChunks = 0;
  
  for (uint16_t i = 0; i < totalChunks; i++) {
    Serial.printf("\n[%d/%d] ", i + 1, totalChunks);
    
    if (sendChunkWithRetry(photoData, photoSize, photoId, i, totalChunks)) {
      successfulChunks++;
      
      // Progress bar
      uint8_t progress = (successfulChunks * 100) / totalChunks;
      Serial.printf("Progress: %d%% ", progress);
      
      // Simple progress bar
      Serial.print("[");
      for (uint8_t p = 0; p < 20; p++) {
        Serial.print(p < (progress / 5) ? "=" : " ");
      }
      Serial.println("]");
    } else {
      Serial.println("✗ Chunk transmission failed, aborting");
      return false;
    }
  }
  
  // Send END packet
  Serial.println("\n→ Sending END packet...");
  PacketHeader endPkt = {
    .type = PKT_END,
    .photoId = photoId,
    .chunkIndex = totalChunks,
    .totalChunks = totalChunks,
    .dataLen = (uint16_t)photoSize,
    .crc = calculateCRC16(photoData + photoSize - 32, 32) // CRC of last 32 bytes
  };
  
  state = lora.transmit((uint8_t*)&endPkt, sizeof(endPkt));
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("✗ END packet failed: %d\n", state);
    return false;
  }
  
  uint32_t duration = millis() - startTime;
  float speed = (photoSize * 8.0) / (duration / 1000.0); // bits per second
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  TRANSMISSION COMPLETE!           ║");
  Serial.println("╚═══════════════════════════════════╝");
  Serial.printf("Duration: %u ms\n", duration);
  Serial.printf("Speed: %.2f bps (%.2f bytes/s)\n", speed, speed / 8);
  Serial.printf("Chunks: %d/%d successful\n", successfulChunks, totalChunks);
  
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║   PHOTO TUNNEL - SENDER STATION   ║");
  Serial.println("╚═══════════════════════════════════╝\n");
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Initialize LoRa
  Serial.print("Initializing SX1262... ");
  int16_t state = lora.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, 0x12, LORA_POWER);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✓ OK");
    loraReady = true;
    
    lora.setCurrentLimit(60.0);
    lora.setCRC(true);
    
    Serial.printf("Frequency: %.1f MHz\n", LORA_FREQ);
    Serial.printf("Bandwidth: %.1f kHz\n", LORA_BW);
    Serial.printf("Spreading Factor: %d\n", LORA_SF);
    Serial.printf("Coding Rate: 4/%d\n", LORA_CR);
    Serial.printf("TX Power: %d dBm\n", LORA_POWER);
    Serial.printf("Chunk Size: %d bytes\n", CHUNK_SIZE);
  } else {
    Serial.printf("✗ Failed (error %d)\n", state);
    Serial.println("Check wiring and restart");
    loraReady = false;
  }
  
  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("Commands:");
  Serial.println("  s - Send demo photo");
  Serial.println("  p - Send ping");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void loop() {
  static uint32_t lastBlink = 0;
  static bool ledState = false;
  
  // LED heartbeat
  if (millis() - lastBlink >= 500) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    lastBlink = millis();
  }
  
  // Check for serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    
    if (cmd == 's' || cmd == 'S') {
      if (loraReady) {
        // Create demo photo data (simulate a small JPEG)
        const uint32_t demoPhotoSize = 2048; // 2KB demo
        uint8_t* demoPhoto = new uint8_t[demoPhotoSize];
        
        // Fill with JPEG-like pattern
        demoPhoto[0] = 0xFF;
        demoPhoto[1] = 0xD8; // JPEG SOI
        for (uint32_t i = 2; i < demoPhotoSize - 2; i++) {
          demoPhoto[i] = (i * 137) % 256; // Pattern data
        }
        demoPhoto[demoPhotoSize - 2] = 0xFF;
        demoPhoto[demoPhotoSize - 1] = 0xD9; // JPEG EOI
        
        sendPhoto(demoPhoto, demoPhotoSize);
        delete[] demoPhoto;
      }
    } else if (cmd == 'p' || cmd == 'P') {
      if (loraReady) {
        Serial.println("→ Sending PING...");
        PacketHeader ping = {
          .type = PKT_PING,
          .photoId = millis(),
          .chunkIndex = 0,
          .totalChunks = 0,
          .dataLen = 0,
          .crc = 0
        };
        
        int16_t state = lora.transmit((uint8_t*)&ping, sizeof(ping));
        if (state == RADIOLIB_ERR_NONE) {
          Serial.println("✓ PING sent");
        } else {
          Serial.printf("✗ PING failed: %d\n", state);
        }
      }
    }
  }
}
