// Copy into a manual project's src directory and replace target values with
// the exact approved profile. Do not convert a legacy integer projectid.
#include <Elementic.h>

extern const uint16_t ELEMENTIC_METADATA_ABI = 1;
extern const uint64_t ELEMENTIC_FIRMWARE_PROJECT_ID = UINT64_C(0);
extern const uint32_t ELEMENTIC_PROJECT_REVISION = 0;
extern const uint16_t ELEMENTIC_CONFIG_SCHEMA = 1;
extern const uint32_t ELEMENTIC_TARGET_REVISION = 1;
extern const char ELEMENTIC_TARGET_PROFILE[49] = "replace-with-target";
extern const char ELEMENTIC_MCU_FAMILY[17] = "replace-with-mcu";
extern const char ELEMENTIC_APP_VERSION[32] = "0.0.0-local";
extern const char ELEMENTIC_BUILD_ULID[27] = "";
extern const char ELEMENTIC_RELEASE_ULID[27] = "";
