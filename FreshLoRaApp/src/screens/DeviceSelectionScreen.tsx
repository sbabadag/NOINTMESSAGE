import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  SafeAreaView,
  FlatList,
  Alert,
} from 'react-native';
import { NavigationProp } from '@react-navigation/native';
import { BLEService } from '../services/BLEService';
import { LoRaDevice } from '../types';

interface Props {
  navigation: NavigationProp<any>;
}

const DeviceSelectionScreen: React.FC<Props> = ({ navigation }) => {
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState<LoRaDevice[]>([]);
  const [bleService] = useState(new BLEService());

  useEffect(() => {
    initializeBLE();
  }, []);

  const initializeBLE = async () => {
    try {
      await bleService.initialize();
      console.log(' BLE Service initialized');
    } catch (error) {
      console.error(' BLE initialization failed:', error);
      Alert.alert(
        'Bluetooth Error', 
        'Failed to initialize Bluetooth. Make sure Bluetooth is enabled and permissions are granted.',
        [{ text: 'OK' }]
      );
    }
  };

  const addMockDevices = () => {
    const mockDevices: LoRaDevice[] = [
      { id: 'mock-m1-001', name: 'M1', rssi: -45, stationType: 'M1' },
      { id: 'mock-m2-002', name: 'M2', rssi: -52, stationType: 'M2' }
    ];
    setDevices(mockDevices);
    console.log(' 🧪 Added mock devices for testing');
  };

  const startScan = async () => {
    setScanning(true);
    setDevices([]);
    
    try {
      console.log(' 🔍 Starting BLE scan...');
      const foundDevices = await bleService.scanForDevices();
      setDevices(foundDevices);
      
      if (foundDevices.length === 0) {
        Alert.alert(
          'No Devices Found',
          'No LoRa stations detected.\n\nTroubleshooting:\n• Ensure ESP32 M1/M2 devices are powered on\n• Check Bluetooth and Location permissions\n• Make sure Location services are enabled\n• Try moving closer to devices',
          [
            { text: 'Add Mock Devices', onPress: addMockDevices },
            { text: 'OK' }
          ]
        );
      } else {
        console.log(` ✅ Found ${foundDevices.length} LoRa devices`);
      }
    } catch (error) {
      console.error(' Scan failed:', error);
      const errorMessage = error instanceof Error ? error.message : 'Failed to scan for devices. Please try again.';
      Alert.alert(
        'Scan Error', 
        errorMessage,
        [
          { text: 'Settings', onPress: () => {
            // On Android, you can't directly open settings, but we can provide guidance
            Alert.alert(
              'Permissions Required',
              'Please ensure:\n\n• Bluetooth is enabled\n• Location services are enabled\n• App has Bluetooth and Location permissions\n\nGo to Settings > Apps > LoRa BLE Chat > Permissions',
              [{ text: 'OK' }]
            );
          }},
          { text: 'Retry', onPress: startScan },
          { text: 'Cancel' }
        ]
      );
    } finally {
      setScanning(false);
    }
  };

  const connectToDevice = async (device: LoRaDevice) => {
    try {
      console.log(' Connecting to device:', device.name);
      const success = await bleService.connectToDevice(device.id);
      
      if (success) {
        console.log(' Connected successfully');
        navigation.navigate('Chat', { 
          deviceName: device.name,
          deviceId: device.id,
          stationType: device.stationType,
          bleService: bleService
        });
      } else {
        Alert.alert(
          'Connection Failed',
          `Could not connect to ${device.name}. Make sure the device is nearby and not connected to another phone.`,
          [{ text: 'OK' }]
        );
      }
    } catch (error) {
      console.error(' Connection error:', error);
      Alert.alert('Connection Error', 'An error occurred while connecting to the device.');
    }
  };

  const renderDevice = ({ item }: { item: LoRaDevice }) => (
    <TouchableOpacity style={styles.deviceItem} onPress={() => connectToDevice(item)}>
      <View style={styles.deviceInfo}>
        <Text style={styles.deviceName}> {item.name} Station</Text>
        <Text style={styles.deviceDetails}>
          Type: {item.stationType}  Signal: {item.rssi ? `${item.rssi}dBm` : 'Unknown'}
        </Text>
      </View>
      <Text style={styles.connectButton}>Connect </Text>
    </TouchableOpacity>
  );

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.content}>
        <Text style={styles.title}> LoRa Stations</Text>
        <Text style={styles.subtitle}>
          {scanning ? 'Scanning for devices...' : `Found ${devices.length} LoRa stations`}
        </Text>

        <TouchableOpacity 
          style={[styles.scanButton, scanning && styles.scanningButton]} 
          onPress={startScan}
          disabled={scanning}
        >
          <Text style={styles.scanButtonText}>
            {scanning ? ' Scanning...' : ' Scan for Devices'}
          </Text>
        </TouchableOpacity>

        <FlatList
          data={devices}
          renderItem={renderDevice}
          keyExtractor={(item) => item.id}
          style={styles.deviceList}
          showsVerticalScrollIndicator={false}
          ListEmptyComponent={
            !scanning ? (
              <View style={styles.emptyContainer}>
                <Text style={styles.emptyText}>No devices found</Text>
                <Text style={styles.emptySubtext}>Tap "Scan for Devices" to search for LoRa stations</Text>
              </View>
            ) : null
          }
        />

        <View style={styles.infoBox}>
          <Text style={styles.infoTitle}> Troubleshooting:</Text>
          <Text style={styles.infoText}> Make sure ESP32 M1/M2 stations are powered on</Text>
          <Text style={styles.infoText}> Check that Bluetooth is enabled</Text>
          <Text style={styles.infoText}> Move closer to the devices (within 10 meters)</Text>
          <Text style={styles.infoText}> Grant location permissions for BLE scanning</Text>
        </View>
      </View>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  content: {
    flex: 1,
    padding: 20,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#4285F4',
    textAlign: 'center',
    marginBottom: 8,
  },
  subtitle: {
    fontSize: 16,
    color: '#666',
    textAlign: 'center',
    marginBottom: 30,
  },
  scanButton: {
    backgroundColor: '#4285F4',
    padding: 16,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 20,
  },
  scanningButton: {
    backgroundColor: '#90CAF9',
  },
  scanButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  deviceList: {
    flex: 1,
  },
  deviceItem: {
    backgroundColor: '#ffffff',
    padding: 16,
    borderRadius: 8,
    marginBottom: 12,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    elevation: 2,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
  },
  deviceInfo: {
    flex: 1,
  },
  deviceName: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 4,
  },
  deviceDetails: {
    fontSize: 14,
    color: '#666',
  },
  connectButton: {
    fontSize: 16,
    color: '#4285F4',
    fontWeight: 'bold',
  },
  emptyContainer: {
    padding: 40,
    alignItems: 'center',
  },
  emptyText: {
    fontSize: 18,
    color: '#999',
    marginBottom: 8,
  },
  emptySubtext: {
    fontSize: 14,
    color: '#ccc',
    textAlign: 'center',
  },
  infoBox: {
    backgroundColor: '#fff3e0',
    padding: 15,
    borderRadius: 8,
    marginTop: 20,
  },
  infoTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#e65100',
    marginBottom: 8,
  },
  infoText: {
    fontSize: 14,
    color: '#e65100',
    marginBottom: 4,
  },
});

export default DeviceSelectionScreen;
