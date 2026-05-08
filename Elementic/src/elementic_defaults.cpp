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

#if defined(ESP8266)

  #include <ESP8266WiFi.h>
  static WiFiClient _elementic_wifiClient;
  Client& netClient = _elementic_wifiClient;

#elif defined(ESP32)

  #include <WiFi.h>
  static WiFiClient _elementic_wifiClient;
  Client& netClient = _elementic_wifiClient;

#elif defined(ARDUINO_ARCH_AVR)

  // AVR typically uses Ethernet (W5100/W5500 etc.)
  // You must have Ethernet library available and initialized in your sketch.
  #include <SPI.h>
  #include <Ethernet.h>

  static EthernetClient _elementic_ethClient;
  Client& netClient = _elementic_ethClient;

#else

  // Unsupported platform: keep compilation informative.
  // NOTE: If Elementic.cpp requires netClient to exist, you'll need to add support for your platform here.
  #warning "Elementic: netClient not defined for this platform. Add a Client implementation (WiFi/Ethernet/etc)."

#endif
