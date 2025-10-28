export interface LoRaDevice {
  id: string;
  name: string;
  isConnected: boolean;
  rssi?: number;
  stationType?: 'M1' | 'M2';
}

export interface ChatMessage {
  id: string;
  text: string;
  timestamp: Date;
  fromDevice: string;
  deviceId: number;
  rssi?: number;
  snr?: number;
  isOutgoing: boolean;
  stationType?: 'M1' | 'M2';
}

export interface BLECharacteristics {
  serviceUUID: string;
  rxCharacteristicUUID: string;
  txCharacteristicUUID: string;
  statusCharacteristicUUID: string;
}

// Updated UUIDs to match ESP32 firmware exactly
export const LORA_BLE_CONFIG: BLECharacteristics = {
  serviceUUID: '12345678-1234-1234-1234-123456789abc',
  rxCharacteristicUUID: '12345678-1234-1234-1234-123456789abd',  // Phone receives messages here
  txCharacteristicUUID: '12345678-1234-1234-1234-123456789abe',  // Phone sends messages here
  statusCharacteristicUUID: '12345678-1234-1234-1234-123456789abf', // Status updates
};

export interface MessagePacket {
  timestamp: number;
  messageLen: number;
  message: string;
}