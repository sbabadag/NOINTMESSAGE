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
  const flatListRef = useRef<FlatList>(null);
  
  const {deviceName, bleService}: {deviceName: string; bleService: BLEService} = route.params;

  useEffect(() => {
    // Set up message listener
    bleService.setMessageCallback((message: ChatMessage) => {
      setMessages((prevMessages) => [...prevMessages, message]);
      // Auto scroll to bottom
      setTimeout(() => {
        flatListRef.current?.scrollToEnd({animated: true});
      }, 100);
    });

    // Add welcome message
    const welcomeMessage: ChatMessage = {
      id: 'welcome',
      text: `Connected to ${deviceName}. You can now send messages via LoRa!`,
      timestamp: new Date(),
      fromDevice: 'System',
      deviceId: 0,
      isOutgoing: false,
    };
    setMessages([welcomeMessage]);

    return () => {
      bleService.disconnect();
    };
  }, []);

  const sendMessage = async () => {
    if (!inputText.trim()) return;

    const messageText = inputText.trim();
    setInputText('');

    try {
      const success = await bleService.sendMessage(messageText);
      if (!success) {
        Alert.alert('Error', 'Failed to send message. Please check your connection.');
      }
    } catch (error) {
      Alert.alert('Error', 'An error occurred while sending the message.');
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
        item.isOutgoing ? styles.outgoingBubble : styles.incomingBubble
      ]}>
        <Text style={[
          styles.messageText,
          item.isOutgoing ? styles.outgoingText : styles.incomingText
        ]}>
          {item.text}
        </Text>
        
        <View style={styles.messageInfo}>
          <Text style={[
            styles.messageTime,
            item.isOutgoing ? styles.outgoingTime : styles.incomingTime
          ]}>
            {formatTime(item.timestamp)}
          </Text>
          
          {!item.isOutgoing && item.fromDevice !== 'System' && (
            <Text style={styles.deviceInfo}>
              From: {item.fromDevice}
              {item.rssi && ` • ${item.rssi}dBm`}
              {item.snr && ` • SNR ${item.snr}dB`}
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
        <Text style={styles.deviceName}>{deviceName}</Text>
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
});

export default ChatScreen;