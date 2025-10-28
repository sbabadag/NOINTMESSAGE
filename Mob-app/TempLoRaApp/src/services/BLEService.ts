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
    return new Promise((resolve) => {
      const subscription = this.manager.onStateChange((state) => {
        if (state === 'PoweredOn') {
          subscription.remove();
          resolve();
        }
      }, true);
    });
  }

  async scanForDevices(): Promise<LoRaDevice[]> {
    const devices: LoRaDevice[] = [];
    const foundDevices = new Set<string>();

    return new Promise((resolve) => {
      this.manager.startDeviceScan(null, null, (error, device) => {
        if (error) {
          console.error('Scan error:', error);
          return;
        }

        if (device && device.name && 
            (device.name === 'M1' || device.name === 'M2' || device.name.includes('MessageTunnel')) &&
            !foundDevices.has(device.id)) {
          
          foundDevices.add(device.id);
          devices.push({
            id: device.id,
            name: device.name,
            isConnected: false,
            rssi: device.rssi || undefined,
          });
        }
      });

      // Stop scanning after 10 seconds
      setTimeout(() => {
        this.manager.stopDeviceScan();
        resolve(devices);
      }, 10000);
    });
  }

  async connectToDevice(deviceId: string): Promise<boolean> {
    try {
      console.log('Connecting to device:', deviceId);
      this.connectedDevice = await this.manager.connectToDevice(deviceId);
      
      console.log('Discovering services...');
      await this.connectedDevice.discoverAllServicesAndCharacteristics();
      
      console.log('Setting up notifications...');
      await this.setupNotifications();
      
      return true;
    } catch (error) {
      console.error('Connection error:', error);
      return false;
    }
  }

  private async setupNotifications(): Promise<void> {
    if (!this.connectedDevice) return;

    try {
      await this.connectedDevice.monitorCharacteristicForService(
        LORA_BLE_CONFIG.serviceUUID,
        LORA_BLE_CONFIG.rxCharacteristicUUID,
        (error, characteristic) => {
          if (error) {
            console.error('Monitor error:', error);
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
    } catch (error) {
      console.error('Setup notifications error:', error);
    }
  }

  private parseReceivedMessage(characteristic: Characteristic): ChatMessage | null {
    try {
      if (!characteristic.value) return null;
      
      const decodedData = decodeURIComponent(escape(atob(characteristic.value)));
      console.log('Received raw data:', decodedData);
      
      // Try to parse as JSON first, fallback to plain text
      let messageData;
      try {
        messageData = JSON.parse(decodedData);
      } catch {
        // If not JSON, treat as plain text message
        messageData = {
          message: decodedData,
          device_id: 0,
          rssi: 0,
          snr: 0,
          msg_id: Date.now()
        };
      }

      return {
        id: `msg_${Date.now()}_${Math.random()}`,
        text: messageData.message || decodedData,
        timestamp: new Date(),
        fromDevice: this.connectedDevice?.name || 'Unknown',
        deviceId: messageData.device_id || 0,
        rssi: messageData.rssi,
        snr: messageData.snr,
        isOutgoing: false,
      };
    } catch (error) {
      console.error('Parse message error:', error);
      return null;
    }
  }

  async sendMessage(text: string): Promise<boolean> {
    if (!this.connectedDevice) {
      console.error('No connected device');
      return false;
    }

    try {
      const messageData = btoa(unescape(encodeURIComponent(text)));
      
      await this.connectedDevice.writeCharacteristicWithResponseForService(
        LORA_BLE_CONFIG.serviceUUID,
        LORA_BLE_CONFIG.txCharacteristicUUID,
        messageData
      );

      // Add sent message to callback
      if (this.messageCallback) {
        const sentMessage: ChatMessage = {
          id: `sent_${Date.now()}_${Math.random()}`,
          text,
          timestamp: new Date(),
          fromDevice: 'You',
          deviceId: 0,
          isOutgoing: true,
        };
        this.messageCallback(sentMessage);
      }

      return true;
    } catch (error) {
      console.error('Send message error:', error);
      return false;
    }
  }

  setMessageCallback(callback: (message: ChatMessage) => void): void {
    this.messageCallback = callback;
  }

  async disconnect(): Promise<void> {
    if (this.connectedDevice) {
      try {
        await this.manager.cancelDeviceConnection(this.connectedDevice.id);
        this.connectedDevice = null;
      } catch (error) {
        console.error('Disconnect error:', error);
      }
    }
  }

  getConnectedDevice(): Device | null {
    return this.connectedDevice;
  }

  destroy(): void {
    this.manager.destroy();
  }
}