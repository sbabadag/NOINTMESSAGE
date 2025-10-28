# LoRa-Bluetooth Communication Tunnel

This project creates a communication tunnel between two mobile phones using LoRa technology. Each phone connects via Bluetooth to a Wio SX1262 with XIAO ESP32S3 device, which then communicates with the other device over long-range LoRa radio.

## Hardware Required

- 2x Wio SX1262 with XIAO ESP32S3 development boards
- 2x Mobile phones with Bluetooth capability
- LoRa antennas (included with boards)
- USB-C cables for programming and power

## Features

- **Long-range communication**: Up to 2-5km range depending on environment
- **Bluetooth connectivity**: Each device acts as a BLE server for phone connection
- **Bidirectional messaging**: Messages flow both ways between phones
- **Message integrity**: Checksum verification and message IDs
- **Status indicators**: LED feedback for connection status
- **Error handling**: Automatic reconnection and error recovery
- **Test functionality**: Built-in test messages via user button

## Setup Instructions

### 1. Hardware Configuration

1. Connect LoRa antennas to both devices
2. Ensure devices are powered (USB or battery)
3. Note the device IDs (configured in firmware)

### 2. Firmware Configuration

For **Device A** (first device):
```cpp
#define MY_DEVICE_ID    DEVICE_ID_A  // Keep as DEVICE_ID_A
```

For **Device B** (second device):
```cpp
#define MY_DEVICE_ID    DEVICE_ID_B  // Change to DEVICE_ID_B
```

### 3. Regional LoRa Settings

Update the frequency for your region:
```cpp
// For US/Canada/South America
#define LORA_FREQUENCY    915.0   

// For Europe/Asia/Africa  
#define LORA_FREQUENCY    868.0   
```

### 4. Programming the Devices

1. Connect Device A to computer via USB-C
2. Set `MY_DEVICE_ID` to `DEVICE_ID_A`
3. Upload firmware using PlatformIO
4. Connect Device B to computer
5. Set `MY_DEVICE_ID` to `DEVICE_ID_B`  
6. Upload firmware using PlatformIO

## Usage

### Connecting Mobile Phones

1. Power on both devices
2. On each phone, scan for Bluetooth devices
3. Look for "LoRa_Tunnel" in available devices
4. Connect to the device
5. LED will light up when connected

### Sending Messages

Once connected, any data sent from a phone's Bluetooth connection will be:
1. Received by the local device via Bluetooth
2. Transmitted over LoRa to the remote device
3. Forwarded via Bluetooth to the remote phone

### Mobile App Recommendations

For testing, you can use these Bluetooth apps:
- **Android**: "Bluetooth Terminal" or "Serial Bluetooth Terminal"
- **iOS**: "LightBlue Explorer" or "Bluetooth Terminal"

Set up the app to connect to the "LoRa_Tunnel" service and use these UUIDs:
- Service: `12345678-1234-1234-1234-123456789abc`
- TX (receive): `87654321-4321-4321-4321-cba987654321`
- RX (send): `11111111-2222-3333-4444-555555555555`

## LED Status Indicators

- **OFF**: No Bluetooth connection
- **ON**: Phone connected via Bluetooth
- **2 blinks**: LoRa message transmitted successfully
- **3 blinks**: LoRa message received
- **5 fast blinks**: System startup
- **5 slow blinks**: LoRa initialization error

## Testing

### Button Test
Press the user button on either device to send a test message to the other device.

### Serial Monitor
Connect via USB and open serial monitor (115200 baud) to see detailed communication logs.

### Range Testing
Start with devices close together, then gradually increase distance to test range.

## Troubleshooting

### No Bluetooth Connection
- Check if device is powered on
- Restart Bluetooth on phone
- Check serial monitor for error messages

### No LoRa Communication
- Verify antenna connections
- Check frequency settings match your region
- Ensure devices are using different device IDs
- Check serial monitor for LoRa initialization errors

### Poor Range
- Use external antennas if available
- Ensure antennas have clear line of sight
- Avoid interference from WiFi and other 2.4GHz devices
- Consider increasing TX power (max 20dBm, check local regulations)

## Technical Details

### LoRa Configuration
- Frequency: 915MHz (US) / 868MHz (EU)
- Bandwidth: 125kHz
- Spreading Factor: 12 (maximum range)
- Coding Rate: 4/8
- TX Power: 14dBm
- Sync Word: Private network

### Message Structure
```cpp
struct LoRaMessage {
  uint8_t sourceId;      // Source device ID
  uint8_t targetId;      // Target device ID  
  uint16_t messageId;    // Message sequence number
  uint16_t payloadLength; // Data length
  uint8_t payload[200];  // Actual message data
  uint32_t timestamp;    // Send timestamp
  uint8_t checksum;      // Data integrity check
};
```

### Power Consumption
- Active (TX): ~120mA
- Active (RX): ~15mA  
- Idle (BLE only): ~10mA
- Deep sleep: ~2µA (not implemented in this version)

## Legal Considerations

- Ensure LoRa frequency complies with local regulations
- Respect maximum power limits in your region
- This is for experimental/educational use
- For commercial use, proper certifications may be required

## Future Enhancements

- Message acknowledgment system
- Automatic frequency switching  
- Mesh networking support
- Encryption for secure communication
- Battery level monitoring
- Deep sleep mode for battery operation
- Web interface for configuration

## Support

Check the serial monitor output for detailed debugging information. Most issues can be diagnosed by observing the startup sequence and message flow in the serial output.