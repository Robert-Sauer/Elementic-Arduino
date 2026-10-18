#include <Arduino.h>
#include <Elementic.h>

const uint16_t ELEMENTIC_METADATA_ABI = 1;
const uint64_t ELEMENTIC_FIRMWARE_PROJECT_ID = UINT64_C(0x0000000000000001);
const uint32_t ELEMENTIC_PROJECT_REVISION = 1;
const uint16_t ELEMENTIC_CONFIG_SCHEMA = 1;
const uint16_t ELEMENTIC_DEVICEID = 12;
const uint32_t ELEMENTIC_TARGET_REVISION = 1;
const char ELEMENTIC_TARGET_PROFILE[49] = "rp-compatibility-test";
const char ELEMENTIC_MCU_FAMILY[17] = "rp2";
const char ELEMENTIC_APP_VERSION[32] = "test";
const char ELEMENTIC_BUILD_ULID[27] = "";
const char ELEMENTIC_RELEASE_ULID[27] = "";

void setup() {
  ElementicSetup();
}

void loop() {
  ElementicLoop();
}
