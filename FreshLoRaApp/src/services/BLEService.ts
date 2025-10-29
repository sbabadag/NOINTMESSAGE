import {BleManager, Device, Characteristic} from 'react-native-ble-plx';
import {LORA_BLE_CONFIG, LoRaDevice, ChatMessage} from '../types';

export class BLEService {
  private manager: BleManager;
  private connectedDevice: Device | null = null;
  private messageCallback: ((message: ChatMessage) => void) | null = null;

  constructor() {
    this.manager = new BleManager();
  }

  async initialize(): Promise<void> {
    try {
      console.log(' Initializing BLE service...');
      
      // Set up timeout for initialization
      const initTimeout = setTimeout(() => {
        console.log(' BLE initialization timeout - continuing anyway');
      }, 5000);

      return new Promise((resolve, reject) => {
        const subscription = this.manager.onStateChange((state) => {
          try {
            console.log(' BLE State changed to:', state);
            
            switch (state) {
              case 'PoweredOn':
                console.log(' BLE is ready');
                clearTimeout(initTimeout);
                subscription.remove();
                resolve();
                break;
              case 'PoweredOff':
                console.log(' Bluetooth is turned off');
                subscription.remove();
                reject(new Error('Bluetooth is turned off'));
                break;
              case 'Unauthorized':
                console.log(' Bluetooth permission denied');
                subscription.remove();
                reject(new Error('Bluetooth permission denied'));
                break;
              default:
                console.log(` BLE State: ${state} (waiting...)`);
                break;
            }
          } catch (stateError) {
            console.error(' Error in BLE state handler:', stateError);
            subscription.remove();
            reject(stateError);
          }
        }, true); // Get initial state immediately
      });
    } catch (error) {
      console.error(' Critical BLE initialization error:', error);
      throw error;
    }
  }

  async scanForDevices(): Promise<LoRaDevice[]> {
    const devices: LoRaDevice[] = [];
    const deviceIds = new Set<string>();

    try {
      console.log(' ?? Starting BLE scan for ESP32 LoRa devices...');
      
      // Check BLE state before scanning
      const state = await this.manager.state();
      console.log(' ?? Current BLE state:', state);
      
      if (state !== 'PoweredOn') {
        throw new Error(`Bluetooth is not ready. Current state: ${state}. Please enable Bluetooth and try again.`);
      }
      
      // Stop any existing scan
      await this.manager.stopDeviceScan();
      
      // Small delay to ensure scan is stopped
      await new Promise(resolve => setTimeout(resolve, 500));

      return new Promise((resolve, reject) => {
        let scanTimeout: NodeJS.Timeout;
        let scanStarted = false;

        // Scan for all devices with enhanced error handling
        this.manager.startDeviceScan(
          null, // Don't filter by service UUID as ESP32 might not advertise it
          { 
            allowDuplicates: false
          }, 
          (error, device) => {
            if (error) {
              console.error(' ? BLE scan error:', error);
              console.error(' Error details:', {
                message: error.message,
                code: error.errorCode,
                reason: error.reason
              });
              
              if (scanTimeout) clearTimeout(scanTimeout);
              this.manager.stopDeviceScan();
              
              // Provide specific error messages
              let errorMessage = 'Failed to scan for devices. ';
              if (error.message.includes('Location')) {
                errorMessage += 'Please enable Location services and grant location permission.';
              } else if (error.message.includes('Permission')) {
                errorMessage += 'Please grant Bluetooth permissions in app settings.';
              } else if (error.message.includes('Bluetooth')) {
                errorMessage += 'Please ensure Bluetooth is enabled.';
              } else {
                errorMessage += `Error: ${error.message}`;
              }
              
              reject(new Error(errorMessage));
              return;
            }

            if (!scanStarted) {
              scanStarted = true;
              console.log(' ? BLE scan started successfully');
            }

            if (device) {
              // Log all devices for debugging
              console.log(` ?? Found device: "${device.name || 'Unknown'}" (ID: ${device.id.substring(0,8)}..., RSSI: ${device.rssi})`);
              
            // Look for ESP32 LoRa devices with pattern matching for your firmware
            if (device.name && !deviceIds.has(device.id)) {
              const deviceName = device.name.toLowerCase();
              const originalName = device.name;
              
              // Device detection patterns matching your firmware
              const isLoRaDevice = 
                originalName === 'M1-LoRa-Bridge' ||
                originalName === 'M2-LoRa-Bridge' ||
                originalName === 'M1' || 
                originalName === 'M2' ||
                originalName.includes('LoRa-Bridge') ||
                originalName.includes('LoRa_ESP32') || 
                originalName.includes('LoRa_') || 
                originalName.includes('ESP32') ||
                deviceName.includes('lora') ||
                deviceName.includes('m1') ||
                deviceName.includes('m2') ||
                originalName.startsWith('M1_') ||
                originalName.startsWith('M2_');
                
                if (isLoRaDevice) {
                  deviceIds.add(device.id);
                  console.log(' ?? Detected ESP32 LoRa device:', originalName);
                  
                // Station type detection matching your firmware
                let stationType: 'M1' | 'M2' = 'M1';
                if (originalName === 'M2-LoRa-Bridge' ||
                    originalName.includes('M2') || 
                    originalName === 'M2' || 
                    deviceName.includes('m2') ||
                    originalName.startsWith('M2_')) {
                  stationType = 'M2';
                }                  devices.push({
                    id: device.id,
                    name: originalName,
                    rssi: device.rssi || undefined,
                    stationType: stationType,
                  });
                  
                  console.log(` ? Added device: ${originalName} (${stationType})`);
                }
              }
            }
          }
        );

        // Set scan timeout with progress updates
        let timeRemaining = 15; // Increased to 15 seconds
        const progressInterval = setInterval(() => {
          timeRemaining--;
          if (timeRemaining > 0 && timeRemaining % 3 === 0) {
            console.log(` ?? Scanning... ${timeRemaining}s remaining, found ${devices.length} devices`);
          }
        }, 1000);

        scanTimeout = setTimeout(() => {
          clearInterval(progressInterval);
          this.manager.stopDeviceScan();
          console.log(` ?? Scan complete. Found ${devices.length} ESP32 LoRa devices.`);
          
          if (devices.length === 0) {
            console.log(' ?? Troubleshooting tips:');
            console.log('   - Ensure ESP32 devices are powered on');
            console.log('   - Check ESP32 is advertising BLE');
            console.log('   - Move closer to ESP32 devices');
            console.log('   - Check device names match patterns (M1, M2, ESP32, LoRa_*)');
          }
          
          resolve(devices);
        }, 15000); // 15 second timeout
      });
    } catch (error) {
      console.error(' ? Device scan failed:', error);
      await this.manager.stopDeviceScan(); // Ensure scan is stopped
      throw error;
    }
  }

  async connectToDevice(deviceId: string): Promise<boolean> {
    try {
      console.log(' Connecting to ESP32 device...');
      
      // Disconnect any existing connection
      if (this.connectedDevice) {
        await this.connectedDevice.cancelConnection();
      }

      // Connect with timeout
      this.connectedDevice = await this.manager.connectToDevice(deviceId, {
        timeout: 10000,
      });

      console.log(' Discovering services and characteristics...');
      await this.connectedDevice.discoverAllServicesAndCharacteristics();

      console.log(' Connected and ready for messaging');
      
      // Set up notifications for incoming messages
      await this.setupNotifications();
      
      return true;
    } catch (error) {
      console.error(' Connection failed:', error);
      this.connectedDevice = null;
      return false;
    }
  }

  private async setupNotifications(): Promise<void> {
    if (!this.connectedDevice) {
      throw new Error('No device connected');
    }

    try {
      console.log(' Setting up notifications for incoming messages...');
      
      // Monitor the TX characteristic (ESP32 sends data here)
      this.connectedDevice.monitorCharacteristicForService(
        LORA_BLE_CONFIG.serviceUUID,
        LORA_BLE_CONFIG.rxCharacteristicUUID,
        (error, characteristic) => {
          if (error) {
            console.error(' Notification error:', error);
            return;
          }

          if (characteristic?.value) {
            try {
              // Decode base64 to string - your firmware sends plain text
              const message = atob(characteristic.value);
              console.log(' Received raw message from ESP32:', message);
              
              this.handleIncomingMessage(message);
            } catch (decodeError) {
              console.error(' Failed to decode incoming message:', decodeError);
            }
          }
        }
      );

      console.log(' Notifications enabled');
    } catch (error) {
      console.error(' Failed to setup notifications:', error);
      throw error;
    }
  }

  private handleIncomingMessage(rawMessage: string): void {
    try {
      console.log(' Processing incoming LoRa message:', rawMessage);

      // Your firmware sends plain text messages from the other station
      // The message comes from the remote station via LoRa
      const messageText = rawMessage.trim();
      
      // Determine which station this came from based on connected device
      const connectedStationType = this.getConnectedStationType();
      const fromStationType = connectedStationType === 'M1' ? 'M2' : 'M1'; // Message comes from the other station
      
      // Convert to ChatMessage format
      const chatMessage: ChatMessage = {
        id: Date.now().toString() + Math.random(),
        text: messageText,
        sender: 'remote',
        timestamp: new Date(),
        isOwn: false,
        deviceId: fromStationType === 'M1' ? 1 : 2,
        stationType: fromStationType,
      };

      console.log(' Processed message:', chatMessage);
      
      // Notify the UI
      if (this.messageCallback) {
        this.messageCallback(chatMessage);
      }
    } catch (error) {
      console.error(' Failed to process incoming message:', error);
    }
  }

  async sendMessage(text: string, messageId?: string): Promise<boolean> {
    if (!this.connectedDevice) {
      console.error(' No device connected for sending');
      return false;
    }

    try {
      console.log(' Sending message to ESP32 via BLE:', text);

      // Your firmware expects plain text, not JSON
      const messageText = text.trim();
      console.log(' Sending plain text:', messageText);

      // Convert string to base64 for BLE transmission
      const base64Message = btoa(messageText);

      // Send to ESP32 RX characteristic (where ESP32 receives data)
      await this.connectedDevice.writeCharacteristicWithResponseForService(
        LORA_BLE_CONFIG.serviceUUID,
        LORA_BLE_CONFIG.txCharacteristicUUID,
        base64Message
      );

      console.log(' Message sent successfully to ESP32');
      return true;
    } catch (error) {
      console.error(' Failed to send message:', error);
      return false;
    }
  }

  setMessageCallback(callback: (message: ChatMessage) => void): void {
    this.messageCallback = callback;
  }

  async disconnect(): Promise<void> {
    try {
      if (this.connectedDevice) {
        await this.connectedDevice.cancelConnection();
        this.connectedDevice = null;
        console.log(' Disconnected from ESP32');
      }
    } catch (error) {
      console.error(' Disconnect error:', error);
    }
  }

  isConnected(): boolean {
    return this.connectedDevice !== null;
  }

  getConnectedDeviceName(): string | null {
    return this.connectedDevice?.name || null;
  }

  getConnectedStationType(): 'M1' | 'M2' | null {
    if (!this.connectedDevice) return null;
    
    const name = this.connectedDevice.name;
    if (name === 'M2-LoRa-Bridge' || name?.includes('M2') || name === 'M2') return 'M2';
    if (name === 'M1-LoRa-Bridge' || name?.includes('M1') || name === 'M1') return 'M1';
    return 'M1'; // Default to M1
  }
}
