import React from 'react';
import {
  SafeAreaView,
  StatusBar,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import {NavigationContainer} from '@react-navigation/native';
import {createStackNavigator} from '@react-navigation/stack';

import HomeScreen from './src/screens/HomeScreen';
import ChatScreen from './src/screens/ChatScreen';
import DeviceSelectionScreen from './src/screens/DeviceSelectionScreen';

const Stack = createStackNavigator();

// Error Boundary Component for crash prevention
class ErrorBoundary extends React.Component<{children: React.ReactNode}, {hasError: boolean; error: string}> {
  constructor(props: {children: React.ReactNode}) {
    super(props);
    this.state = {hasError: false, error: ''};
  }

  static getDerivedStateFromError(error: Error) {
    return {hasError: true, error: error.message};
  }

  componentDidCatch(error: Error, errorInfo: any) {
    console.error('App crashed:', error, errorInfo);
  }

  render() {
    if (this.state.hasError) {
      return (
        <SafeAreaView style={styles.errorContainer}>
          <View style={styles.errorContent}>
            <Text style={styles.errorTitle}>🚨 App Error</Text>
            <Text style={styles.errorMessage}>
              The app encountered an error. Please restart the app.
            </Text>
            <Text style={styles.errorDetails}>{this.state.error}</Text>
          </View>
        </SafeAreaView>
      );
    }
    return this.props.children;
  }
}

function App(): JSX.Element {
  return (
    <ErrorBoundary>
      <NavigationContainer>
        <StatusBar barStyle="dark-content" />
        <SafeAreaView style={styles.container}>
          <Stack.Navigator initialRouteName="Home">
            <Stack.Screen
              name="Home"
              component={HomeScreen}
              options={{
                title: 'LoRa BLE Chat',
                headerStyle: {backgroundColor: '#4285F4'},
                headerTintColor: '#fff',
              }}
            />
            <Stack.Screen
              name="DeviceSelection"
              component={DeviceSelectionScreen}
              options={{
                title: 'Select LoRa Device',
                headerStyle: {backgroundColor: '#4285F4'},
                headerTintColor: '#fff',
              }}
            />
            <Stack.Screen
              name="Chat"
              component={ChatScreen}
              options={({route}) => ({
                title: (route.params as any)?.deviceName || 'Chat',
                headerStyle: {backgroundColor: '#4285F4'},
                headerTintColor: '#fff',
              })}
            />
          </Stack.Navigator>
        </SafeAreaView>
      </NavigationContainer>
    </ErrorBoundary>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#ffffff',
  },
  errorContainer: {
    flex: 1,
    backgroundColor: '#ffebee',
    justifyContent: 'center',
    alignItems: 'center',
    padding: 20,
  },
  errorContent: {
    backgroundColor: '#ffffff',
    padding: 20,
    borderRadius: 10,
    maxWidth: '90%',
    alignItems: 'center',
  },
  errorTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#d32f2f',
    marginBottom: 10,
    textAlign: 'center',
  },
  errorMessage: {
    fontSize: 16,
    color: '#666',
    marginBottom: 10,
    textAlign: 'center',
  },
  errorDetails: {
    fontSize: 12,
    color: '#999',
    textAlign: 'center',
  },
});

export default App;