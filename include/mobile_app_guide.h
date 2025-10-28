/*
 * Mobile Phone App Development Guide
 * 
 * This file provides guidance for developing mobile applications
 * that can communicate with the LoRa tunnel devices via Bluetooth.
 */

#ifndef MOBILE_APP_GUIDE_H
#define MOBILE_APP_GUIDE_H

// ===== BLUETOOTH SERVICE CONFIGURATION =====
/*
Service UUID: 12345678-1234-1234-1234-123456789abc
Device Name: LoRa_Tunnel

Characteristics:
1. TX Characteristic (Device → Phone)
   - UUID: 87654321-4321-4321-4321-cba987654321
   - Properties: NOTIFY
   - Use: Receive messages from remote phone via LoRa

2. RX Characteristic (Phone → Device)  
   - UUID: 11111111-2222-3333-4444-555555555555
   - Properties: WRITE, WRITE_NO_RESPONSE
   - Use: Send messages to remote phone via LoRa
*/

// ===== EXAMPLE ANDROID CODE (Kotlin) =====
/*
class LoRaTunnelService {
    companion object {
        const val SERVICE_UUID = "12345678-1234-1234-1234-123456789abc"
        const val CHAR_TX_UUID = "87654321-4321-4321-4321-cba987654321" // Receive
        const val CHAR_RX_UUID = "11111111-2222-3333-4444-555555555555" // Send
    }
    
    private var bluetoothGatt: BluetoothGatt? = null
    private var rxCharacteristic: BluetoothGattCharacteristic? = null
    private var txCharacteristic: BluetoothGattCharacteristic? = null
    
    // Connect to device
    fun connectToDevice(device: BluetoothDevice) {
        bluetoothGatt = device.connectGatt(context, false, gattCallback)
    }
    
    // Send message to remote phone
    fun sendMessage(message: String) {
        rxCharacteristic?.let { char ->
            char.value = message.toByteArray()
            bluetoothGatt?.writeCharacteristic(char)
        }
    }
    
    // GATT callback to handle connection and data
    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt?, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                gatt?.discoverServices()
            }
        }
        
        override fun onServicesDiscovered(gatt: BluetoothGatt?, status: Int) {
            val service = gatt?.getService(UUID.fromString(SERVICE_UUID))
            rxCharacteristic = service?.getCharacteristic(UUID.fromString(CHAR_RX_UUID))
            txCharacteristic = service?.getCharacteristic(UUID.fromString(CHAR_TX_UUID))
            
            // Enable notifications for receiving messages
            txCharacteristic?.let { char ->
                gatt.setCharacteristicNotification(char, true)
                val descriptor = char.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"))
                descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                gatt.writeDescriptor(descriptor)
            }
        }
        
        override fun onCharacteristicChanged(gatt: BluetoothGatt?, characteristic: BluetoothGattCharacteristic?) {
            if (characteristic?.uuid.toString() == CHAR_TX_UUID) {
                val receivedMessage = String(characteristic.value)
                // Handle received message from remote phone
                onMessageReceived(receivedMessage)
            }
        }
    }
    
    fun onMessageReceived(message: String) {
        // Update UI with received message
        // This message came from the remote phone via LoRa
    }
}
*/

// ===== EXAMPLE iOS CODE (Swift) =====
/*
import CoreBluetooth

class LoRaTunnelService: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    private let serviceUUID = CBUUID(string: "12345678-1234-1234-1234-123456789abc")
    private let charTXUUID = CBUUID(string: "87654321-4321-4321-4321-cba987654321") // Receive
    private let charRXUUID = CBUUID(string: "11111111-2222-3333-4444-555555555555") // Send
    
    private var centralManager: CBCentralManager!
    private var peripheral: CBPeripheral?
    private var rxCharacteristic: CBCharacteristic?
    private var txCharacteristic: CBCharacteristic?
    
    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }
    
    // Start scanning for devices
    func startScanning() {
        centralManager.scanForPeripherals(withServices: [serviceUUID], options: nil)
    }
    
    // Send message to remote phone
    func sendMessage(_ message: String) {
        guard let peripheral = peripheral,
              let characteristic = rxCharacteristic else { return }
        
        let data = message.data(using: .utf8)!
        peripheral.writeValue(data, for: characteristic, type: .withoutResponse)
    }
    
    // MARK: - CBCentralManagerDelegate
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            startScanning()
        }
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, 
                       advertisementData: [String : Any], rssi RSSI: NSNumber) {
        if peripheral.name == "LoRa_Tunnel" {
            self.peripheral = peripheral
            peripheral.delegate = self
            centralManager.connect(peripheral, options: nil)
            centralManager.stopScan()
        }
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        peripheral.discoverServices([serviceUUID])
    }
    
    // MARK: - CBPeripheralDelegate
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let services = peripheral.services else { return }
        
        for service in services {
            if service.uuid == serviceUUID {
                peripheral.discoverCharacteristics([charTXUUID, charRXUUID], for: service)
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let characteristics = service.characteristics else { return }
        
        for characteristic in characteristics {
            if characteristic.uuid == charTXUUID {
                txCharacteristic = characteristic
                peripheral.setNotifyValue(true, for: characteristic)
            } else if characteristic.uuid == charRXUUID {
                rxCharacteristic = characteristic
            }
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        if characteristic.uuid == charTXUUID {
            if let data = characteristic.value,
               let message = String(data: data, encoding: .utf8) {
                onMessageReceived(message)
            }
        }
    }
    
    func onMessageReceived(_ message: String) {
        // Handle received message from remote phone via LoRa
        DispatchQueue.main.async {
            // Update UI
        }
    }
}
*/

// ===== MESSAGE PROTOCOLS =====
/*
The devices support any binary or text data, but here are some
suggested message formats for building chat applications:

1. Simple Text Message:
   "Hello from Phone A!"

2. JSON Message Format:
   {
     "type": "message",
     "sender": "Alice",
     "content": "Hello there!",
     "timestamp": 1640995200
   }

3. Command Messages:
   "CMD:PING"
   "CMD:STATUS"
   "CMD:DISCONNECT"

4. File Transfer (Base64):
   "FILE:image.jpg:base64data..."

5. Location Sharing:
   "LOC:40.7128,-74.0060"
*/

// ===== TESTING APPS =====
/*
For quick testing without custom app development:

Android:
- "Bluetooth Terminal" by Qwerty
- "Serial Bluetooth Terminal" by Kai Morich
- "BLE Scanner" by Bluepixel Technologies

iOS:  
- "LightBlue Explorer" by Punch Through
- "BLE Scanner 4.0" by Bluepixel Technologies
- "Bluetooth Terminal" by Hannes Schindler

Configuration:
1. Connect to "LoRa_Tunnel" device
2. Find service: 12345678-1234-1234-1234-123456789abc
3. Use RX characteristic to send data
4. Subscribe to TX characteristic to receive data
*/

#endif // MOBILE_APP_GUIDE_H