#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("===========================");
    Serial.println("🚀 Hello World Test");
    Serial.println("===========================");
    
    pinMode(LED_BUILTIN, OUTPUT);
    
    Serial.println("✅ Setup complete!");
}

void loop() {
    static int counter = 0;
    
    Serial.printf("Hello World! Count: %d\n", counter++);
    
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
}