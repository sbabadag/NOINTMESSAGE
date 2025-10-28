import React from 'react';
import {
  SafeAreaView,
  StatusBar,
  StyleSheet,
  Text,
  View,
  TouchableOpacity,
} from 'react-native';
import {NavigationContainer} from '@react-navigation/native';
import {createStackNavigator} from '@react-navigation/stack';

// Safe imports with error handling
let HomeScreen: any = null;
let ChatScreen: any = null; 
let DeviceSelectionScreen: any = null;

try {
  HomeScreen = require('./src/screens/HomeScreen').default;
  ChatScreen = require('./src/screens/ChatScreen').default;
  DeviceSelectionScreen = require('./src/screens/DeviceSelectionScreen').default;
} catch (error) {
  console.error('❌ Failed to load screens:', error);
}

const Stack = createStackNavigator();

// Enhanced Error Boundary Component with recovery
class ErrorBoundary extends React.Component<{children: React.ReactNode}, {hasError: boolean; error: string; errorInfo: string}> {
  constructor(props: {children: React.ReactNode}) {
    super(props);
    this.state = {hasError: false, error: '', errorInfo: ''};
  }

  static getDerivedStateFromError(error: Error) {
    console.error('🚨 App Error Boundary caught:', error);
    return {hasError: true, error: error.message, errorInfo: error.stack || ''};
  }

  componentDidCatch(error: Error, errorInfo: any) {
    console.error('🚨 Full App Crash Details:', error, errorInfo);
    console.error('🚨 Component Stack:', errorInfo.componentStack);
  }

  handleRestart = () => {
    console.log('🔄 Attempting to recover from error...');
    this.setState({hasError: false, error: '', errorInfo: ''});
  };

  render() {
    if (this.state.hasError) {
      return (
        <SafeAreaView style={styles.errorContainer}>
          <View style={styles.errorContent}>
            <Text style={styles.errorTitle}>🚨 App Crashed</Text>
            <Text style={styles.errorMessage}>
              The app crashed but we caught it! Details:
            </Text>
            <Text style={styles.errorDetails}>{this.state.error}</Text>
            
            <TouchableOpacity 
              style={styles.restartButton} 
              onPress={this.handleRestart}
            >
              <Text style={styles.restartButtonText}>🔄 Try Again</Text>
            </TouchableOpacity>
            
            <Text style={styles.helpText}>
              If this keeps happening:{'\n'}
              • Close and reopen the app{'\n'}
              • Check ESP32 is powered on{'\n'}
              • Enable Bluetooth and Location
            </Text>
          </View>
        </SafeAreaView>
      );
    }
    return this.props.children;
  }
}

function App(): JSX.Element {
  // Safety check for screen components
  if (!HomeScreen || !ChatScreen || !DeviceSelectionScreen) {
    return (
      <SafeAreaView style={styles.errorContainer}>
        <View style={styles.errorContent}>
          <Text style={styles.errorTitle}>🚨 Loading Error</Text>
          <Text style={styles.errorMessage}>
            Failed to load app screens. Please restart the app.
          </Text>
        </View>
      </SafeAreaView>
    );
  }

  return (
    <ErrorBoundary>
      <NavigationContainer
        onStateChange={(state) => {
          console.log('📱 Navigation state changed:', state?.routeNames);
        }}
        fallback={
          <SafeAreaView style={styles.errorContainer}>
            <Text style={styles.errorTitle}>Navigation Loading...</Text>
          </SafeAreaView>
        }
      >
        <StatusBar barStyle="dark-content" />
        <SafeAreaView style={styles.container}>
          <Stack.Navigator 
            initialRouteName="Home"
            screenOptions={{
              headerBackTitleVisible: false,
            }}
          >
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
              options={({route}) => {
                const params = route.params as any;
                return {
                  title: params?.deviceName || 'Chat',
                  headerStyle: {backgroundColor: '#4285F4'},
                  headerTintColor: '#fff',
                };
              }}
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
    marginBottom: 15,
  },
  restartButton: {
    backgroundColor: '#4285F4',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 5,
    marginBottom: 15,
  },
  restartButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  helpText: {
    fontSize: 12,
    color: '#666',
    textAlign: 'center',
    lineHeight: 16,
  },
});

export default App;