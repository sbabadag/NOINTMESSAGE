import React from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  SafeAreaView,
} from 'react-native';
import { NavigationProp } from '@react-navigation/native';

interface Props {
  navigation: NavigationProp<any>;
}

const HomeScreen: React.FC<Props> = ({ navigation }) => {
  const startChat = () => {
    navigation.navigate('DeviceSelection');
  };

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.content}>
        <Text style={styles.title}> LoRa BLE Chat</Text>
        <Text style={styles.subtitle}>
          Long-Range Messaging via ESP32 + SX1262
        </Text>
        
        <View style={styles.infoBox}>
          <Text style={styles.infoTitle}> How it works:</Text>
          <Text style={styles.infoText}> Connect to ESP32 M1 or M2 station via Bluetooth</Text>
          <Text style={styles.infoText}> Messages sent via LoRa to remote station</Text>
          <Text style={styles.infoText}> Chat between two phones over long distances</Text>
        </View>

        <TouchableOpacity style={styles.button} onPress={startChat}>
          <Text style={styles.buttonText}> Find LoRa Stations</Text>
        </TouchableOpacity>

        <View style={styles.statusBox}>
          <Text style={styles.statusText}> Fresh app - No crashes</Text>
          <Text style={styles.statusText}> Ready for LoRa communication</Text>
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
    justifyContent: 'center',
  },
  title: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#4285F4',
    textAlign: 'center',
    marginBottom: 8,
  },
  subtitle: {
    fontSize: 16,
    color: '#666',
    textAlign: 'center',
    marginBottom: 40,
  },
  infoBox: {
    backgroundColor: '#e3f2fd',
    padding: 20,
    borderRadius: 10,
    marginBottom: 30,
  },
  infoTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#1976d2',
    marginBottom: 10,
  },
  infoText: {
    fontSize: 14,
    color: '#424242',
    marginBottom: 5,
  },
  button: {
    backgroundColor: '#4285F4',
    padding: 16,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 30,
  },
  buttonText: {
    color: '#ffffff',
    fontSize: 18,
    fontWeight: 'bold',
  },
  statusBox: {
    backgroundColor: '#e8f5e8',
    padding: 15,
    borderRadius: 8,
    alignItems: 'center',
  },
  statusText: {
    fontSize: 14,
    color: '#2e7d32',
    marginBottom: 5,
  },
});

export default HomeScreen;
