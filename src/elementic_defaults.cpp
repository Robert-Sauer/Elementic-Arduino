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
#include <Client.h>
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
extern const char ELEMENTIC_LIBRARY_VERSION[] = "1.1.0";
extern const uint16_t ELEMENTIC_STORAGE_SCHEMA = 2;

#if defined(ESP8266)

  #include <ESP8266WiFi.h>
  static WiFiClient _elementic_wifiClient;
  Client& netClient = _elementic_wifiClient;

#elif defined(ESP32)

  #include <WiFi.h>
  static WiFiClient _elementic_wifiClient;
  Client& netClient = _elementic_wifiClient;

#elif ELEMENTIC_HAS_WIFI && (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_PICOW)

  // The Arduino-Pico core supplies WiFiClient for both Pico W (RP2040) and
  // Pico 2 W (RP2350). Without this branch MQTT would receive NullClient.
  #include <WiFi.h>
  static WiFiClient _elementic_wifiClient;
  Client& netClient = _elementic_wifiClient;

#elif ELEMENTIC_HAS_ETHERNET

  // Ethernet platforms use W5100/W5200/W5500-compatible clients.
  #include <SPI.h>
  #include <Ethernet.h>

  static EthernetClient _elementic_ethClient;
  Client& netClient = _elementic_ethClient;

#else

  // Serial-only targets retain offline configuration without pretending that
  // a network adapter exists.
  class ElementicNullClient : public Client {
   public:
    int connect(IPAddress, uint16_t) override { return 0; }
    int connect(const char*, uint16_t) override { return 0; }
    size_t write(uint8_t) override { return 0; }
    size_t write(const uint8_t*, size_t) override { return 0; }
    int available() override { return 0; }
    int read() override { return -1; }
    int read(uint8_t*, size_t) override { return 0; }
    int peek() override { return -1; }
    void flush() override {}
    void stop() override {}
    uint8_t connected() override { return 0; }
    operator bool() override { return false; }
  };

  static ElementicNullClient _elementic_nullClient;
  Client& netClient = _elementic_nullClient;

#endif
