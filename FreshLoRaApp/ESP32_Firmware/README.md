# ESP32 LoRa BLE Bridge - Required Libraries

## Arduino IDE Library Installation

Install these libraries through Arduino IDE Library Manager:

### 1. BLE Libraries (Built-in with ESP32)
- **ESP32 BLE Arduino** - Built-in with ESP32 board package
- No additional installation needed

### 2. JSON Processing
```
Name: ArduinoJson
Author: Benoit Blanchon
Version: 6.21.3 or newer
```

### 3. LoRa Communication
```
Name: RadioLib
Author: Jan Gromes
Version: 6.0.0 or newer
```

## PlatformIO Configuration

Create `platformio.ini` file:

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps = 
    bblanchon/ArduinoJson@^6.21.3
    jgromes/RadioLib@^6.0.0

build_flags = 
    -D CORE_DEBUG_LEVEL=3
    -D CONFIG_ARDUHAL_LOG_COLORS
```

## Hardware Connections

### SX1262 LoRa Module → ESP32
```
SX1262 Pin  →  ESP32 Pin
VCC         →  3.3V
GND         →  GND
SCK         →  GPIO 5
MISO        →  GPIO 19
MOSI        →  GPIO 27
CS          →  GPIO 18
RST         →  GPIO 14
DIO1        →  GPIO 26
DIO2        →  GPIO 35 (optional)
DIO3        →  GPIO 34 (optional)
```

### Power Supply
- **ESP32**: 3.3V or 5V via USB/external
- **SX1262**: 3.3V (shared with ESP32)
- **Current Draw**: ~200mA during transmission

## Compilation Instructions

### Arduino IDE
1. Install ESP32 board package
2. Install required libraries
3. Select Board: "ESP32 Dev Module"
4. Set STATION_TYPE in code: `#define STATION_TYPE "M1"` or `"M2"`
5. Upload to each ESP32

### PlatformIO
1. Create new project
2. Copy `platformio.ini` configuration
3. Copy firmware code to `src/main.cpp`
4. Build and upload: `pio run --target upload`

## Configuration for Each Station

### M1 Station
```cpp
#define STATION_TYPE "M1"
#define DEVICE_NAME STATION_TYPE
```

### M2 Station  
```cpp
#define STATION_TYPE "M2"
#define DEVICE_NAME STATION_TYPE
```

## Testing Setup

1. **Flash M1 firmware** to first ESP32
2. **Flash M2 firmware** to second ESP32
3. **Power both devices**
4. **Run React Native app** on two phones
5. **Connect Phone A** to M1 station
6. **Connect Phone B** to M2 station
7. **Send test messages** between phones

## Troubleshooting

### BLE Issues
- Check if ESP32 appears in phone's Bluetooth scan
- Verify characteristic UUIDs match app configuration
- Ensure proper BLE permissions in Android

### LoRa Issues
- Verify wiring connections
- Check antenna connections
- Ensure both stations use same frequency/settings
- Monitor serial output for transmission status

### Message Flow Issues
- Check JSON parsing in serial monitor
- Verify station type configuration
- Test LoRa range (start close, then increase distance)

## Range Testing

### Expected Ranges
- **Urban**: 1-3 km
- **Suburban**: 3-8 km  
- **Rural**: 8-15 km
- **Line of sight**: 15-30 km

### Optimization Tips
- Use external antenna for better range
- Position antennas high and clear
- Adjust TX power (max 20 dBm in EU)
- Consider weather conditions