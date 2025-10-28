# 📸 Photo Tunnel System - Complete Guide

## System Overview

```
┌─────────────────┐                      ┌─────────────────┐                    ┌──────────────┐
│  SENDER STATION │                      │ RECEIVER STATION│                    │ MOBILE PHONE │
│                 │                      │                 │                    │              │
│  XIAO ESP32-S3  │    LoRa 915MHz      │  XIAO ESP32-S3  │    Bluetooth LE   │   Android/   │
│  + SX1262       │ ══════════════════> │  + SX1262       │ ─────────────────> │     iOS      │
│                 │   Photo Chunks       │  + BLE Server   │    Photo Data     │              │
│  [Photo Input]  │                      │  [Reassembly]   │                    │  [Display]   │
└─────────────────┘                      └─────────────────┘                    └──────────────┘
  Camera/Serial/SD                        Buffer + Forward                      Save/View Photo
```

## 🎯 Features

- ✅ **Long-Range Photo Transfer**: Send photos up to several kilometers without internet
- ✅ **Reliable Transmission**: Chunked transfer with CRC verification and ACK/NACK protocol
- ✅ **Progress Tracking**: Real-time progress updates on both serial and BLE
- ✅ **Automatic Recovery**: Retry mechanism for failed chunks
- ✅ **Mobile Integration**: Ready-to-use BLE protocol for Android/iOS apps
- ✅ **Scalable**: Supports photos up to 100KB (adjustable)

## 📋 Hardware Requirements

### Each Station Needs:
- **Seeed XIAO ESP32-S3** microcontroller
- **Wio SX1262 LoRa Module** (915 MHz or 868 MHz)
- Manual pin connections (as tested):
  ```
  XIAO ESP32-S3    →    Wio SX1262
  ─────────────────────────────────
  D7 (GPIO44)      →    NSS (CS)
  D1 (GPIO2)       →    DIO1
  D0 (GPIO1)       →    RESET
  D2 (GPIO3)       →    BUSY
  D10 (GPIO9)      →    SCK
  D9 (GPIO8)       →    MISO
  D8 (GPIO7)       →    MOSI
  3.3V             →    VCC
  GND              →    GND
  ```

### Additional for Sender (Optional):
- Camera module (OV2640, ESP32-CAM, etc.)
- microSD card module
- USB connection for serial photo upload

## 🚀 Quick Start

### 1. Flash Sender Station

```bash
# Copy photo_tunnel_sender.cpp to src/main.cpp
cp photo_tunnel_sender.cpp src/main.cpp

# Build and upload
pio run --target upload

# Open serial monitor
pio device monitor --baud 115200
```

**Commands:**
- Press `s` - Send demo photo (2KB test image)
- Press `p` - Send ping packet

### 2. Flash Receiver Station

```bash
# Copy photo_tunnel_receiver.cpp to src/main.cpp  
cp photo_tunnel_receiver.cpp src/main.cpp

# Build and upload
pio run --target upload

# Open serial monitor
pio device monitor --baud 115200
```

The receiver automatically:
- Starts LoRa listening
- Starts BLE advertising as "PhotoTunnel"
- Waits for photo transmission

### 3. Connect Mobile App

See [MOBILE_APP_GUIDE.md](MOBILE_APP_GUIDE.md) for complete mobile app integration.

**Quick BLE Connection:**
1. Scan for device named "PhotoTunnel"
2. Connect to service UUID: `12345678-1234-1234-1234-123456789abc`
3. Subscribe to all characteristics with NOTIFY property
4. Wait for photo data

## 📡 LoRa Protocol

### Packet Types

| Type | Value | Description |
|------|-------|-------------|
| PKT_START | 0x01 | Photo transmission start |
| PKT_DATA | 0x02 | Photo data chunk |
| PKT_END | 0x03 | Photo transmission end |
| PKT_ACK | 0x04 | Acknowledgment |
| PKT_NACK | 0x05 | Negative ACK (retransmit) |
| PKT_PING | 0x06 | Keep-alive test |

### Packet Header Structure

```cpp
struct PacketHeader {
  uint8_t  type;         // Packet type
  uint32_t photoId;      // Unique photo identifier
  uint16_t chunkIndex;   // Current chunk number (0-based)
  uint16_t totalChunks;  // Total chunks in photo
  uint16_t dataLen;      // Bytes in this packet
  uint16_t crc;          // CRC16 of data
} __attribute__((packed)); // 13 bytes
```

### Transmission Flow

```
Sender                           Receiver
  │                                 │
  ├──── PKT_START ──────────────────>│
  │                                 │ (Prepare buffer)
  │                                 │
  ├──── PKT_DATA (chunk 0) ────────>│
  │<──────── PKT_ACK ────────────────┤
  │                                 │
  ├──── PKT_DATA (chunk 1) ────────>│
  │<──────── PKT_ACK ────────────────┤
  │                                 │
  ├──── PKT_DATA (chunk 2) ────────>│
  │<──────── PKT_NACK (CRC error) ───┤
  │                                 │
  ├──── PKT_DATA (chunk 2 retry) ──>│
  │<──────── PKT_ACK ────────────────┤
  │                                 │
  │         ... more chunks ...      │
  │                                 │
  ├──── PKT_END ────────────────────>│
  │                                 │ (Verify & send to phone)
```

## 📱 BLE Protocol

### Service & Characteristics

**Service UUID**: `12345678-1234-1234-1234-123456789abc`

| Characteristic | UUID | Properties | Purpose |
|----------------|------|------------|---------|
| Photo Data | `...789abd` | READ, NOTIFY | Photo bytes (chunks) |
| Photo Info | `...789abe` | READ, NOTIFY | Metadata & commands |
| Status | `...789abf` | READ, NOTIFY | Progress updates |

### Photo Info Messages

```
START:123456:51200:256
│     │      │     │
│     │      │     └─ Total chunks
│     │      └─────── Photo size (bytes)
│     └──────────────── Photo ID
└────────────────────── Command

COMPLETE:51200
│        │
│        └──── Final size
└───────────── Command
```

### Status Messages

```
PROGRESS:128/256
│        │   │
│        │   └─ Total chunks
│        └───── Received chunks
└────────────── Command
```

## ⚙️ Configuration

### LoRa Parameters (edit in both files)

```cpp
constexpr float LORA_FREQ = 915.0;      // 915 MHz (US) or 868 MHz (EU)
constexpr float LORA_BW = 125.0;        // Bandwidth in kHz
constexpr uint8_t LORA_SF = 7;          // Spreading Factor (7-12)
constexpr uint8_t LORA_CR = 5;          // Coding Rate (5-8)
constexpr int8_t LORA_POWER = 22;       // TX Power (2-22 dBm)
```

### Photo Transfer Settings

```cpp
constexpr uint16_t CHUNK_SIZE = 200;    // Bytes per LoRa packet
constexpr uint8_t MAX_RETRIES = 3;      // Retry attempts per chunk
constexpr uint32_t ACK_TIMEOUT = 2000;  // ACK wait time (ms)
constexpr uint32_t MAX_PHOTO_SIZE = 100000; // Max photo size
```

### Performance Tuning

**For Longer Range:**
```cpp
LORA_SF = 12;     // Max range, slower speed
LORA_BW = 62.5;   // Narrower bandwidth
LORA_CR = 8;      // More error correction
```

**For Faster Transfer:**
```cpp
LORA_SF = 7;      // Shorter range, faster speed  
LORA_BW = 250.0;  // Wider bandwidth
CHUNK_SIZE = 240; // Larger chunks
```

## 📊 Expected Performance

### Transfer Speeds (SF7, BW125, CR5)

| Photo Size | Chunks | Time | Speed |
|------------|--------|------|-------|
| 10 KB | 50 | ~15 sec | 5.3 kbps |
| 50 KB | 250 | ~75 sec | 5.3 kbps |
| 100 KB | 500 | ~150 sec | 5.3 kbps |

**Note**: Times include ACK overhead. Actual speed varies with signal quality.

### Range Estimates

| Environment | SF7 | SF10 | SF12 |
|-------------|-----|------|------|
| Urban | 1-2 km | 3-5 km | 5-10 km |
| Suburban | 2-4 km | 5-8 km | 10-15 km |
| Rural/Open | 5-10 km | 10-20 km | 20-30 km |

## 🔧 Troubleshooting

### Sender Issues

**"SX1262 initialization failed"**
- Check wiring connections
- Verify power supply (3.3V)
- Confirm pin mappings match your hardware

**"ACK timeout"**
- Check receiver is powered on and listening
- Verify both stations use same LoRa frequency
- Reduce distance between stations
- Check antenna connections

### Receiver Issues

**"CRC error"**
- Signal quality poor - move stations closer
- Interference - change frequency
- Increase spreading factor for better reliability

**"Missing chunks"**
- Normal for first transmission
- Sender will retry automatically
- Check serial monitor for retry messages

### BLE Connection Issues

**Phone can't find "PhotoTunnel"**
- Ensure receiver station is powered
- Check LED blinks faster when BLE advertising
- Try restarting Bluetooth on phone

**Photo incomplete on phone**
- Check all BLE notifications are subscribed
- Verify app handles all data chunks
- Check phone storage space

## 🎨 Example Use Cases

### 1. Remote Security Camera
```
Camera in barn → Sender Station → Receiver at house → Phone app
```

### 2. Wildlife Photography
```
Camera trap → Sender → Base station → Researcher's phone
```

### 3. Event Photography
```
Photographer camera → Sender → Event display → Attendee phones
```

### 4. Disaster Communication
```
Drone camera → Sender → Ground station → Relief worker phones
```

## 📝 Adding Real Camera Support

### Option 1: ESP32-CAM Integration

```cpp
#include "esp_camera.h"

// In setup()
camera_config_t config;
config.pin_d0 = Y2_GPIO_NUM;
// ... configure camera ...
esp_camera_init(&config);

// Capture photo
camera_fb_t* fb = esp_camera_fb_get();
if (fb) {
  sendPhoto(fb->buf, fb->len);
  esp_camera_fb_return(fb);
}
```

### Option 2: Serial Upload

Upload photos via USB serial:
```python
# upload_photo.py
import serial
import sys

ser = serial.Serial('/dev/ttyUSB0', 115200)
with open(sys.argv[1], 'rb') as f:
    photo_data = f.read()
    ser.write(b'U')  # Upload command
    ser.write(len(photo_data).to_bytes(4, 'little'))
    ser.write(photo_data)
```

### Option 3: SD Card

```cpp
#include <SD.h>

File photoFile = SD.open("/photo.jpg");
if (photoFile) {
  uint32_t size = photoFile.size();
  uint8_t* buffer = new uint8_t[size];
  photoFile.read(buffer, size);
  sendPhoto(buffer, size);
  delete[] buffer;
}
```

## 📚 Further Documentation

- [Mobile App Development Guide](MOBILE_APP_GUIDE.md) - Complete mobile app examples
- [Protocol Specification](PROTOCOL.md) - Detailed packet formats
- [Hardware Setup Guide](HARDWARE.md) - Wiring diagrams and photos

## 🤝 Contributing

Have improvements? Ideas for features?
1. Test thoroughly with your hardware
2. Document changes clearly
3. Share your results!

## 📄 License

MIT License - Use freely, attribution appreciated!

---

**Ready to test?** Flash both stations and press `s` on the sender! 📸✨
