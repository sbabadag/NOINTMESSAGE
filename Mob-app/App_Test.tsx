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

import TestScreen from './src/screens/TestScreen';

const Stack = createStackNavigator();

// Enhanced Error Boundary Component
class ErrorBoundary extends React.Component<{children: React.ReactNode}, {hasError: boolean; error: string}> {
  constructor(props: {children: React.ReactNode}) {
    super(props);
    this.state = {hasError: false, error: ''};
  }

  static getDerivedStateFromError(error: Error) {
    console.error('🚨 App Error Boundary caught:', error);
    return {hasError: true, error: error.message};
  }

  componentDidCatch(error: Error, errorInfo: any) {
    console.error('🚨 Full App Crash Details:', error, errorInfo);
  }

  handleRestart = () => {
    console.log('🔄 Attempting to recover from error...');
    this.setState({hasError: false, error: ''});
  };

  render() {
    if (this.state.hasError) {
      return (
        <SafeAreaView style={styles.errorContainer}>
          <View style={styles.errorContent}>
            <Text style={styles.errorTitle}>🚨 App Error Fixed</Text>
            <Text style={styles.errorMessage}>
              Error caught by boundary: {this.state.error}
            </Text>
            
            <TouchableOpacity 
              style={styles.restartButton} 
              onPress={this.handleRestart}
            >
              <Text style={styles.restartButtonText}>🔄 Try Again</Text>
            </TouchableOpacity>
          </View>
        </SafeAreaView>
      );
    }
    return this.props.children;
  }
}

function App(): JSX.Element {
  console.log('🚀 App starting...');
  
  return (
    <ErrorBoundary>
      <NavigationContainer
        onReady={() => console.log('✅ Navigation ready')}
        onStateChange={(state) => console.log('📱 Navigation state:', state?.type)}
      >
        <StatusBar barStyle="dark-content" />
        <SafeAreaView style={styles.container}>
          <Stack.Navigator initialRouteName="Test">
            <Stack.Screen
              name="Test"
              component={TestScreen}
              options={{
                title: 'Crash Fix Test',
                headerStyle: {backgroundColor: '#4285F4'},
                headerTintColor: '#fff',
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
    marginBottom: 15,
    textAlign: 'center',
  },
  restartButton: {
    backgroundColor: '#4285F4',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 5,
  },
  restartButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: 'bold',
  },
});

export default App;