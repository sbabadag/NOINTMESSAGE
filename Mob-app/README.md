# LoRa BLE Chat App

A React Native mobile application that connects to LoRa devices via Bluetooth Low Energy (BLE) for long-range mesh messaging.

## 🎉 Project Status: Ready for Development!

All core components have been implemented and configured. The app is ready to run once Node.js is installed.

## Features

- 📱 **Cross-platform**: Works on both iOS and Android
- 📡 **BLE Integration**: Connect to LoRa devices via Bluetooth
- 💬 **Real-time Chat**: Send and receive messages through LoRa network
- 🔍 **Device Discovery**: Automatic scanning for LoRa_Station_1 and LoRa_Station_2
- 📊 **Signal Info**: Display RSSI and SNR values for received messages
- 🎨 **Modern UI**: Clean, intuitive chat interface

## Hardware Requirements

- **LoRa Devices**: XIAO ESP32S3 + SX1262 LoRa modules
- **BLE Services**: 
  - Service UUID: `12345678-1234-5678-9abc-def012345678`
  - TX Characteristic: `12345678-1234-5678-9abc-def012345679`
  - RX Characteristic: `12345678-1234-5678-9abc-def012345678`

## Prerequisites

Before running this app, you need:

1. **Node.js** (version 16 or higher)
2. **React Native CLI** or **Expo CLI**
3. **Android Studio** (for Android development)
4. **Xcode** (for iOS development, macOS only)

## Installation

1. **Install Node.js**:
   ```bash
   # Download from https://nodejs.org/
   # Or use a package manager like Chocolatey on Windows:
   choco install nodejs
   ```

2. **Install React Native CLI**:
   ```bash
   npm install -g react-native-cli
   ```

3. **Install dependencies**:
   ```bash
   npm install
   ```

4. **Android Setup**:
   - Install Android Studio
   - Set up Android SDK and emulator
   - Enable USB debugging on your device

5. **iOS Setup** (macOS only):
   - Install Xcode from App Store
   - Install iOS Simulator

## Running the App

### Development Mode

1. **Start Metro bundler**:
   ```bash
   npm start
   ```

2. **Run on Android**:
   ```bash
   npm run android
   ```

3. **Run on iOS**:
   ```bash
   npm run ios
   ```

### Building for Production

1. **Android APK**:
   ```bash
   cd android
   ./gradlew assembleRelease
   ```

2. **iOS Archive** (macOS only):
   ```bash
   npx react-native run-ios --configuration Release
   ```

## Usage

1. **Start the App**: Launch LoRa BLE Chat on your mobile device
2. **Enable Bluetooth**: Ensure Bluetooth is enabled on your phone
3. **Scan for Devices**: Tap "Start Messaging" to scan for LoRa devices
4. **Connect**: Select your LoRa device (LoRa_Station_1 or LoRa_Station_2)
5. **Chat**: Send messages through the LoRa network!

## Permissions

The app requires the following permissions:

### Android
- `BLUETOOTH`
- `BLUETOOTH_ADMIN` 
- `ACCESS_FINE_LOCATION`
- `BLUETOOTH_SCAN`
- `BLUETOOTH_CONNECT`

### iOS
- `NSBluetoothAlwaysUsageDescription`
- `NSBluetoothPeripheralUsageDescription`

## Troubleshooting

### Common Issues

1. **"No devices found"**:
   - Ensure LoRa devices are powered on
   - Check Bluetooth is enabled
   - Verify devices are in range

2. **Connection failed**:
   - Restart the LoRa device
   - Try scanning again
   - Check device compatibility

3. **Messages not sending**:
   - Verify BLE connection is active
   - Check LoRa device serial output
   - Ensure proper BLE characteristics

### Debug Mode

To see detailed logs:
```bash
npx react-native log-android  # Android
npx react-native log-ios      # iOS
```

## Development

### Project Structure
```
src/
├── screens/           # React Native screens
│   ├── HomeScreen.tsx
│   ├── DeviceSelectionScreen.tsx
│   └── ChatScreen.tsx
├── services/          # BLE service logic
│   └── BLEService.ts
└── types/            # TypeScript type definitions
    └── index.ts
```

### Dependencies

- **react-native-ble-plx**: BLE communication
- **@react-navigation**: Screen navigation
- **react-native-permissions**: Runtime permissions
- **react-native-vector-icons**: UI icons

## Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the MIT License.

## Support

For support with LoRa hardware integration, refer to the main LoRa project documentation in the parent directory.