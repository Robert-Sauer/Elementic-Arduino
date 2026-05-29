#include <Arduino.h>

#define OutputChannels 15
#define InputChannels 15

extern const uint16_t ELEMENTIC_DEVICEID = 20; // NodeMCU V2/V3 ESP8266 ESP-12E

#include <Elementic.h>

void setup() {
  ElementicSetup();
}

void loop() {
  ElementicLoop();
  yield();
}
