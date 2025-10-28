import React from 'react';
import {
  SafeAreaView,
  StatusBar,
  StyleSheet,
} from 'react-native';
import {NavigationContainer} from '@react-navigation/native';
import {createStackNavigator} from '@react-navigation/stack';

import HomeScreen from './src/screens/HomeScreen';
import ChatScreen from './src/screens/ChatScreen';
import DeviceSelectionScreen from './src/screens/DeviceSelectionScreen';

const Stack = createStackNavigator();

function App(): JSX.Element {
  return (
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
              title: route.params?.deviceName || 'Chat',
              headerStyle: {backgroundColor: '#4285F4'},
              headerTintColor: '#fff',
            })}
          />
        </Stack.Navigator>
      </SafeAreaView>
    </NavigationContainer>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#ffffff',
  },
});

export default App;