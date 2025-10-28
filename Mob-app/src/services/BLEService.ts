import {BleManager, Device, Characteristic} from 'react-native-ble-plx';
import {LORA_BLE_CONFIG, LoRaDevice, ChatMessage} from '../types';

export class BLEService {
  private manager: BleManager;
  private connectedDevice: Device | null = null;
  private messageCallback: ((message: ChatMessage) => void) | null = null;
  private statusCallback: ((status: string) => void) | null = null;

  constructor() {
    this.manager = new BleManager();
  }

  async initialize(): Promise<void> {
    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        console.log('⏰ BLE initialization timeout');
        resolve(); // Don't reject, just resolve to prevent crashes
      }, 10000);

      try {
        const subscription = this.manager.onStateChange((state) => {
          console.log('📡 BLE State:', state);
          
          if (state === 'PoweredOn') {
            clearTimeout(timeout);
            subscription.remove();
            resolve();
          } else if (state === 'PoweredOff' || state === 'Unauthorized') {
            clearTimeout(timeout);
            subscription.remove();
            console.log('❌ BLE not available:', state);
            resolve(); // Don't reject to prevent crashes
          }
        }, true);
      } catch (error) {
        clearTimeout(timeout);
        console.error('❌ BLE initialization error:', error);
        resolve(); // Don't reject to prevent crashes
      }
    });
  }

  async scanForDevices(): Promise<LoRaDevice[]> {
    const devices: LoRaDevice[] = [];
    const foundDevices = new Set<string>();

    return new Promise((resolve) => {
      console.log('🔍 Scanning for M1/M2 LoRa stations...');
      
      try {
        // Scan without service filter first (more reliable)
        this.manager.startDeviceScan(null, null, (error, device) => {
          if (error) {
            console.error('❌ Scan error:', error);
            return;
          }

          if (device && device.name && 
              (device.name === 'M1' || device.name === 'M2' || 
               device.name.includes('LoRa') || device.name.includes('ESP32')) &&
              !foundDevices.has(device.id)) {
            
            foundDevices.add(device.id);
            
            const stationType: 'M1' | 'M2' = device.name.includes('M1') || device.name === 'M1' ? 'M1' : 'M2';
            
            devices.push({
              id: device.id,
              name: device.name,
              isConnected: false,
              rssi: device.rssi || undefined,
              stationType: stationType,
            });

            console.log(`📡 Found ${device.name} station (RSSI: ${device.rssi}dBm)`);
          }
        });
      } catch (error) {
        console.error('❌ Failed to start scan:', error);
      }

      // Stop scanning after 10 seconds 
      setTimeout(() => {
        try {
          this.manager.stopDeviceScan();
          console.log(`✅ Scan complete. Found ${devices.length} LoRa station(s)`);
          resolve(devices);
        } catch (error) {
          console.error('❌ Failed to stop scan:', error);
          resolve(devices);
        }
      }, 10000);
    });
  }

  async connectToDevice(deviceId: string): Promise<boolean> {
    try {
      console.log('🔗 Connecting to LoRa station:', deviceId);
      this.connectedDevice = await this.manager.connectToDevice(deviceId);
      
      console.log('🔍 Discovering services and characteristics...');
      await this.connectedDevice.discoverAllServicesAndCharacteristics();
      
      console.log('📡 Setting up message notifications...');
      await this.setupNotifications();
      
      console.log('✅ Successfully connected to', this.connectedDevice.name);
      return true;
    } catch (error) {
      console.error('❌ Connection error:', error);
      return false;
    }
  }

  private async setupNotifications(): Promise<void> {
    if (!this.connectedDevice) return;

    try {
      // Monitor RX characteristic for incoming messages from LoRa
      await this.connectedDevice.monitorCharacteristicForService(
        LORA_BLE_CONFIG.serviceUUID,
        LORA_BLE_CONFIG.rxCharacteristicUUID,
        (error, characteristic) => {
          if (error) {
            console.error('📡 RX Monitor error:', error);
            return;
          }

          if (characteristic?.value) {
            const message = this.parseReceivedMessage(characteristic);
            if (message && this.messageCallback) {
              this.messageCallback(message);
            }
          }
        }
      );

      // Monitor status characteristic for connection status
      await this.connectedDevice.monitorCharacteristicForService(
        LORA_BLE_CONFIG.serviceUUID,
        LORA_BLE_CONFIG.statusCharacteristicUUID,
        (error, characteristic) => {
          if (error) {
            console.error('📊 Status Monitor error:', error);
            return;
          }

          if (characteristic?.value && this.statusCallback) {
            const statusValue = atob(characteristic.value);
            console.log('📊 Status update:', statusValue);
            this.statusCallback(statusValue);
          }
        }
      );

      console.log('✅ Notifications setup complete');
    } catch (error) {
      console.error('❌ Setup notifications error:', error);
    }
  }

  private parseReceivedMessage(characteristic: Characteristic): ChatMessage | null {
    try {
      if (!characteristic.value) return null;
      
      const decodedData = atob(characteristic.value);
      console.log('📨 Received LoRa message:', decodedData);
      
      // Parse formatted message from ESP32: "[M1→M2 12:34:56] Hello World"
      const formatRegex = /\[(M[12])→(M[12]) (\d{2}:\d{2}:\d{2})\] (.+)/;
      const match = decodedData.match(formatRegex);
      
      if (match) {
        const [, fromStation, toStation, timeStr, messageText] = match;
        
        return {
          id: `lora_${Date.now()}_${Math.random()}`,
          text: messageText,
          timestamp: new Date(),
          fromDevice: `${fromStation} Station`,
          deviceId: fromStation === 'M1' ? 1 : 2,
          stationType: fromStation as 'M1' | 'M2',
          isOutgoing: false,
        };
      } else {
        // Fallback for non-formatted messages
        return {
          id: `msg_${Date.now()}_${Math.random()}`,
          text: decodedData,
          timestamp: new Date(),
          fromDevice: this.connectedDevice?.name || 'LoRa Device',
          deviceId: 0,
          isOutgoing: false,
        };
      }
    } catch (error) {
      console.error('❌ Parse message error:', error);
      return null;
    }
  }

  async sendMessage(text: string): Promise<boolean> {
    if (!this.connectedDevice) {
      console.error('❌ No connected device');
      return false;
    }

    if (!text.trim()) {
      console.error('❌ Empty message');
      return false;
    }

    try {
      console.log(`📤 Sending message via ${this.connectedDevice.name}: "${text}"`);
      
      // Encode message as base64 for BLE transmission
      const messageData = btoa(text);
      
      await this.connectedDevice.writeCharacteristicWithResponseForService(
        LORA_BLE_CONFIG.serviceUUID,
        LORA_BLE_CONFIG.txCharacteristicUUID,
        messageData
      );

      // Add sent message to callback with proper station type
      if (this.messageCallback) {
        const stationType = this.connectedDevice.name === 'M1' ? 'M1' : 'M2';
        const sentMessage: ChatMessage = {
          id: `sent_${Date.now()}_${Math.random()}`,
          text,
          timestamp: new Date(),
          fromDevice: 'You',
          deviceId: 0,
          stationType: stationType,
          isOutgoing: true,
        };
        this.messageCallback(sentMessage);
      }

      console.log('✅ Message sent successfully');
      return true;
    } catch (error) {
      console.error('❌ Send message error:', error);
      return false;
    }
  }

  setMessageCallback(callback: (message: ChatMessage) => void): void {
    this.messageCallback = callback;
  }

  setStatusCallback(callback: (status: string) => void): void {
    this.statusCallback = callback;
  }

  async disconnect(): Promise<void> {
    if (this.connectedDevice) {
      try {
        console.log('🔌 Disconnecting from', this.connectedDevice.name);
        await this.manager.cancelDeviceConnection(this.connectedDevice.id);
        this.connectedDevice = null;
        console.log('✅ Disconnected successfully');
      } catch (error) {
        console.error('❌ Disconnect error:', error);
      }
    }
  }

  getConnectedDevice(): Device | null {
    return this.connectedDevice;
  }

  getConnectedStationType(): 'M1' | 'M2' | null {
    if (this.connectedDevice?.name === 'M1') return 'M1';
    if (this.connectedDevice?.name === 'M2') return 'M2';
    return null;
  }

  destroy(): void {
    this.manager.destroy();
  }
}