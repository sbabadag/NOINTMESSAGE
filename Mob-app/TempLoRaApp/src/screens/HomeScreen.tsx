import React from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  Image,
  Alert,
} from 'react-native';
import {NavigationProp} from '@react-navigation/native';

interface Props {
  navigation: NavigationProp<any>;
}

const HomeScreen: React.FC<Props> = ({navigation}) => {
  const handleStartChat = () => {
    navigation.navigate('DeviceSelection');
  };

  const showAbout = () => {
    Alert.alert(
      'About LoRa BLE Chat',
      'Connect to LoRa devices via Bluetooth and send messages over long-range radio networks.\n\nSupported devices:\n• LoRa_Station_1\n• LoRa_Station_2',
      [{text: 'OK'}]
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>LoRa BLE Chat</Text>
        <Text style={styles.subtitle}>
          Long-Range Messaging via Bluetooth
        </Text>
      </View>

      <View style={styles.content}>
        <View style={styles.iconContainer}>
          <Text style={styles.icon}>📡</Text>
        </View>

        <Text style={styles.description}>
          Connect to LoRa devices via Bluetooth and send messages across long distances without internet connectivity.
        </Text>

        <TouchableOpacity style={styles.primaryButton} onPress={handleStartChat}>
          <Text style={styles.primaryButtonText}>Start Messaging</Text>
        </TouchableOpacity>

        <TouchableOpacity style={styles.secondaryButton} onPress={showAbout}>
          <Text style={styles.secondaryButtonText}>About</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.footer}>
        <Text style={styles.footerText}>
          Make sure your LoRa devices are powered on and nearby
        </Text>
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
    paddingTop: 40,
    paddingHorizontal: 20,
    paddingBottom: 20,
    backgroundColor: '#4285F4',
  },
  title: {
    fontSize: 28,
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
    paddingTop: 40,
    alignItems: 'center',
  },
  iconContainer: {
    marginBottom: 30,
  },
  icon: {
    fontSize: 80,
  },
  description: {
    fontSize: 16,
    color: '#666',
    textAlign: 'center',
    lineHeight: 24,
    marginBottom: 40,
    paddingHorizontal: 20,
  },
  primaryButton: {
    backgroundColor: '#4285F4',
    paddingVertical: 16,
    paddingHorizontal: 32,
    borderRadius: 8,
    marginBottom: 16,
    minWidth: 200,
  },
  primaryButtonText: {
    color: '#ffffff',
    fontSize: 18,
    fontWeight: 'bold',
    textAlign: 'center',
  },
  secondaryButton: {
    backgroundColor: 'transparent',
    paddingVertical: 12,
    paddingHorizontal: 32,
    borderRadius: 8,
    borderWidth: 2,
    borderColor: '#4285F4',
    minWidth: 200,
  },
  secondaryButtonText: {
    color: '#4285F4',
    fontSize: 16,
    fontWeight: '600',
    textAlign: 'center',
  },
  footer: {
    paddingHorizontal: 20,
    paddingBottom: 20,
  },
  footerText: {
    fontSize: 14,
    color: '#999',
    textAlign: 'center',
    fontStyle: 'italic',
  },
});

export default HomeScreen;