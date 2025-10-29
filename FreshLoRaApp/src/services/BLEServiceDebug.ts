import {BleManager, Device, Characteristic} from 'react-native-ble-plx';
import {LORA_BLE_CONFIG, LoRaDevice, ChatMessage} from '../types';

export class BLEServiceDebug {
  private manager: BleManager;
  private connectedDevice: Device | null = null;
  private messageCallback: ((message: ChatMessage) => void) | null = null;

  constructor() {
    this.manager = new BleManager();
  }

  async initialize(): Promise<void> {
    try {
      console.log('🔧 [DEBUG] Initializing BLE service...');
      
      return new Promise((resolve, reject) => {
        const subscription = this.manager.onStateChange((state) => {
          try {
            console.log('🔧 [DEBUG] BLE State changed to:', state);
            
            switch (state) {
              case 'PoweredOn':
                console.log('✅ [DEBUG] BLE is ready');
                subscription.remove();
                resolve();
                break;
              case 'PoweredOff':
                console.log('❌ [DEBUG] Bluetooth is turned off');
                subscription.remove();
                reject(new Error('Bluetooth is turned off. Please enable Bluetooth in your device settings.'));
                break;
              case 'Unauthorized':
                console.log('❌ [DEBUG] Bluetooth permission denied');
                subscription.remove();
                reject(new Error('Bluetooth permission denied. Please grant Bluetooth permissions in app settings.'));
                break;
              case 'Unsupported':
                console.log('❌ [DEBUG] Bluetooth not supported');
                subscription.remove();
                reject(new Error('Bluetooth Low Energy is not supported on this device.'));
                break;
              default:
                console.log(`🔧 [DEBUG] BLE State: ${state} (waiting...)`);
                break;
            }
          } catch (stateError) {
            console.error('❌ [DEBUG] Error in BLE state handler:', stateError);
            subscription.remove();
            reject(stateError);
          }
        }, true);
      });
    } catch (error) {
      console.error('❌ [DEBUG] Critical BLE initialization error:', error);
      throw error;
    }
  }

  async scanForDevices(): Promise<LoRaDevice[]> {
    const devices: LoRaDevice[] = [];
    const deviceIds = new Set<string>();
    const allDevices: any[] = [];

    try {
      console.log('🔍 [DEBUG] Starting comprehensive device scan...');
      
      // Check BLE state
      const state = await this.manager.state();
      console.log('📡 [DEBUG] Current BLE state:', state);
      
      if (state !== 'PoweredOn') {
        throw new Error(`Bluetooth is not ready. Current state: ${state}`);
      }
      
      // Stop any existing scan
      await this.manager.stopDeviceScan();
      await new Promise(resolve => setTimeout(resolve, 500));

      return new Promise((resolve, reject) => {
        let scanTimeout: NodeJS.Timeout;
        let deviceCount = 0;

        console.log('🔍 [DEBUG] Starting device scan (15 seconds)...');
        
        this.manager.startDeviceScan(
          null, // Scan all devices
          { allowDuplicates: false },
          (error, device) => {
            if (error) {
              console.error('❌ [DEBUG] Scan error:', {
                message: error.message,
                code: error.errorCode,
                reason: error.reason
              });
              
              if (scanTimeout) clearTimeout(scanTimeout);
              this.manager.stopDeviceScan();
              reject(error);
              return;
            }

            if (device) {
              deviceCount++;
              
              // Log every device found
              const deviceInfo = {
                id: device.id,
                name: device.name || 'Unknown',
                rssi: device.rssi,
                isConnectable: device.isConnectable,
                manufacturerData: device.manufacturerData,
                serviceUUIDs: device.serviceUUIDs
              };
              
              allDevices.push(deviceInfo);
              console.log(`📱 [DEBUG] Device #${deviceCount}:`, deviceInfo);

              // Check for LoRa devices with very broad matching
              if (device.name && !deviceIds.has(device.id)) {
                const deviceName = device.name;
                const lowerName = deviceName.toLowerCase();
                
                // Very broad matching for testing
                const isLoRaDevice = 
                  deviceName === 'M1' || 
                  deviceName === 'M2' ||
                  deviceName.startsWith('M1') || 
                  deviceName.startsWith('M2') ||
                  lowerName.includes('lora') ||
                  lowerName.includes('esp32') ||
                  deviceName.includes('LoRa') ||
                  deviceName.includes('ESP32');
                
                if (isLoRaDevice) {
                  deviceIds.add(device.id);
                  console.log('🎯 [DEBUG] FOUND LORA DEVICE:', deviceName);
                  
                  let stationType: 'M1' | 'M2' = 'M1';
                  if (deviceName.includes('M2') || deviceName === 'M2' || lowerName.includes('m2')) {
                    stationType = 'M2';
                  }
                  
                  devices.push({
                    id: device.id,
                    name: deviceName,
                    rssi: device.rssi || undefined,
                    stationType: stationType,
                  });
                  
                  console.log(`✨ [DEBUG] Added LoRa device: ${deviceName} (${stationType})`);
                }
              }
            }
          }
        );

        // Progress updates
        let timeRemaining = 15;
        const progressInterval = setInterval(() => {
          timeRemaining--;
          if (timeRemaining % 3 === 0 && timeRemaining > 0) {
            console.log(`⏱️ [DEBUG] Scanning... ${timeRemaining}s left, found ${deviceCount} total devices, ${devices.length} LoRa devices`);
          }
        }, 1000);

        scanTimeout = setTimeout(() => {
          clearInterval(progressInterval);
          this.manager.stopDeviceScan();
          
          console.log('🏁 [DEBUG] Scan completed!');
          console.log(`📊 [DEBUG] Summary: Found ${deviceCount} total devices, ${devices.length} LoRa devices`);
          
          // Log all devices for debugging
          if (allDevices.length > 0) {
            console.log('📱 [DEBUG] All devices found:');
            allDevices.forEach((dev, index) => {
              console.log(`  ${index + 1}. ${dev.name} (${dev.id.substring(0, 8)}...) RSSI: ${dev.rssi}`);
            });
          } else {
            console.log('😞 [DEBUG] No devices found at all. This indicates:');
            console.log('  - Bluetooth scan is not working');
            console.log('  - Permission issues');
            console.log('  - Location services disabled');
            console.log('  - Android BLE scan restrictions');
          }
          
          if (devices.length === 0) {
            console.log('💡 [DEBUG] No LoRa devices found. Troubleshooting:');
            console.log('  - Make sure ESP32 devices are powered on');
            console.log('  - Check ESP32 BLE advertising is active');
            console.log('  - Verify device names (should be M1, M2, or contain ESP32/LoRa)');
            console.log('  - Move closer to ESP32 devices (< 10 meters)');
          }
          
          resolve(devices);
        }, 15000);
      });
    } catch (error) {
      console.error('❌ [DEBUG] Device scan failed:', error);
      await this.manager.stopDeviceScan();
      throw error;
    }
  }

  // Add all other methods from original BLEService...
  async connectToDevice(deviceId: string): Promise<boolean> {
    // Implementation same as original
    return false;
  }

  setMessageCallback(callback: (message: ChatMessage) => void): void {
    this.messageCallback = callback;
  }

  async disconnect(): Promise<void> {
    // Implementation same as original
  }

  isConnected(): boolean {
    return this.connectedDevice !== null;
  }

  getConnectedDeviceName(): string | null {
    return this.connectedDevice?.name || null;
  }

  getConnectedStationType(): 'M1' | 'M2' | null {
    return null;
  }

  async sendMessage(text: string, messageId?: string): Promise<boolean> {
    return false;
  }
}