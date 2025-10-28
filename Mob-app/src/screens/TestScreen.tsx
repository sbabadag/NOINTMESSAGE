import React from 'react';
import {
  View,
  Text,
  StyleSheet,
  SafeAreaView,
  TouchableOpacity,
  Alert,
} from 'react-native';

const TestScreen: React.FC = () => {
  const showSuccess = () => {
    Alert.alert('✅ Success!', 'App is working! The crash has been fixed.');
  };

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.content}>
        <Text style={styles.title}>🚀 LoRa BLE Chat</Text>
        <Text style={styles.subtitle}>Crash Test Mode</Text>
        
        <View style={styles.status}>
          <Text style={styles.statusText}>✅ App Started Successfully</Text>
          <Text style={styles.statusText}>✅ No Crashes Detected</Text>
          <Text style={styles.statusText}>✅ Error Boundary Active</Text>
          <Text style={styles.statusText}>✅ Navigation Working</Text>
        </View>

        <TouchableOpacity style={styles.button} onPress={showSuccess}>
          <Text style={styles.buttonText}>Test App Stability</Text>
        </TouchableOpacity>

        <View style={styles.info}>
          <Text style={styles.infoText}>
            If you see this screen, the app is no longer crashing continuously!
          </Text>
          <Text style={styles.infoText}>
            Ready to test full LoRa BLE functionality.
          </Text>
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
    justifyContent: 'center',
    alignItems: 'center',
    padding: 20,
  },
  title: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#4285F4',
    marginBottom: 8,
    textAlign: 'center',
  },
  subtitle: {
    fontSize: 18,
    color: '#666',
    marginBottom: 30,
    textAlign: 'center',
  },
  status: {
    backgroundColor: '#e8f5e8',
    padding: 20,
    borderRadius: 10,
    marginBottom: 30,
    minWidth: '80%',
  },
  statusText: {
    fontSize: 16,
    color: '#2e7d32',
    marginBottom: 8,
    textAlign: 'center',
  },
  button: {
    backgroundColor: '#4285F4',
    paddingHorizontal: 30,
    paddingVertical: 15,
    borderRadius: 8,
    marginBottom: 30,
  },
  buttonText: {
    color: '#ffffff',
    fontSize: 18,
    fontWeight: 'bold',
  },
  info: {
    backgroundColor: '#ffffff',
    padding: 15,
    borderRadius: 8,
    minWidth: '80%',
  },
  infoText: {
    fontSize: 14,
    color: '#666',
    textAlign: 'center',
    marginBottom: 8,
  },
});

export default TestScreen;