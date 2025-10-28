import React, {useState, useEffect, useRef} from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  FlatList,
  StyleSheet,
  Alert,
  KeyboardAvoidingView,
  Platform,
} from 'react-native';
import {NavigationProp, RouteProp} from '@react-navigation/native';
import {BLEService} from '../services/BLEService';
import {ChatMessage} from '../types';

interface Props {
  navigation: NavigationProp<any>;
  route: RouteProp<any, any>;
}

const ChatScreen: React.FC<Props> = ({navigation, route}) => {
  const [messages, setMessages] = useState<ChatMessage[]>([]);
  const [inputText, setInputText] = useState('');
  const [isConnected, setIsConnected] = useState(true);
  const [connectionStatus, setConnectionStatus] = useState('Connected');
  const flatListRef = useRef<FlatList>(null);
  
  const deviceName = route.params?.deviceName || 'Unknown Device';
  const bleService = route.params?.bleService as BLEService;
  const stationType = bleService?.getConnectedStationType();
  const remotStationType = stationType === 'M1' ? 'M2' : 'M1';

  useEffect(() => {
    try {
      // Set up message listener with error handling
      if (bleService && bleService.setMessageCallback) {
        bleService.setMessageCallback((message: ChatMessage) => {
          try {
            setMessages((prevMessages) => [...prevMessages, message]);
            // Auto scroll to bottom
            setTimeout(() => {
              flatListRef.current?.scrollToEnd({animated: true});
            }, 100);
          } catch (error) {
            console.error('❌ Error handling message:', error);
          }
        });
      }

      // Set up status listener with error handling
      if (bleService && bleService.setStatusCallback) {
        bleService.setStatusCallback((status: string) => {
          try {
            setConnectionStatus(status);
            console.log('📊 Connection status update:', status);
          } catch (error) {
            console.error('❌ Error handling status:', error);
          }
        });
      }

      // Add welcome message
      const welcomeMessage: ChatMessage = {
        id: 'welcome',
        text: `📡 Connected to ${deviceName} Station!\n\n💬 Messages you send will be transmitted via LoRa to ${remotStationType} Station.\n\n📨 Messages from ${remotStationType} Station will appear here.`,
        timestamp: new Date(),
        fromDevice: 'System',
        deviceId: 0,
        isOutgoing: false,
      };
      setMessages([welcomeMessage]);
    } catch (error) {
      console.error('❌ Error in ChatScreen useEffect:', error);
    }

    return () => {
      try {
        if (bleService && bleService.disconnect) {
          bleService.disconnect();
        }
      } catch (error) {
        console.error('❌ Error disconnecting:', error);
      }
    };
  }, []);

  const sendMessage = async () => {
    if (!inputText.trim()) return;

    const messageText = inputText.trim();
    setInputText('');

    try {
      if (!bleService) {
        Alert.alert('Error', 'BLE service not available');
        return;
      }

      const success = await bleService.sendMessage(messageText);
      if (!success) {
        Alert.alert('Send Failed', 'Could not send message. Check LoRa connection.');
      }
    } catch (error) {
      console.error('❌ Send message error:', error);
      Alert.alert('Send Error', 'An error occurred while sending the message.');
      
      // Add the message back to input if it failed
      setInputText(messageText);
    }
  };

  const formatTime = (date: Date): string => {
    return date.toLocaleTimeString([], {hour: '2-digit', minute: '2-digit'});
  };

  const renderMessage = ({item}: {item: ChatMessage}) => (
    <View style={[
      styles.messageContainer,
      item.isOutgoing ? styles.outgoingMessage : styles.incomingMessage
    ]}>
      <View style={[
        styles.messageBubble,
        item.isOutgoing ? styles.outgoingBubble : styles.incomingBubble,
        item.fromDevice === 'System' && styles.systemBubble
      ]}>
        {/* Station indicator for non-system messages */}
        {item.stationType && item.fromDevice !== 'System' && (
          <View style={styles.stationHeader}>
            <Text style={styles.stationText}>
              {item.isOutgoing ? `${stationType} → ${remotStationType}` : `${item.stationType} Station`}
            </Text>
          </View>
        )}
        
        <Text style={[
          styles.messageText,
          item.isOutgoing ? styles.outgoingText : styles.incomingText,
          item.fromDevice === 'System' && styles.systemText
        ]}>
          {item.text}
        </Text>
        
        <View style={styles.messageInfo}>
          <Text style={[
            styles.messageTime,
            item.isOutgoing ? styles.outgoingTime : styles.incomingTime,
            item.fromDevice === 'System' && styles.systemTime
          ]}>
            {formatTime(item.timestamp)}
          </Text>
          
          {!item.isOutgoing && item.fromDevice !== 'System' && item.fromDevice !== 'You' && (
            <Text style={styles.deviceInfo}>
              📡 Via LoRa from {item.fromDevice}
              {item.rssi && ` • Signal: ${item.rssi}dBm`}
              {item.snr && ` • SNR: ${item.snr}dB`}
            </Text>
          )}
        </View>
      </View>
    </View>
  );

  return (
    <KeyboardAvoidingView 
      style={styles.container}
      behavior={Platform.OS === 'ios' ? 'padding' : 'height'}
    >
      <View style={styles.header}>
        <View style={styles.deviceInfo}>
          <Text style={styles.deviceName}>{deviceName} Station</Text>
          <Text style={styles.tunnelInfo}>
            📡 LoRa Tunnel: {stationType} ↔ {remotStationType}
          </Text>
        </View>
        <Text style={styles.connectionStatus}>
          {isConnected ? '🟢 Connected' : '🔴 Disconnected'}
        </Text>
      </View>

      <FlatList
        ref={flatListRef}
        data={messages}
        keyExtractor={(item) => item.id}
        renderItem={renderMessage}
        style={styles.messagesList}
        showsVerticalScrollIndicator={false}
        onContentSizeChange={() => flatListRef.current?.scrollToEnd({animated: true})}
      />

      <View style={styles.inputContainer}>
        <TextInput
          style={styles.textInput}
          value={inputText}
          onChangeText={setInputText}
          placeholder="Type your message..."
          placeholderTextColor="#999"
          multiline
          maxLength={200}
        />
        <TouchableOpacity 
          style={[styles.sendButton, !inputText.trim() && styles.sendButtonDisabled]}
          onPress={sendMessage}
          disabled={!inputText.trim()}
        >
          <Text style={styles.sendButtonText}>Send</Text>
        </TouchableOpacity>
      </View>
    </KeyboardAvoidingView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  header: {
    paddingTop: 10,
    paddingHorizontal: 20,
    paddingBottom: 15,
    backgroundColor: '#4285F4',
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  deviceName: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#ffffff',
  },
  connectionStatus: {
    fontSize: 14,
    color: '#e3f2fd',
  },
  messagesList: {
    flex: 1,
    paddingHorizontal: 16,
    paddingTop: 16,
  },
  messageContainer: {
    marginBottom: 12,
  },
  outgoingMessage: {
    alignItems: 'flex-end',
  },
  incomingMessage: {
    alignItems: 'flex-start',
  },
  messageBubble: {
    maxWidth: '80%',
    paddingHorizontal: 16,
    paddingVertical: 12,
    borderRadius: 20,
  },
  outgoingBubble: {
    backgroundColor: '#4285F4',
  },
  incomingBubble: {
    backgroundColor: '#ffffff',
    borderWidth: 1,
    borderColor: '#e0e0e0',
  },
  messageText: {
    fontSize: 16,
    lineHeight: 20,
  },
  outgoingText: {
    color: '#ffffff',
  },
  incomingText: {
    color: '#333',
  },
  messageInfo: {
    marginTop: 4,
  },
  messageTime: {
    fontSize: 12,
  },
  outgoingTime: {
    color: '#e3f2fd',
    textAlign: 'right',
  },
  incomingTime: {
    color: '#999',
  },
  deviceInfo: {
    fontSize: 10,
    color: '#666',
    marginTop: 2,
  },
  inputContainer: {
    flexDirection: 'row',
    paddingHorizontal: 16,
    paddingVertical: 12,
    backgroundColor: '#ffffff',
    alignItems: 'flex-end',
    borderTopWidth: 1,
    borderTopColor: '#e0e0e0',
  },
  textInput: {
    flex: 1,
    borderWidth: 1,
    borderColor: '#e0e0e0',
    borderRadius: 20,
    paddingHorizontal: 16,
    paddingVertical: 12,
    fontSize: 16,
    maxHeight: 100,
    marginRight: 12,
  },
  sendButton: {
    backgroundColor: '#4285F4',
    paddingHorizontal: 20,
    paddingVertical: 12,
    borderRadius: 20,
  },
  sendButtonDisabled: {
    backgroundColor: '#ccc',
  },
  sendButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  systemBubble: {
    backgroundColor: '#f0f0f0',
    borderWidth: 1,
    borderColor: '#ddd',
  },
  systemText: {
    color: '#666',
    fontStyle: 'italic',
  },
  systemTime: {
    color: '#999',
  },
  stationHeader: {
    marginBottom: 4,
    paddingBottom: 4,
    borderBottomWidth: 1,
    borderBottomColor: 'rgba(255,255,255,0.2)',
  },
  stationText: {
    fontSize: 11,
    fontWeight: 'bold',
    color: '#e3f2fd',
    textTransform: 'uppercase',
  },
  tunnelInfo: {
    fontSize: 12,
    color: '#e3f2fd',
    marginTop: 2,
  },
});

export default ChatScreen;