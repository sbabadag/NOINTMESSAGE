/*
 * Pin Configuration for Wio SX1262 with XIAO ESP32S3
 * 
 * This file documents the pin connections between the XIAO ESP32S3
 * and the Wio SX1262 LoRa module based on the official schematic.
 */

#ifndef WIO_SX1262_PINS_H
#define WIO_SX1262_PINS_H

// ===== LORA MODULE PINS =====
// These pins connect the ESP32S3 to the SX1262 LoRa chip
#define LORA_NSS_PIN    D7    // SPI Chip Select (NSS)
#define LORA_DIO1_PIN   D1    // Digital I/O 1 (Interrupt)  
#define LORA_NRST_PIN   D0    // Reset pin (active low)
#define LORA_BUSY_PIN   D2    // Busy status pin
// Note: MOSI, MISO, SCK use default SPI pins

// ===== USER INTERFACE PINS =====
#define USER_LED_PIN    LED_BUILTIN  // Built-in LED on XIAO
#define USER_BUTTON_PIN D3           // User button on Wio board

// ===== XIAO ESP32S3 PIN MAPPING =====
/*
Physical Pin Layout (Top View):
     USB-C
   ┌─────────┐
D0 │1      14│ D10
D1 │2      13│ D9  
D2 │3      12│ D8
D3 │4      11│ D7
D4 │5      10│ D6
D5 │6       9│ D5
3V3│7       8│ GND
   └─────────┘

Alternative Pin Names:
- D0 = GPIO1   (LORA_NRST)
- D1 = GPIO2   (LORA_DIO1) 
- D2 = GPIO3   (LORA_BUSY)
- D3 = GPIO4   (USER_BUTTON)
- D4 = GPIO5
- D5 = GPIO6
- D6 = GPIO43  
- D7 = GPIO44  (LORA_NSS/CS)
- D8 = GPIO7   (MOSI)
- D9 = GPIO8   (MISO) 
- D10= GPIO9   (SCK)
*/

// ===== SPI CONFIGURATION =====
// The Wio SX1262 uses hardware SPI for communication
// Default SPI pins on XIAO ESP32S3:
// - MOSI (Master Out Slave In): D8 (GPIO7)
// - MISO (Master In Slave Out):  D9 (GPIO8)  
// - SCK (Serial Clock):         D10 (GPIO9)
// - CS (Chip Select):           D7 (GPIO44) - LORA_NSS_PIN

// ===== POWER PINS =====
// - 3V3: 3.3V power supply
// - 5V:  5V power supply (when USB connected)
// - GND: Ground
// - BAT: Battery input (3.7V Li-Po)

// ===== ADDITIONAL NOTES =====
/*
1. The Wio SX1262 board stacks on top of the XIAO ESP32S3
2. All LoRa communication happens via SPI
3. DIO1 is used for interrupt-driven receive
4. BUSY pin indicates when the SX1262 is processing
5. NRST is used to reset the LoRa chip during initialization
6. User button and LED are for status indication and testing
*/

#endif // WIO_SX1262_PINS_H