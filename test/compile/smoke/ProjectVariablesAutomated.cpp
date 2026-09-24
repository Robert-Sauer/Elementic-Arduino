#include <Elementic.h>

const int device_type = 0;
const long projectid = 0;

extern const uint16_t ELEMENTIC_METADATA_ABI = 1;
extern const uint64_t ELEMENTIC_FIRMWARE_PROJECT_ID = UINT64_C(0);
extern const uint32_t ELEMENTIC_PROJECT_REVISION = 0;
extern const uint16_t ELEMENTIC_CONFIG_SCHEMA = 1;
extern const uint32_t ELEMENTIC_TARGET_REVISION = 1;
extern const char ELEMENTIC_TARGET_PROFILE[49] = "compile-smoke";
extern const char ELEMENTIC_MCU_FAMILY[17] = "compile-only";
extern const char ELEMENTIC_APP_VERSION[32] = "0.0.0-local";
extern const char ELEMENTIC_BUILD_ULID[27] = "";
extern const char ELEMENTIC_RELEASE_ULID[27] = "";

const char MQTTName[] = "";
const char MQTTServer[] = "";
const char MQTTUsername[] = "";
const char MQTTPassword[] = "";
const uint8_t factorymac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
const bool factoryDHCP = true;
const uint8_t factoryip[4] = {0, 0, 0, 0};
const uint8_t factorygateway[4] = {0, 0, 0, 0};
const uint8_t factorysubnet[4] = {0, 0, 0, 0};
const char factoryhostname[] = "elementic-smoke";
const char factorySSID[] = "";
const char factoryWIFIPassword[] = "";

void genericsetupvariables() {}
void inputsetupvariables() {}
void outputsetupvariables() {}

void elementicLoadFactoryDefaults() {
  elementicResetConfigurationToEmpty();
  setupNetwork();
  genericsetupvariables();
  MQTTSetupVariables();
  outputsetupvariables();
  inputsetupvariables();
  elementicApplySharedMac(false);
}
