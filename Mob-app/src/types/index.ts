export interface LoRaDevice {
  id: string;
  name: string;
  isConnected: boolean;
  rssi?: number;
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
}

export interface BLECharacteristics {
  serviceUUID: string;
  rxCharacteristicUUID: string;
  txCharacteristicUUID: string;
}

export const LORA_BLE_CONFIG: BLECharacteristics = {
  serviceUUID: '12345678-1234-1234-1234-123456789abc',
  rxCharacteristicUUID: '12345678-1234-1234-1234-123456789abd',
  txCharacteristicUUID: '12345678-1234-1234-1234-123456789abe',
};