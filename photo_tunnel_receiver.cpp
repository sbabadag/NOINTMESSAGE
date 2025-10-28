// RECEIVER STATION: LoRa Chunks → Photo → BLE to Phone
// This station receives LoRa chunks, reassembles the photo, and sends it to a phone via BLE

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <NimBLEDevice.h>

// Pin mapping for XIAO ESP32S3 + Wio SX1262
constexpr uint8_t PIN_LORA_NSS   = D7;  // GPIO44
constexpr uint8_t PIN_LORA_DIO1  = D1;  // GPIO2
constexpr uint8_t PIN_LORA_RESET = D0;  // GPIO1
constexpr uint8_t PIN_LORA_BUSY  = D2;  // GPIO3
constexpr uint8_t PIN_LORA_SCK   = D10; // GPIO9
constexpr uint8_t PIN_LORA_MISO  = D9;  // GPIO8
constexpr uint8_t PIN_LORA_MOSI  = D8;  // GPIO7

// LoRa Configuration (must match sender)
constexpr float LORA_FREQ = 915.0;
constexpr float LORA_BW = 125.0;
constexpr uint8_t LORA_SF = 7;
constexpr uint8_t LORA_CR = 5;

// BLE Configuration
const char* BLE_DEVICE_NAME = "PhotoTunnel";
const char* SERVICE_UUID = "12345678-1234-1234-1234-123456789abc";
const char* CHAR_PHOTO_DATA_UUID = "12345678-1234-1234-1234-123456789abd";
const char* CHAR_PHOTO_INFO_UUID = "12345678-1234-1234-1234-123456789abe";
const char* CHAR_STATUS_UUID = "12345678-1234-1234-1234-123456789abf";

// Photo reception settings
constexpr uint16_t CHUNK_SIZE = 200;
constexpr uint32_t MAX_PHOTO_SIZE = 100000; // 100KB max
constexpr uint16_t BLE_MTU = 512;           // BLE MTU size

// Packet types (must match sender)
enum PacketType : uint8_t {
  PKT_START = 0x01,
  PKT_DATA = 0x02,
  PKT_END = 0x03,
  PKT_ACK = 0x04,
  PKT_NACK = 0x05,
  PKT_PING = 0x06
};

struct PacketHeader {
  uint8_t type;
  uint32_t photoId;
  uint16_t chunkIndex;
  uint16_t totalChunks;
  uint16_t dataLen;
  uint16_t crc;
} __attribute__((packed));

// Photo reception state
struct PhotoReception {
  bool active = false;
  uint32_t photoId = 0;
  uint32_t totalSize = 0;
  uint16_t totalChunks = 0;
  uint16_t receivedChunks = 0;
  uint8_t* buffer = nullptr;
  bool* chunkReceived = nullptr;
  uint32_t startTime = 0;
  uint32_t lastChunkTime = 0;
} photoRx;

// BLE objects
NimBLEServer* pServer = nullptr;
NimBLEService* pService = nullptr;
NimBLECharacteristic* pCharPhotoData = nullptr;
NimBLECharacteristic* pCharPhotoInfo = nullptr;
NimBLECharacteristic* pCharStatus = nullptr;
bool deviceConnected = false;

SX1262 lora = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RESET, PIN_LORA_BUSY);
bool loraReady = false;

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

// BLE Server Callbacks
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    deviceConnected = true;
    Serial.println("📱 Phone connected via BLE");
  }
  
  void onDisconnect(NimBLEServer* pServer) {
    deviceConnected = false;
    Serial.println("📱 Phone disconnected");
    pServer->startAdvertising();
  }
};

// Initialize BLE
void initBLE() {
  Serial.print("Initializing BLE... ");
  
  NimBLEDevice::init(BLE_DEVICE_NAME);
  NimBLEDevice::setMTU(BLE_MTU);
  
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  pService = pServer->createService(SERVICE_UUID);
  
  // Photo Data Characteristic (for sending photo chunks to phone)
  pCharPhotoData = pService->createCharacteristic(
    CHAR_PHOTO_DATA_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  
  // Photo Info Characteristic (metadata: size, chunks, progress)
  pCharPhotoInfo = pService->createCharacteristic(
    CHAR_PHOTO_INFO_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  
  // Status Characteristic (system status, errors)
  pCharStatus = pService->createCharacteristic(
    CHAR_STATUS_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  
  pService->start();
  
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  
  Serial.println("✓ OK");
  Serial.printf("BLE Name: %s\n", BLE_DEVICE_NAME);
}

// Send ACK packet
void sendAck(uint16_t chunkIndex) {
  PacketHeader ack = {
    .type = PKT_ACK,
    .photoId = photoRx.photoId,
    .chunkIndex = chunkIndex,
    .totalChunks = 0,
    .dataLen = 0,
    .crc = 0
  };
  
  lora.transmit((uint8_t*)&ack, sizeof(ack));
}

// Send NACK packet
void sendNack(uint16_t chunkIndex) {
  PacketHeader nack = {
    .type = PKT_NACK,
    .photoId = photoRx.photoId,
    .chunkIndex = chunkIndex,
    .totalChunks = 0,
    .dataLen = 0,
    .crc = 0
  };
  
  lora.transmit((uint8_t*)&nack, sizeof(nack));
}

// Initialize photo reception
void startPhotoReception(uint32_t photoId, uint16_t totalChunks, uint32_t totalSize) {
  // Clean up previous reception
  if (photoRx.buffer) {
    delete[] photoRx.buffer;
  }
  if (photoRx.chunkReceived) {
    delete[] photoRx.chunkReceived;
  }
  
  photoRx.active = true;
  photoRx.photoId = photoId;
  photoRx.totalChunks = totalChunks;
  photoRx.totalSize = totalSize;
  photoRx.receivedChunks = 0;
  photoRx.startTime = millis();
  photoRx.lastChunkTime = millis();
  
  // Allocate buffers
  photoRx.buffer = new uint8_t[totalSize];
  photoRx.chunkReceived = new bool[totalChunks];
  memset(photoRx.chunkReceived, 0, totalChunks);
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  PHOTO RECEPTION STARTED          ║");
  Serial.println("╚═══════════════════════════════════╝");
  Serial.printf("Photo ID: %u\n", photoId);
  Serial.printf("Size: %u bytes\n", totalSize);
  Serial.printf("Chunks: %u\n", totalChunks);
  
  // Notify phone via BLE
  if (deviceConnected && pCharPhotoInfo) {
    char info[64];
    snprintf(info, sizeof(info), "START:%u:%u:%u", photoId, totalSize, totalChunks);
    pCharPhotoInfo->setValue(info);
    pCharPhotoInfo->notify();
  }
}

// Process received data chunk
void processDataChunk(const uint8_t* packet, uint16_t len) {
  if (len < sizeof(PacketHeader)) {
    return;
  }
  
  PacketHeader* hdr = (PacketHeader*)packet;
  const uint8_t* data = packet + sizeof(PacketHeader);
  uint16_t dataLen = len - sizeof(PacketHeader);
  
  if (dataLen != hdr->dataLen) {
    Serial.printf("✗ Data length mismatch: expected %d, got %d\n", hdr->dataLen, dataLen);
    sendNack(hdr->chunkIndex);
    return;
  }
  
  // Verify CRC
  uint16_t calcCrc = calculateCRC16(data, dataLen);
  if (calcCrc != hdr->crc) {
    Serial.printf("✗ CRC error for chunk %d\n", hdr->chunkIndex);
    sendNack(hdr->chunkIndex);
    return;
  }
  
  // Check if chunk already received
  if (photoRx.chunkReceived[hdr->chunkIndex]) {
    Serial.printf("⚠ Duplicate chunk %d, ignoring\n", hdr->chunkIndex);
    sendAck(hdr->chunkIndex); // Still send ACK
    return;
  }
  
  // Copy data to buffer
  uint32_t offset = hdr->chunkIndex * CHUNK_SIZE;
  memcpy(photoRx.buffer + offset, data, dataLen);
  photoRx.chunkReceived[hdr->chunkIndex] = true;
  photoRx.receivedChunks++;
  photoRx.lastChunkTime = millis();
  
  // Send ACK
  sendAck(hdr->chunkIndex);
  
  // Progress update
  uint8_t progress = (photoRx.receivedChunks * 100) / photoRx.totalChunks;
  Serial.printf("✓ Chunk %d/%d (%d%%) ", 
                photoRx.receivedChunks, photoRx.totalChunks, progress);
  
  // Progress bar
  Serial.print("[");
  for (uint8_t p = 0; p < 20; p++) {
    Serial.print(p < (progress / 5) ? "=" : " ");
  }
  Serial.println("]");
  
  // Update phone via BLE
  if (deviceConnected && pCharStatus) {
    char status[32];
    snprintf(status, sizeof(status), "PROGRESS:%d/%d", photoRx.receivedChunks, photoRx.totalChunks);
    pCharStatus->setValue(status);
    pCharStatus->notify();
  }
}

// Send photo to phone via BLE
void sendPhotoToPhone() {
  if (!deviceConnected) {
    Serial.println("⚠ No phone connected, photo saved locally");
    return;
  }
  
  Serial.println("\n→ Sending photo to phone via BLE...");
  
  // Send in chunks (BLE MTU - overhead)
  const uint16_t bleChunkSize = BLE_MTU - 3;
  uint32_t offset = 0;
  uint16_t bleChunks = (photoRx.totalSize + bleChunkSize - 1) / bleChunkSize;
  
  for (uint16_t i = 0; i < bleChunks; i++) {
    uint16_t len = min((uint32_t)bleChunkSize, photoRx.totalSize - offset);
    
    pCharPhotoData->setValue(photoRx.buffer + offset, len);
    pCharPhotoData->notify();
    
    offset += len;
    
    Serial.printf("  BLE chunk %d/%d sent\n", i + 1, bleChunks);
    delay(20); // Small delay to avoid overwhelming BLE
  }
  
  // Send completion notification
  if (pCharPhotoInfo) {
    char info[32];
    snprintf(info, sizeof(info), "COMPLETE:%u", photoRx.totalSize);
    pCharPhotoInfo->setValue(info);
    pCharPhotoInfo->notify();
  }
  
  Serial.println("✓ Photo sent to phone!");
}

// Finalize photo reception
void finalizePhotoReception() {
  uint32_t duration = millis() - photoRx.startTime;
  float speed = (photoRx.totalSize * 8.0) / (duration / 1000.0);
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  PHOTO RECEPTION COMPLETE!        ║");
  Serial.println("╚═══════════════════════════════════╝");
  Serial.printf("Duration: %u ms\n", duration);
  Serial.printf("Speed: %.2f bps (%.2f bytes/s)\n", speed, speed / 8);
  Serial.printf("Received: %d/%d chunks\n", photoRx.receivedChunks, photoRx.totalChunks);
  
  // Send to phone
  sendPhotoToPhone();
  
  // Cleanup
  photoRx.active = false;
  // Keep buffer for inspection, will be freed on next photo
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  PHOTO TUNNEL - RECEIVER STATION  ║");
  Serial.println("╚═══════════════════════════════════╝\n");
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Initialize BLE
  initBLE();
  
  // Initialize LoRa
  Serial.print("Initializing SX1262... ");
  int16_t state = lora.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, 0x12, 10);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✓ OK");
    loraReady = true;
    
    lora.setCurrentLimit(60.0);
    lora.setCRC(true);
    
    // Start receiving
    lora.startReceive();
    
    Serial.printf("Frequency: %.1f MHz\n", LORA_FREQ);
    Serial.printf("Bandwidth: %.1f kHz\n", LORA_BW);
    Serial.printf("Spreading Factor: %d\n", LORA_SF);
    Serial.printf("Listening for packets...\n");
  } else {
    Serial.printf("✗ Failed (error %d)\n", state);
    loraReady = false;
  }
  
  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("Waiting for photo transmission...");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void loop() {
  static uint32_t lastBlink = 0;
  static bool ledState = false;
  
  // LED heartbeat
  if (millis() - lastBlink >= (deviceConnected ? 200 : 500)) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    lastBlink = millis();
  }
  
  // Check for LoRa packets
  if (loraReady) {
    int16_t state = lora.receive();
    
    if (state == RADIOLIB_ERR_NONE) {
      uint8_t buffer[sizeof(PacketHeader) + CHUNK_SIZE];
      int len = lora.getPacketLength();
      
      if (len > 0 && len <= sizeof(buffer)) {
        state = lora.readData(buffer, len);
        
        if (state == RADIOLIB_ERR_NONE && len >= sizeof(PacketHeader)) {
          PacketHeader* hdr = (PacketHeader*)buffer;
          
          // Handle different packet types
          switch (hdr->type) {
            case PKT_START:
              startPhotoReception(hdr->photoId, hdr->totalChunks, hdr->dataLen);
              break;
              
            case PKT_DATA:
              if (photoRx.active && hdr->photoId == photoRx.photoId) {
                processDataChunk(buffer, len);
              }
              break;
              
            case PKT_END:
              if (photoRx.active && hdr->photoId == photoRx.photoId) {
                Serial.println("\n← END packet received");
                
                // Check if all chunks received
                if (photoRx.receivedChunks == photoRx.totalChunks) {
                  finalizePhotoReception();
                } else {
                  Serial.printf("⚠ Missing chunks: %d/%d\n", 
                                photoRx.receivedChunks, photoRx.totalChunks);
                }
              }
              break;
              
            case PKT_PING:
              Serial.println("← PING received");
              break;
          }
        }
      }
      
      // Restart receive
      lora.startReceive();
    }
  }
  
  // Timeout check for active reception
  if (photoRx.active && (millis() - photoRx.lastChunkTime > 10000)) {
    Serial.println("\n⚠ Reception timeout - no chunks for 10s");
    photoRx.active = false;
  }
}
