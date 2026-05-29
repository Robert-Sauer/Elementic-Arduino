#include <Arduino.h>

#define OutputChannels 15
#define InputChannels 15

extern const uint16_t ELEMENTIC_DEVICEID = 8; // ESP32-C6

#include <Elementic.h>

void setup() {
  ElementicSetup();
}

void loop() {
  ElementicLoop();
}
