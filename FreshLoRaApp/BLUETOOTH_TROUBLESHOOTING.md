# 🔧 BLE SCANNING TROUBLESHOOTING GUIDE

## 🚨 COMMON CAUSES OF "FAILED TO SCAN DEVICES" ERROR

### 1. **Android Permissions Issues** (Most Common)
```bash
Required Permissions:
✅ BLUETOOTH
✅ BLUETOOTH_ADMIN  
✅ ACCESS_FINE_LOCATION
✅ ACCESS_COARSE_LOCATION
✅ BLUETOOTH_SCAN (Android 12+)
✅ BLUETOOTH_CONNECT (Android 12+)
```

**Solution:**
- Go to Android Settings > Apps > LoRa BLE Chat > Permissions
- Enable ALL Bluetooth and Location permissions
- **CRITICAL**: Enable Location services globally on Android

### 2. **Location Services Disabled**
BLE scanning on Android requires Location services to be enabled.

**Solution:**
- Go to Android Settings > Location > Turn ON
- This is required even if your app doesn't use GPS

### 3. **Android BLE Scan Limitations**
Android limits BLE scanning frequency to prevent battery drain.

**Solution:**
- Wait 30 seconds between scan attempts
- Restart the app if scanning fails repeatedly
- Clear Bluetooth cache in Android settings

### 4. **React Native BLE Library Issues**
Sometimes the react-native-ble-plx library needs reinitialization.

**Solution:**
- Force close and restart the app
- Clear app data in Android settings
- Restart Android Bluetooth service

---

## 🔧 IMMEDIATE DEBUGGING STEPS

### Step 1: Enable Debug Mode
I've created an enhanced BLE service with detailed logging. To use it:

1. **Temporarily replace BLEService import** in `DeviceSelectionScreen.tsx`:
```typescript
// Change this line:
import { BLEService } from '../services/BLEService';

// To this:
import { BLEServiceDebug as BLEService } from '../services/BLEServiceDebug';
```

2. **Rebuild the app** and check console logs
3. **Look for detailed scan information** in debug output

### Step 2: Check Android Settings
1. **Bluetooth**: Settings > Connected devices > Bluetooth > ON
2. **Location**: Settings > Location > ON
3. **App Permissions**: Settings > Apps > LoRa BLE Chat > Permissions
   - Allow "Nearby devices" or "Bluetooth"  
   - Allow "Location" (precise)

### Step 3: Test Basic BLE Functionality
1. **Open any other BLE app** (like "BLE Scanner" from Play Store)
2. **See if it can find devices**
3. **If no devices found**, it's an Android system issue

---

## 🧪 TESTING WITHOUT ESP32 HARDWARE

### Option 1: Use BLE Scanner App
1. Install "BLE Scanner" or "nRF Connect" from Play Store
2. Check if it finds ANY BLE devices
3. If yes, the issue is in our app
4. If no, it's an Android system issue

### Option 2: Create Mock Devices
I can help you create a mock BLE service that simulates ESP32 devices:

```typescript
// Add to BLEService for testing
const mockDevices: LoRaDevice[] = [
  { id: 'mock-m1', name: 'M1', rssi: -45, stationType: 'M1' },
  { id: 'mock-m2', name: 'M2', rssi: -52, stationType: 'M2' }
];
```

### Option 3: Test with Any BLE Device
Temporarily modify device detection to find ANY BLE device:

```typescript
// In scanForDevices(), replace device filtering with:
if (device.name) {
  // Accept ANY named device for testing
  devices.push({
    id: device.id,
    name: device.name,
    rssi: device.rssi,
    stationType: 'M1' // Default for testing
  });
}
```

---

## 🔍 ANALYZING DEBUG OUTPUT

When you run with BLEServiceDebug, look for these patterns:

### ✅ **Good Output:**
```
🔧 [DEBUG] BLE State changed to: PoweredOn
✅ [DEBUG] BLE is ready
🔍 [DEBUG] Starting device scan (15 seconds)...
📱 [DEBUG] Device #1: { name: "SomeDevice", rssi: -45 }
📱 [DEBUG] Device #2: { name: "M1", rssi: -52 }
🎯 [DEBUG] FOUND LORA DEVICE: M1
```

### ❌ **Bad Output (Permission Issue):**
```
🔧 [DEBUG] BLE State changed to: Unauthorized
❌ [DEBUG] Bluetooth permission denied
```

### ❌ **Bad Output (No Devices):**
```
✅ [DEBUG] BLE is ready
🔍 [DEBUG] Starting device scan (15 seconds)...
😞 [DEBUG] No devices found at all
```

### ❌ **Bad Output (Scan Error):**
```
❌ [DEBUG] Scan error: { message: "Location services disabled" }
```

---

## 🚀 QUICK FIXES TO TRY

### Fix 1: Force Bluetooth Reset
```bash
1. Turn OFF Bluetooth in Android settings
2. Wait 10 seconds  
3. Turn ON Bluetooth
4. Restart your app
5. Try scanning again
```

### Fix 2: Clear App Data
```bash
1. Go to Settings > Apps > LoRa BLE Chat
2. Tap "Storage" 
3. Tap "Clear Data" (this resets permissions)
4. Restart app and re-grant permissions
```

### Fix 3: Restart BLE Service
```bash
1. Force close the app
2. Turn Bluetooth OFF and ON
3. Restart the app
4. Initialize BLE again
```

### Fix 4: Use Different Android Device
- Some Android devices have BLE scanning issues
- Try on a different phone/tablet if available
- Samsung and Google Pixel devices usually work well

---

## 📱 ANDROID VERSION SPECIFIC ISSUES

### Android 12+ (API 31+)
- Requires new BLUETOOTH_SCAN and BLUETOOTH_CONNECT permissions
- ✅ Your app.json already includes these

### Android 10+ (API 29+)
- Requires ACCESS_FINE_LOCATION for BLE scanning
- Background location restrictions
- ✅ Your app.json includes this

### Android 6+ (API 23+)
- Runtime permissions required
- Location services must be enabled
- ✅ Handled in your app

---

## 🎯 NEXT STEPS

1. **Try the debug version** I created above
2. **Check the console logs** for detailed error information
3. **Verify Android permissions** are properly granted
4. **Test with BLE Scanner app** to confirm Android BLE works
5. **Report back** with the specific error messages from debug logs

The debug version will tell us exactly where the scanning is failing and help identify the root cause! 🔍