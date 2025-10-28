import React, {useState, useEffect} from 'react';
import {
  View,
  Text,
  FlatList,
  TouchableOpacity,
  StyleSheet,
  Alert,
  ActivityIndicator,
} from 'react-native';
import {NavigationProp} from '@react-navigation/native';
import {BLEService} from '../services/BLEService';
import {LoRaDevice} from '../types';

interface Props {
  navigation: NavigationProp<any>;
}

const DeviceSelectionScreen: React.FC<Props> = ({navigation}) => {
  const [devices, setDevices] = useState<LoRaDevice[]>([]);
  const [scanning, setScanning] = useState(false);
  const [bleService] = useState(new BLEService());

  useEffect(() => {
    initializeBLE();
    return () => {
      bleService.destroy();
    };
  }, []);

  const initializeBLE = async () => {
    try {
      await bleService.initialize();
      startScan();
    } catch (error) {
      Alert.alert('Error', 'Failed to initialize Bluetooth. Please check if Bluetooth is enabled.');
    }
  };

  const startScan = async () => {
    setScanning(true);
    setDevices([]);
    
    try {
      const foundDevices = await bleService.scanForDevices();
      setDevices(foundDevices);
      
      if (foundDevices.length === 0) {
        Alert.alert(
          'No Devices Found',
          'No LoRa devices found. Make sure your devices are powered on and nearby.',
          [
            {text: 'Scan Again', onPress: startScan},
            {text: 'Cancel', style: 'cancel'},
          ]
        );
      }
    } catch (error) {
      Alert.alert('Error', 'Failed to scan for devices. Please try again.');
    } finally {
      setScanning(false);
    }
  };

  const connectToDevice = async (device: LoRaDevice) => {
    try {
      const connected = await bleService.connectToDevice(device.id);
      
      if (connected) {
        navigation.navigate('Chat', {
          deviceId: device.id,
          deviceName: device.name,
          bleService: bleService,
        });
      } else {
        Alert.alert('Connection Failed', 'Could not connect to the device. Please try again.');
      }
    } catch (error) {
      Alert.alert('Error', 'An error occurred while connecting to the device.');
    }
  };

  const renderDevice = ({item}: {item: LoRaDevice}) => (
    <TouchableOpacity
      style={styles.deviceItem}
      onPress={() => connectToDevice(item)}
    >
      <View style={styles.deviceInfo}>
        <Text style={styles.deviceName}>{item.name}</Text>
        <Text style={styles.deviceId}>ID: {item.id.substring(0, 8)}...</Text>
        {item.rssi && (
          <Text style={styles.deviceRssi}>Signal: {item.rssi} dBm</Text>
        )}
      </View>
      <View style={styles.deviceStatus}>
        <Text style={styles.connectText}>Tap to Connect</Text>
        <Text style={styles.arrow}>→</Text>
      </View>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Select LoRa Device</Text>
        <Text style={styles.subtitle}>
          {scanning ? 'Scanning for devices...' : `Found ${devices.length} device(s)`}
        </Text>
      </View>

      <View style={styles.content}>
        {scanning ? (
          <View style={styles.loadingContainer}>
            <ActivityIndicator size="large" color="#4285F4" />
            <Text style={styles.loadingText}>Scanning for LoRa devices...</Text>
          </View>
        ) : (
          <>
            <FlatList
              data={devices}
              keyExtractor={(item) => item.id}
              renderItem={renderDevice}
              style={styles.deviceList}
              showsVerticalScrollIndicator={false}
            />
            
            <TouchableOpacity style={styles.scanButton} onPress={startScan}>
              <Text style={styles.scanButtonText}>Scan Again</Text>
            </TouchableOpacity>
          </>
        )}
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  header: {
    paddingTop: 20,
    paddingHorizontal: 20,
    paddingBottom: 20,
    backgroundColor: '#4285F4',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#ffffff',
    textAlign: 'center',
    marginBottom: 8,
  },
  subtitle: {
    fontSize: 16,
    color: '#e3f2fd',
    textAlign: 'center',
  },
  content: {
    flex: 1,
    paddingHorizontal: 20,
    paddingTop: 20,
  },
  loadingContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  loadingText: {
    marginTop: 16,
    fontSize: 16,
    color: '#666',
  },
  deviceList: {
    flex: 1,
  },
  deviceItem: {
    backgroundColor: '#ffffff',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    elevation: 2,
    shadowColor: '#000',
    shadowOffset: {width: 0, height: 2},
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
  deviceId: {
    fontSize: 14,
    color: '#666',
    marginBottom: 2,
  },
  deviceRssi: {
    fontSize: 12,
    color: '#4285F4',
    fontWeight: '600',
  },
  deviceStatus: {
    alignItems: 'center',
  },
  connectText: {
    fontSize: 14,
    color: '#4285F4',
    fontWeight: '600',
    marginBottom: 4,
  },
  arrow: {
    fontSize: 20,
    color: '#4285F4',
  },
  scanButton: {
    backgroundColor: '#4285F4',
    paddingVertical: 16,
    borderRadius: 8,
    marginTop: 16,
    marginBottom: 20,
  },
  scanButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: 'bold',
    textAlign: 'center',
  },
});

export default DeviceSelectionScreen;