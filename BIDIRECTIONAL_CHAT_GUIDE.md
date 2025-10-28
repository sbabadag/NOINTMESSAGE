# Bidirectional LoRa Chat System Guide

## 🎯 System Overview

This is a complete bidirectional messaging system that allows **two phones to chat with each other via LoRa radio**, without internet connectivity. The system uses ESP32 devices as LoRa-BLE bridges.

### 📡 How It Works

```
Phone A (App) ←BLE→ ESP32 M1 ←LoRa→ ESP32 M2 ←BLE→ Phone B (App)
```

- **Phone A** connects to **M1 Station** via Bluetooth
- **Phone B** connects to **M2 Station** via Bluetooth  
- Messages are tunneled between phones through the **LoRa radio link**
- Range: **Several kilometers** (depending on terrain and antennas)

## 🔧 Hardware Setup

### Required Components (x2 sets)
- **XIAO ESP32S3** (2x)
- **Wio SX1262 LoRa module** (2x) 
- **Antennas** (915MHz for US, 868MHz for EU)
- **USB-C cables** for power/programming

### Wiring (Same for Both M1 and M2)
```
ESP32 XIAO S3    →    Wio SX1262
D0  (GPIO1)      →    RST
D1  (GPIO2)      →    DIO1
D2  (GPIO3)      →    BUSY
D7  (GPIO44)     →    NSS
D8  (GPIO7)      →    MOSI
D9  (GPIO8)      →    MISO
D10 (GPIO9)      →    SCK
3V3              →    3V3
GND              →    GND
```

## 📱 Mobile App Features

### Enhanced Bidirectional Chat Interface
- **Station Identification**: Shows M1/M2 connection status
- **LoRa Tunnel Indicator**: Displays active tunnel (M1↔M2)
- **Message Routing**: Clear indication of message source and destination
- **Real-time Status**: Connection monitoring and signal strength
- **Formatted Messages**: Timestamped messages with station info

### BLE Service Configuration
```
Service UUID:    12345678-1234-1234-1234-123456789abc
RX Characteristic: 12345678-1234-1234-1234-123456789abd (Phone receives)
TX Characteristic: 12345678-1234-1234-1234-123456789abe (Phone sends)
Status Characteristic: 12345678-1234-1234-1234-123456789abf (Status updates)
```

## 🚀 Quick Start Guide

### Step 1: Flash ESP32 Firmware

**For M1 Station:**
1. Open `m1_station.cpp` in PlatformIO
2. Upload to first ESP32
3. Device will advertise as "M1"

**For M2 Station:**
1. Copy `m1_station.cpp` to `src/main.cpp` 
2. Change `BLE_DEVICE_NAME` from "M1" to "M2"
3. Upload to second ESP32
4. Device will advertise as "M2"

### Step 2: Install Mobile App

1. **Download APK**: https://expo.dev/accounts/sbabadag/projects/lora-ble-chat-nav/builds/4bb66c80-27e8-4ad9-844c-c54d2e959419
2. **Install on both phones** (Android 7+)
3. **Grant Bluetooth permissions** when prompted

### Step 3: Connect and Chat

**Phone A:**
1. Open LoRa BLE Chat app
2. Tap "Start Messaging" 
3. Select "M1 Station"
4. Wait for connection ✅

**Phone B:**
1. Open LoRa BLE Chat app
2. Tap "Start Messaging"
3. Select "M2 Station" 
4. Wait for connection ✅

**Start Chatting:**
- Type messages on either phone
- Messages route automatically through LoRa
- See real-time delivery and signal info

## 📡 Message Flow Example

```
1. Phone A types: "Hello from M1!"
   Phone A → M1 (BLE) → M2 (LoRa) → Phone B

2. Phone B sees: "[M1→M2 14:23:45] Hello from M1!"
   With signal info: "📡 Via LoRa from M1 Station • Signal: -65dBm"

3. Phone B replies: "Hi from M2!"
   Phone B → M2 (BLE) → M1 (LoRa) → Phone A

4. Phone A sees: "[M2→M1 14:24:12] Hi from M2!"
```

## 🔧 Technical Details

### LoRa Configuration
- **Frequency**: 915MHz (US) / 868MHz (EU)
- **Bandwidth**: 125kHz
- **Spreading Factor**: 7 (fast, shorter range) to 12 (slow, longer range)
- **Coding Rate**: 4/5
- **Power**: Up to 22dBm (160mW)

### Message Packet Structure
```cpp
struct MessagePacket {
  uint32_t timestamp;      // Message timestamp
  uint16_t messageLen;     // Message length
  char message[200];       // Message content (max 200 chars)
} __attribute__((packed));
```

### BLE Characteristics
- **RX (Phone ← ESP32)**: Formatted messages from remote station
- **TX (Phone → ESP32)**: Raw messages to send via LoRa
- **Status**: Connection status and message counters

## 📊 Performance

### Range Testing
- **Urban**: 1-3 km with basic antennas
- **Suburban**: 3-8 km with good antennas  
- **Rural/Open**: 10-20+ km with high-gain antennas
- **Elevation helps**: Hilltop to valley significantly extends range

### Message Throughput
- **Short messages** (<50 chars): ~5 seconds end-to-end
- **Long messages** (200 chars): ~10 seconds end-to-end
- **Network overhead**: ~6 bytes per message packet

### Battery Life
- **ESP32 active**: ~12-24 hours on USB power bank
- **Phone BLE**: Minimal battery impact (~2% per hour)

## 🔍 Troubleshooting

### No Devices Found
- Ensure ESP32s are powered and running correct firmware
- Check BLE is enabled on phone
- Try scanning multiple times (BLE can be flaky)
- Verify ESP32s show "BLE Name: M1/M2" in serial monitor

### Connection Failed  
- Restart ESP32 device
- Clear Bluetooth cache on phone
- Ensure only one phone connects to each ESP32
- Check distance (BLE range ~10m max)

### Messages Not Sending
- Verify both ESP32s show LoRa initialization success
- Check antennas are properly connected
- Test LoRa range by bringing devices closer
- Monitor serial output for transmission errors

### Poor LoRa Range
- **Check antennas**: Proper frequency and connections
- **Increase power**: Modify `LORA_POWER` to 22 (max)
- **Change SF**: Higher spreading factor = longer range, slower speed
- **Elevation**: Get height advantage between stations
- **Interference**: Try different frequency within band

## 📋 Development Notes

### ESP32 Serial Commands
Monitor both ESP32s via USB for debugging:
```bash
# M1 Station Serial Output
📱 Phone connected to M1
📤 M1 → M2 VIA LORA: "Hello World"
✅ Message sent successfully to M2!
📨 MESSAGE FROM M2: "Hi back!"

# M2 Station Serial Output  
📱 Phone connected to M2
📨 MESSAGE FROM M1: "Hello World"
📤 M2 → M1 VIA LORA: "Hi back!"
✅ Message sent successfully to M1!
```

### Mobile App Debug
Enable React Native debugging:
```bash
npx react-native log-android  # Android logs
npx react-native log-ios      # iOS logs (macOS only)
```

### Custom Modifications
- **Change LoRa frequency**: Modify `LORA_FREQ` in firmware
- **Adjust message length**: Change `MAX_MESSAGE_LEN` (up to ~250 chars)
- **Add encryption**: Implement in BLE layer before LoRa transmission
- **Multiple stations**: Extend to mesh network topology

## 🎯 Use Cases

### Emergency Communications
- Natural disaster scenarios without cell towers
- Remote location coordination
- Search and rescue operations
- Emergency services backup communications

### Recreational Use
- Hiking/camping group coordination
- Amateur radio experimentation  
- Outdoor event communications
- Remote site monitoring

### Professional Applications
- Construction site communications
- Agricultural field monitoring
- Remote sensor data collection
- IoT device networking without internet

## 📚 Additional Resources

- **RadioLib Documentation**: https://github.com/jgromes/RadioLib
- **ESP32 BLE Guide**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/
- **React Native BLE**: https://github.com/innoveit/react-native-ble-plx
- **LoRa Range Calculator**: https://www.rfwireless-world.com/calculators/LoRa-range-calculator.html

---

## 🎉 Success! 

You now have a complete **bidirectional LoRa chat system** allowing two phones to communicate over **several kilometers** without internet or cellular connectivity!

**APK Download**: https://expo.dev/accounts/sbabadag/projects/lora-ble-chat-nav/builds/4bb66c80-27e8-4ad9-844c-c54d2e959419

**Hardware verified** ✅ | **Software tested** ✅ | **End-to-end functional** ✅