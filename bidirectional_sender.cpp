// BIDIRECTIONAL MESSAGE TUNNEL - SENDER STATION
// Sends messages via Serial/LoRa AND receives messages from LoRa

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// Pin mapping for XIAO ESP32S3 + Wio SX1262
constexpr uint8_t PIN_LORA_NSS   = D7;  // GPIO44
constexpr uint8_t PIN_LORA_DIO1  = D1;  // GPIO2
constexpr uint8_t PIN_LORA_RESET = D0;  // GPIO1
constexpr uint8_t PIN_LORA_BUSY  = D2;  // GPIO3
constexpr uint8_t PIN_LORA_SCK   = D10; // GPIO9
constexpr uint8_t PIN_LORA_MISO  = D9;  // GPIO8
constexpr uint8_t PIN_LORA_MOSI  = D8;  // GPIO7

// LoRa Configuration (must match receiver)
constexpr float LORA_FREQ = 915.0;
constexpr float LORA_BW = 125.0;
constexpr uint8_t LORA_SF = 7;
constexpr uint8_t LORA_CR = 5;
constexpr int8_t LORA_POWER = 22;

// Message settings
constexpr uint16_t MAX_MESSAGE_LEN = 200;

SX1262 lora = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RESET, PIN_LORA_BUSY);
bool loraReady = false;
uint32_t messageCount = 0;

// Message packet structure (must match receiver)
struct MessagePacket {
  uint32_t timestamp;
  uint16_t messageLen;
  char message[MAX_MESSAGE_LEN];
} __attribute__((packed));

// Send message via LoRa
void sendMessage(const char* text) {
  if (!loraReady) {
    Serial.println("✗ LoRa not ready");
    return;
  }
  
  uint16_t len = strlen(text);
  if (len == 0 || len > MAX_MESSAGE_LEN) {
    Serial.println("✗ Message too long or empty");
    return;
  }
  
  MessagePacket packet;
  packet.timestamp = millis();
  packet.messageLen = len;
  strncpy(packet.message, text, MAX_MESSAGE_LEN);
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  📤 SENDING MESSAGE               ║");
  Serial.println("╚═══════════════════════════════════╝");
  Serial.printf("Message: \"%s\"\n", text);
  Serial.printf("Length: %d bytes\n", len);
  
  // Stop receiving to transmit
  lora.standby();
  
  // Transmit
  int16_t state = lora.transmit((uint8_t*)&packet, sizeof(uint32_t) + sizeof(uint16_t) + len);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✓ Message sent successfully!");
    Serial.printf("Time on air: %d ms\n", lora.getTimeOnAir(len + 6));
    Serial.printf("Data rate: %.2f bps\n", lora.getDataRate());
  } else {
    Serial.printf("✗ Transmission failed, error: %d\n", state);
  }
  
  Serial.println("═══════════════════════════════════\n");
  
  // Resume receiving
  lora.startReceive();
  Serial.print("> "); // Show prompt
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  BIDIRECTIONAL MESSAGE TUNNEL     ║");
  Serial.println("║          SENDER STATION            ║");
  Serial.println("╚═══════════════════════════════════╝\n");
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Initialize LoRa
  Serial.print("Initializing SX1262... ");
  int16_t state = lora.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, 0x12, LORA_POWER);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✓ OK");
    loraReady = true;
    
    lora.setCurrentLimit(60.0);
    lora.setCRC(true);
    
    // Start in receive mode
    lora.startReceive();
    
    Serial.println("\n┌─────────────────────────────────┐");
    Serial.printf("│ Frequency: %.1f MHz             │\n", LORA_FREQ);
    Serial.printf("│ Bandwidth: %.1f kHz             │\n", LORA_BW);
    Serial.printf("│ Spreading Factor: %d             │\n", LORA_SF);
    Serial.printf("│ TX Power: %d dBm                │\n", LORA_POWER);
    Serial.println("└─────────────────────────────────┘");
    
  } else {
    Serial.printf("✗ Failed (error %d)\n", state);
    Serial.println("Check wiring and restart");
    loraReady = false;
  }
  
  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("📤 Type message + Enter to send");
  Serial.println("📥 Listening for incoming messages...");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
  Serial.print("> ");
}

void loop() {
  static uint32_t lastBlink = 0;
  static bool ledState = false;
  static String inputBuffer = "";
  
  // LED heartbeat (fast blink when receiving)
  uint32_t blinkInterval = 500;
  if (millis() - lastBlink >= blinkInterval) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    lastBlink = millis();
  }
  
  // Check for incoming LoRa messages
  if (loraReady) {
    int16_t state = lora.scanChannel();
    
    if (state == RADIOLIB_ERR_NONE) {
      uint8_t buffer[sizeof(MessagePacket)];
      int len = lora.getPacketLength();
      
      if (len > 0 && len <= sizeof(buffer)) {
        state = lora.readData(buffer, len);
        
        if (state == RADIOLIB_ERR_NONE && len >= sizeof(uint32_t) + sizeof(uint16_t)) {
          MessagePacket* packet = (MessagePacket*)buffer;
          
          // Validate message length
          if (packet->messageLen > 0 && packet->messageLen <= MAX_MESSAGE_LEN) {
            // Null-terminate message
            packet->message[packet->messageLen] = '\0';
            
            messageCount++;
            
            // Clear current input line and display received message
            Serial.print("\r\033[K"); // Clear line
            
            Serial.println("\n╔═══════════════════════════════════╗");
            Serial.println("║  📨 MESSAGE RECEIVED              ║");
            Serial.println("╚═══════════════════════════════════╝");
            Serial.printf("Message #%u\n", messageCount);
            Serial.printf("From: Remote Station\n");
            Serial.printf("Text: \"%s\"\n", packet->message);
            Serial.printf("RSSI: %d dBm\n", lora.getRSSI());
            Serial.printf("SNR: %.2f dB\n", lora.getSNR());
            Serial.println("═══════════════════════════════════\n");
            
            // Restore input prompt and current buffer
            Serial.print("> ");
            Serial.print(inputBuffer);
            
            // Brief flash to indicate message received
            for (int i = 0; i < 3; i++) {
              digitalWrite(LED_BUILTIN, HIGH);
              delay(50);
              digitalWrite(LED_BUILTIN, LOW);
              delay(50);
            }
          }
        }
      }
      
      // Restart receive
      lora.startReceive();
    }
  }
  
  // Handle serial input for outgoing messages
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        Serial.println(); // New line after input
        sendMessage(inputBuffer.c_str());
        inputBuffer = "";
      } else {
        Serial.print("> "); // Just show prompt if empty
      }
    } else if (c == 8 || c == 127) { // Backspace
      if (inputBuffer.length() > 0) {
        inputBuffer.remove(inputBuffer.length() - 1);
        Serial.print("\b \b"); // Erase character on screen
      }
    } else if (c >= 32 && c <= 126) { // Printable characters
      if (inputBuffer.length() < MAX_MESSAGE_LEN) {
        inputBuffer += c;
        Serial.print(c); // Echo character
      }
    }
  }
}