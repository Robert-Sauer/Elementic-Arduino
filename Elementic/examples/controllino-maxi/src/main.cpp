#include <Arduino.h>

extern const uint16_t ELEMENTIC_DEVICEID = 2;

#include <Elementic.h>

void setup() {
  ElementicSetup();
  ModifyVariableStatus(Generic, 0, "IP Address", STATUS_GRAYED, true);
}

void loop() {
  ElementicLoop();
}
