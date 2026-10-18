#include <Arduino.h>

#include <Elementic.h>

// <elementic:metadata>
const uint16_t ELEMENTIC_METADATA_ABI = 1;
const uint64_t ELEMENTIC_FIRMWARE_PROJECT_ID = UINT64_C(0x0000000000000000);
const uint32_t ELEMENTIC_PROJECT_REVISION = 0;
const uint16_t ELEMENTIC_CONFIG_SCHEMA = 1;
const uint16_t ELEMENTIC_DEVICEID = 20;
const uint32_t ELEMENTIC_TARGET_REVISION = 1;
const char ELEMENTIC_TARGET_PROFILE[49] = "esp8266-esp12e";
const char ELEMENTIC_MCU_FAMILY[17] = "esp8266";
const char ELEMENTIC_APP_VERSION[32] = "0.0.0-local";
const char ELEMENTIC_BUILD_ULID[27] = "";
const char ELEMENTIC_RELEASE_ULID[27] = "";
// </elementic:metadata>

void setup() {
  ElementicSetup();
}

void loop() {
  ElementicLoop();
  yield();
}
