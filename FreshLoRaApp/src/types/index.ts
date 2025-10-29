export interface ChatMessage {
  id: string;
  text: string;
  sender: string;
  timestamp: Date;
  isOwn: boolean;
  deviceId?: number;
  stationType?: 'M1' | 'M2';
}

export interface LoRaDevice {
  id: string;
  name: string;
  rssi?: number;
  stationType?: 'M1' | 'M2';
}

export interface MessagePacket {
  messageId: string;
  text: string;
  timestamp: string;
  sender: string;
  stationType?: 'M1' | 'M2';
}

export interface BLECharacteristics {
  serviceUUID: string;
  rxCharacteristicUUID: string;
  txCharacteristicUUID: string;
  statusCharacteristicUUID: string;
}

// Updated to match ESP32 firmware exactly - Nordic UART Service
export const LORA_BLE_CONFIG: BLECharacteristics = {
  serviceUUID: '6E400001-B5A3-F393-E0A9-E50E24DCCA9E',           // Nordic UART Service
  rxCharacteristicUUID: '6E400003-B5A3-F393-E0A9-E50E24DCCA9E',  // Phone receives from ESP32 TX
  txCharacteristicUUID: '6E400002-B5A3-F393-E0A9-E50E24DCCA9E',  // Phone sends to ESP32 RX
  statusCharacteristicUUID: '6E400001-B5A3-F393-E0A9-E50E24DCCA9E', // Status updates
};
