# LoRa-Bluetooth Tunnel - Quick Setup Guide

## ✅ Project Successfully Compiled!

Your LoRa communication tunnel firmware has been successfully built and is ready to upload to your devices.

## 🚀 Quick Start Steps

### 1. **Program Device A (First Device)**
1. Connect your first Wio SX1262 device via USB-C
2. The firmware is already configured as Device A (`MY_DEVICE_ID = DEVICE_ID_A`)
3. Upload to device:
   ```
   Upload button in VS Code or run: platformio run --target upload
   ```

### 2. **Program Device B (Second Device)**  
1. **IMPORTANT**: Before programming the second device, change line 33 in `src/main.cpp`:
   ```cpp
   // Change this line:
   #define MY_DEVICE_ID    DEVICE_ID_A
   
   // To this:
   #define MY_DEVICE_ID    DEVICE_ID_B
   ```
2. Connect your second device via USB-C
3. Upload the modified firmware to the second device

### 3. **Test the System**
1. Power on both devices (they can run from USB power)
2. Open serial monitor (115200 baud) to see status messages
3. Both devices should show successful LoRa initialization
4. Look for messages like:
   ```
   🔧 Initializing LoRa... ✅ Success!
   🔧 Initializing Bluetooth... ✅ Success!
   ✅ System ready! Connect your phone via Bluetooth.
   ```

### 4. **Connect Your Mobile Phones**

#### Download Bluetooth Terminal App:
- **Android**: "Serial Bluetooth Terminal" by Kai Morich
- **iOS**: "LightBlue Explorer" by Punch Through

#### Connection Steps:
1. On each phone, scan for Bluetooth devices
2. Look for "LoRa_Tunnel" in the device list
3. Connect to it
4. In the app, find the service: `12345678-1234-1234-1234-123456789abc`
5. Use characteristic `11111111-2222-3333-4444-555555555555` to send messages
6. Subscribe to `87654321-4321-4321-4321-cba987654321` to receive messages

### 5. **Send Your First Message**
1. Type a message in the Bluetooth terminal on Phone A
2. The message should appear on Phone B's terminal
3. Try sending a reply back!

## 📱 **Expected Behavior**

```
Phone A → Device A → [LoRa Radio] → Device B → Phone B
       ←           ← [LoRa Radio] ←          ←
```

- **LED OFF**: No phone connected
- **LED ON**: Phone connected and ready  
- **2 Quick Blinks**: Message sent successfully
- **3 Quick Blinks**: Message received from remote device

## 🔧 **Testing Features**

1. **Button Test**: Press the user button on either device to send a test message
2. **Serial Monitor**: Connect via USB to see detailed communication logs
3. **Range Test**: Start close together, then gradually increase distance

## ⚙️ **Configuration Options**

### Regional Frequency (Line 36 in main.cpp):
```cpp
// For US/Canada (915 MHz) - Default
#define LORA_FREQUENCY    915.0

// For Europe/Asia (868 MHz) - Uncomment if needed
// #define LORA_FREQUENCY    868.0  
```

### Communication Range:
- **Close range**: 100-500m in urban areas
- **Line of sight**: 2-5km in open areas  
- **Best performance**: Elevated positions, clear line of sight

## 🆘 **Troubleshooting**

### Device Not Working:
1. Check serial monitor for error messages
2. Ensure antennas are properly connected
3. Verify power supply (USB or battery)

### No Bluetooth Connection:
1. Restart phone's Bluetooth
2. Check if device LED turns on when phone connects
3. Look for "LoRa_Tunnel" in Bluetooth scan results

### No LoRa Communication:
1. Verify both devices have different IDs (A and B)
2. Check frequency matches your region (915MHz US, 868MHz EU)
3. Ensure antennas are connected and have clear line of sight
4. Check serial monitor for LoRa initialization success

### Poor Range:
1. Ensure antennas have clear line of sight
2. Avoid interference from WiFi routers
3. Try elevated positions for both devices
4. Consider external antennas for better performance

## 📊 **System Specifications**

- **Range**: 2-5km line of sight
- **Data Rate**: Up to 5.47 kbps
- **Message Size**: Up to 200 bytes per message
- **Power Consumption**: ~15mA receive, ~120mA transmit
- **Frequency**: 915MHz (US) / 868MHz (EU)
- **Bluetooth Range**: ~100m

## 🎯 **Usage Ideas**

- **Emergency Communication**: Off-grid messaging in remote areas
- **Event Coordination**: Music festivals, camping, outdoor events  
- **Hiking/Climbing**: Stay connected beyond cellular coverage
- **Marine Communication**: Boat-to-shore messaging
- **Agricultural Monitoring**: Farm equipment communication
- **Security Systems**: Long-range alert systems

## 📝 **Next Steps**

1. Test basic communication between devices
2. Experiment with range in different environments
3. Try different message types (text, JSON, binary data)
4. Consider developing custom mobile apps for better user experience
5. Explore mesh networking by adding more devices

## 💡 **Tips for Best Performance**

- Keep antennas vertical for optimal radiation pattern
- Avoid metal objects near antennas
- Higher elevation = better range
- Test during different times of day (atmospheric conditions affect propagation)
- Use external antennas for maximum range

---

**Your LoRa-Bluetooth tunnel is ready to provide internet-free communication between mobile phones!** 

Start with the devices close together to verify everything works, then gradually test longer distances. Enjoy your off-grid communication system! 📡📱