import React, { useState, useEffect, useRef } from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  SafeAreaView,
  FlatList,
  Alert,
  KeyboardAvoidingView,
  Platform,
  Keyboard,
} from 'react-native';
import { RouteProp } from '@react-navigation/native';
import { NavigationProp } from '@react-navigation/native';
import { ChatMessage } from '../types';
import { BLEService } from '../services/BLEService';

interface Props {
  route: RouteProp<any, any>;
  navigation: NavigationProp<any>;
}

const ChatScreen: React.FC<Props> = ({ route, navigation }) => {
  const { deviceName, deviceId, stationType, bleService } = route.params as {
    deviceName: string;
    deviceId: string;
    stationType: string;
    bleService: BLEService;
  };

  const [messages, setMessages] = useState<ChatMessage[]>([]);
  const [inputText, setInputText] = useState('');
  const [isConnected, setIsConnected] = useState(true);
  const [isSending, setIsSending] = useState(false);
  const flatListRef = useRef<FlatList>(null);
  const MAX_RETRY_ATTEMPTS = 3;

  useEffect(() => {
    // Setup message listener for incoming LoRa messages
    const handleMessage = (message: ChatMessage) => {
      console.log(' Received message via LoRa:', message);
      setMessages(prev => [...prev, message]);
    };

    bleService.setMessageCallback(handleMessage);

    // Add welcome message
    const welcomeMessage: ChatMessage = {
      id: Date.now().toString(),
      text: `Connected to ${deviceName} (${stationType} station). You can now send messages through the LoRa tunnel! `,
      sender: 'system',
      timestamp: new Date(),
      isOwn: false,
    };
    setMessages([welcomeMessage]);

    // Cleanup on unmount
    return () => {
      console.log(' Cleaning up chat screen...');
      bleService.disconnect();
    };
  }, []);

  const sendMessage = async () => {
    if (!inputText.trim()) return;
    if (isSending) return;

    setIsSending(true);
    const messageText = inputText.trim();
    const messageId = Date.now().toString();

    // Add outgoing message to UI immediately
    const outgoingMessage: ChatMessage = {
      id: messageId,
      text: messageText,
      sender: 'user',
      timestamp: new Date(),
      isOwn: true,
    };
    setMessages(prev => [...prev, outgoingMessage]);
    setInputText('');

    // Scroll to bottom after adding message
    setTimeout(() => {
      flatListRef.current?.scrollToEnd({ animated: true });
    }, 100);

    // Retry logic
    let attempt = 0;
    let success = false;

    while (attempt < MAX_RETRY_ATTEMPTS && !success) {
      attempt++;
      try {
        console.log(`📤 Sending message via LoRa (Attempt ${attempt}/${MAX_RETRY_ATTEMPTS}):`, messageText);
        success = await bleService.sendMessage(messageText, messageId);
        
        if (!success && attempt < MAX_RETRY_ATTEMPTS) {
          console.log(`⚠️ Attempt ${attempt} failed, retrying...`);
          // Wait 1 second before retrying
          await new Promise(resolve => setTimeout(resolve, 1000));
        }
      } catch (error) {
        console.error(`❌ Send message error (Attempt ${attempt}):`, error);
        if (attempt >= MAX_RETRY_ATTEMPTS) {
          // Show error message after all retries failed
          const errorMessage: ChatMessage = {
            id: (Date.now() + 1).toString(),
            text: `❌ Failed to send message after ${MAX_RETRY_ATTEMPTS} attempts. Please check BLE connection.`,
            sender: 'system',
            timestamp: new Date(),
            isOwn: false,
          };
          setMessages(prev => [...prev, errorMessage]);
          Alert.alert(
            'Send Failed', 
            `Message could not be sent after ${MAX_RETRY_ATTEMPTS} attempts. Please check your connection.`
          );
        }
      }
    }

    if (success) {
      console.log('✅ Message sent successfully!');
    }

    setIsSending(false);
  };

  const renderMessage = ({ item }: { item: ChatMessage }) => (
    <View style={[
      styles.messageContainer,
      item.isOwn ? styles.ownMessage : styles.otherMessage
    ]}>
      <View style={[
        styles.messageBubble,
        item.isOwn ? styles.ownBubble : styles.otherBubble,
        item.sender === 'system' && styles.systemBubble
      ]}>
        <Text style={[
          styles.messageText,
          item.isOwn ? styles.ownText : styles.otherText,
          item.sender === 'system' && styles.systemText
        ]}>
          {item.text}
        </Text>
        <Text style={[
          styles.timestamp,
          item.isOwn ? styles.ownTimestamp : styles.otherTimestamp
        ]}>
          {item.timestamp.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}
        </Text>
      </View>
    </View>
  );

  return (
    <SafeAreaView style={styles.container}>
      <KeyboardAvoidingView 
        style={styles.container} 
        behavior={Platform.OS === 'ios' ? 'padding' : 'height'}
        keyboardVerticalOffset={Platform.OS === 'ios' ? 0 : 0}
      >
      {/* Header */}
      <View style={styles.header}>
        <TouchableOpacity onPress={() => navigation.goBack()} style={styles.backButton}>
          <Text style={styles.backText}>⬅ Back</Text>
        </TouchableOpacity>
        <View style={styles.headerInfo}>
          <Text style={styles.deviceName}>📡 {deviceName}</Text>
          <Text style={styles.connectionStatus}>
            {isConnected ? `✅ Connected (${stationType})` : '❌ Disconnected'}
          </Text>
        </View>
      </View>

      {/* LoRa Tunnel Indicator */}
      <View style={styles.tunnelIndicator}>
        <Text style={styles.tunnelText}>
          📱 Phone ↔ BLE ↔ {stationType} ↔ LoRa Tunnel ↔ Remote ↔ BLE ↔ Other Phone 📱
        </Text>
      </View>

      {/* Messages */}
      <FlatList
        ref={flatListRef}
        data={messages}
        renderItem={renderMessage}
        keyExtractor={(item) => item.id}
        style={styles.messagesList}
        contentContainerStyle={styles.messagesContent}
        showsVerticalScrollIndicator={false}
        inverted={false}
        onContentSizeChange={() => flatListRef.current?.scrollToEnd({ animated: true })}
      />

      {/* Input */}
      <View style={styles.inputContainer}>
        <TextInput
          style={styles.textInput}
          value={inputText}
          onChangeText={setInputText}
          placeholder="Type your message..."
          placeholderTextColor="#999"
          multiline={true}
          maxLength={200}
          returnKeyType="send"
          onSubmitEditing={() => {
            if (inputText.trim() && !isSending) {
              sendMessage();
            }
          }}
          blurOnSubmit={false}
        />
        <TouchableOpacity 
          style={[styles.sendButton, isSending && styles.sendingButton]} 
          onPress={sendMessage}
          disabled={!inputText.trim() || isSending}
        >
          <Text style={styles.sendButtonText}>
            {isSending ? '⏳' : '➤'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Status Bar */}
      <View style={styles.statusBar}>
        <Text style={styles.statusText}>
          LoRa Status: {isConnected ? '🟢 Tunnel Active' : '🔴 Tunnel Down'} | 
          Range: ~10km | Encryption: AES-256
        </Text>
      </View>
      </KeyboardAvoidingView>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 15,
    backgroundColor: '#4285F4',
  },
  backButton: {
    marginRight: 15,
  },
  backText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  headerInfo: {
    flex: 1,
  },
  deviceName: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#ffffff',
  },
  connectionStatus: {
    fontSize: 14,
    color: '#E3F2FD',
  },
  tunnelIndicator: {
    backgroundColor: '#E8F5E8',
    padding: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#ddd',
  },
  tunnelText: {
    fontSize: 12,
    color: '#2E7D32',
    textAlign: 'center',
  },
  messagesList: {
    flex: 1,
    padding: 15,
  },
  messagesContent: {
    flexGrow: 1,
  },
  messageContainer: {
    marginBottom: 10,
  },
  ownMessage: {
    alignItems: 'flex-end',
  },
  otherMessage: {
    alignItems: 'flex-start',
  },
  messageBubble: {
    maxWidth: '80%',
    padding: 12,
    borderRadius: 18,
  },
  ownBubble: {
    backgroundColor: '#4285F4',
  },
  otherBubble: {
    backgroundColor: '#ffffff',
    elevation: 1,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.1,
    shadowRadius: 2,
  },
  systemBubble: {
    backgroundColor: '#FFF3E0',
    maxWidth: '95%',
  },
  messageText: {
    fontSize: 16,
    lineHeight: 20,
  },
  ownText: {
    color: '#ffffff',
  },
  otherText: {
    color: '#333',
  },
  systemText: {
    color: '#E65100',
    fontStyle: 'italic',
  },
  timestamp: {
    fontSize: 12,
    marginTop: 4,
  },
  ownTimestamp: {
    color: '#E3F2FD',
  },
  otherTimestamp: {
    color: '#999',
  },
  inputContainer: {
    flexDirection: 'row',
    padding: 15,
    backgroundColor: '#ffffff',
    alignItems: 'flex-end',
  },
  textInput: {
    flex: 1,
    borderWidth: 1,
    borderColor: '#ddd',
    borderRadius: 20,
    paddingHorizontal: 15,
    paddingVertical: 10,
    marginRight: 10,
    fontSize: 16,
    maxHeight: 100,
  },
  sendButton: {
    backgroundColor: '#4285F4',
    width: 44,
    height: 44,
    borderRadius: 22,
    alignItems: 'center',
    justifyContent: 'center',
  },
  sendingButton: {
    backgroundColor: '#90CAF9',
  },
  sendButtonText: {
    fontSize: 18,
  },
  statusBar: {
    backgroundColor: '#E8F5E8',
    padding: 8,
    alignItems: 'center',
  },
  statusText: {
    fontSize: 12,
    color: '#2E7D32',
  },
});

export default ChatScreen;
