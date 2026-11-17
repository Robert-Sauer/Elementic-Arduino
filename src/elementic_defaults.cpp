/*
 * elementic_defaults.cpp
 *
 * Provides default definitions for optional globals expected by the library when they are not supplied elsewhere.
 */

#include <Arduino.h>
#include <Elementic.h>

// ------------------------------------------------------------
// Defaults for globals that the library currently expects
// ------------------------------------------------------------

// Logging switches (referenced by logging.cpp)
bool logtoMQTT  = false;
bool logtoSerial = true;

unsigned long delayTime = 0;

// ------------------------------------------------------------
// Network client hookup for PubSubClient
// Elementic.cpp constructs PubSubClient with netClient.
// Provide the proper netClient for each platform.
// ------------------------------------------------------------
#include "deviceIdentification.h"

#if defined(__GNUC__)
  #define ELEMENTIC_WEAK __attribute__((weak))
#else
  #define ELEMENTIC_WEAK
#endif

// Metadata ABI 1 manual-project defaults. Generated main.cpp supplies strong
// definitions through declarations in Elementic.h. Firmware Project ID remains
// zero for old sketches; legacy `projectid` is never widened as authoritative.
extern const uint16_t ELEMENTIC_METADATA_ABI ELEMENTIC_WEAK = 1;
extern const uint64_t ELEMENTIC_FIRMWARE_PROJECT_ID ELEMENTIC_WEAK = UINT64_C(0);
extern const uint32_t ELEMENTIC_PROJECT_REVISION ELEMENTIC_WEAK = 0;
extern const uint16_t ELEMENTIC_CONFIG_SCHEMA ELEMENTIC_WEAK = 1;
extern const uint32_t ELEMENTIC_TARGET_REVISION ELEMENTIC_WEAK = 0;
extern const char ELEMENTIC_TARGET_PROFILE[49] ELEMENTIC_WEAK = "manual-unclassified";
extern const char ELEMENTIC_MCU_FAMILY[17] ELEMENTIC_WEAK = "unknown";
extern const char ELEMENTIC_APP_VERSION[32] ELEMENTIC_WEAK = "0.0.0-local";
extern const char ELEMENTIC_BUILD_ULID[27] ELEMENTIC_WEAK = "";
extern const char ELEMENTIC_RELEASE_ULID[27] ELEMENTIC_WEAK = "";
extern const char ELEMENTIC_LIBRARY_VERSION[] = "1.1.3";
extern const uint16_t ELEMENTIC_STORAGE_SCHEMA = 2;

#if ELEMENTIC_USE_MQTT
  #if ELEMENTIC_USE_WIFI
    #if ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_ESP8266
#include <ESP8266WiFi.h>
    #else
#include <WiFi.h>
    #endif
    static WiFiClient elementicWifiClient;
    Client& netClient = elementicWifiClient;
  #elif ELEMENTIC_USE_ETHERNET
#include <Ethernet.h>
    static EthernetClient elementicEthernetClient;
    Client& netClient = elementicEthernetClient;
  #endif
#endif
