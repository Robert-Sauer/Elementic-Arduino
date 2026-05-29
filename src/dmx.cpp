/*
 * dmx.cpp
 *
 * Implements DMX channel handling and platform-specific serial output support for lighting and stage-control scenarios.
 */

#include "Elementic.h"
#include <Arduino.h>

#define DMX_MAX 10 // Max number of DMX channels

// uint8_t dimvalue = 0; already un elementic.cpp
// uint8_t state = 0;
// uint8_t DMXBuffer[DMX_MAX];

#if defined(ESP8266)
  #define DMXSerial Serial1
  const int DMXDirectionPin = D4;

#elif defined(ESP32) || defined(ESP_PLATFORM)
  #define DMXSerial Serial1
  const int DMX_TX_PIN = 17; // Set to correct TX pin for your ESP32-C6 board
  const int DMXDirectionPin = 16;

#elif defined(ARDUINO_ARCH_AVR)
  #if defined(RS485_SERIAL_PORT)
    #define DMXSerial RS485_SERIAL_PORT
  #elif defined(UBRR3H) || defined(ARDUINO_AVR_MEGA2560)
    #define DMXSerial Serial3
  #else
    #define DMXSerial Serial
  #endif

  #if defined(__AVR_ATmega2560__) || defined(ARDUINO_AVR_MEGA2560)
    #define USE_PORTJ
    const uint8_t PORTJ_DDR_MASK = B01000000;
  #endif

#elif defined(ARDUINO_RASPBERRY_PI_PICO)
  #define DMXSerial Serial1
  const int DMX_TX_PIN = 4;  // Adjust if needed
  const int DMXDirectionPin = 5;

#elif defined(esp32)||defined(ESP_PLATFORM) // oud: #elif defined(esp32-c6-devkitm-1)||defined(ESP_PLATFORM)
  #define DMXSerial Serial1
  const int DMX_TX_PIN = 17; // Set to correct TX pin for your ESP32-C6 board
  const int DMXDirectionPin = 16;

#else
  //#error "Unsupported platform for DMX"
#endif

void DMXstart() {
  #if defined(ELEMENTIC_DISABLE_DMX)
    return;
  #endif

  #if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO)
    DMXSerial.begin(250000, SERIAL_8N2, -1, DMX_TX_PIN);
    pinMode(DMXDirectionPin, OUTPUT);
    digitalWrite(DMXDirectionPin, HIGH); // Enable TX mode
  #elif defined(ESP8266)
    DMXSerial.begin(250000);
    pinMode(DMXDirectionPin, OUTPUT);
    digitalWrite(DMXDirectionPin, HIGH);
  #elif defined(ARDUINO_ARCH_AVR)
    DMXSerial.begin(250000);
    #if defined(USE_PORTJ)
      DDRJ |= PORTJ_DDR_MASK;     // Set PJ6 as output
      PORTJ |= PORTJ_DDR_MASK;    // Set HIGH (TX)
    #endif
  #endif

  for (int n = 0; n < DMX_MAX; n++) {
    DMXBuffer[n] = 0;
  }
}

void DMXwrite(int channel, uint8_t value) {
  if (channel < 1) channel = 1;
  if (channel > DMX_MAX) channel = DMX_MAX;
  DMXBuffer[channel - 1] = constrain(value, 0, 255);
}

void DMXflush() {
  #if defined(ELEMENTIC_DISABLE_DMX)
    return;
  #endif

  #if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_RASPBERRY_PI_PICO)
    DMXSerial.updateBaudRate(125000);     // BREAK
    DMXSerial.write((uint8_t)0);
    DMXSerial.flush();

    DMXSerial.updateBaudRate(250000);     // DMX speed
    DMXSerial.write((uint8_t)0);          // START code
    DMXSerial.write(DMXBuffer, DMX_MAX);
    DMXSerial.flush();

  #elif defined(ARDUINO_ARCH_AVR)
    DMXSerial.begin(125000);
    DMXSerial.write((uint8_t)0);
    DMXSerial.flush();

    DMXSerial.begin(250000);
    DMXSerial.write((uint8_t)0);
    DMXSerial.write(DMXBuffer, DMX_MAX);
    DMXSerial.flush();
  #endif
}
// Implementation for dmx
