// MESSAGE TUNNEL - SENDER STATION
// Sends text messages over LoRa to receiver station

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

// LoRa Configuration
constexpr float LORA_FREQ = 915.0;      // Adjust for your region
constexpr float LORA_BW = 125.0;
constexpr uint8_t LORA_SF = 7;
constexpr uint8_t LORA_CR = 5;
constexpr int8_t LORA_POWER = 22;

// Message settings
constexpr uint16_t MAX_MESSAGE_LEN = 200;

SX1262 lora = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RESET, PIN_LORA_BUSY);
bool loraReady = false;

// Simple message packet structure
struct MessagePacket {
  uint32_t timestamp;           // Message timestamp
  uint16_t messageLen;          // Length of message
  char message[MAX_MESSAGE_LEN]; // Message text
} __attribute__((packed));

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
  Serial.println("║  SENDING MESSAGE                  ║");
  Serial.println("╚═══════════════════════════════════╝");
  Serial.printf("Message: \"%s\"\n", text);
  Serial.printf("Length: %d bytes\n", len);
  
  // Transmit
  int16_t state = lora.transmit((uint8_t*)&packet, sizeof(uint32_t) + sizeof(uint16_t) + len);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("✓ Message sent successfully!");
    
    // Get transmission stats
    Serial.printf("Time on air: %d ms\n", lora.getTimeOnAir(len + 6));
    Serial.printf("Data rate: %.2f bps\n", lora.getDataRate());
    
  } else {
    Serial.printf("✗ Transmission failed, error: %d\n", state);
  }
  
  Serial.println("═══════════════════════════════════\n");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║   MESSAGE TUNNEL - SENDER         ║");
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
  Serial.println("Type your message and press Enter");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void loop() {
  static uint32_t lastBlink = 0;
  static bool ledState = false;
  static String inputBuffer = "";
  
  // LED heartbeat
  if (millis() - lastBlink >= 500) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    lastBlink = millis();
  }
  
  // Read serial input
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        // Send the message
        sendMessage(inputBuffer.c_str());
        inputBuffer = "";
        Serial.print("\n> "); // Prompt for next message
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
