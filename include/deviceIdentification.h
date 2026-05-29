/*
 * deviceIdentification.h
 *
 * Compile-time capability detection for supported boards and network drivers. Maps platforms to Wi-Fi and Ethernet support macros.
 */

#pragma once

// =============================================================================
// Solution 2: PLATFORM-based compilation (Arduino-IDE friendly)
// - Compile WiFi on WiFi-capable platforms (ESP8266/ESP32/RP2040 Pico W/Opta)
// - Compile Ethernet on Ethernet-capable platforms (AVR/Wiznet, Teensy, Portenta, Opta)
// - DEVICEID is runtime/user-facing only (e.g. ELEMENTIC_DEVICEID variable), not required
// =============================================================================

// ------------------------------------------------------------
// 1) Driver enums
// ------------------------------------------------------------
#define ELEMENTIC_WIFI_DRIVER_NONE     0
#define ELEMENTIC_WIFI_DRIVER_ESP8266  1
#define ELEMENTIC_WIFI_DRIVER_ESP32    2
#define ELEMENTIC_WIFI_DRIVER_PICOW    3   // Pico W / Pico 2W (CYW43)
#define ELEMENTIC_WIFI_DRIVER_OPTA     4   // Arduino Opta WiFi stack

#define ELEMENTIC_ETH_DRIVER_NONE      0
#define ELEMENTIC_ETH_DRIVER_WIZNET    1   // Ethernet.h (W5100/W5500/W5100S etc.)
#define ELEMENTIC_ETH_DRIVER_OPTA      2   // Opta built-in Ethernet
#define ELEMENTIC_ETH_DRIVER_PORTENTA  3   // Portenta/PMC Ethernet library
#define ELEMENTIC_ETH_DRIVER_TEENSY    4   // NativeEthernet (Teensy 4.1)

// ------------------------------------------------------------
// 2) PLATFORM detection > compile-time feature flags
// ------------------------------------------------------------

// ---- WiFi capability (compile it only where it exists) ----
#if defined(ESP8266)
  #define ELEMENTIC_HAS_WIFI 1
  #define ELEMENTIC_WIFI_DRIVER ELEMENTIC_WIFI_DRIVER_ESP8266

#elif defined(ESP32)
  #define ELEMENTIC_HAS_WIFI 1
  #define ELEMENTIC_WIFI_DRIVER ELEMENTIC_WIFI_DRIVER_ESP32

// RP2040 Pico W / Pico 2W. Do not treat all RP2040 boards as WiFi boards:
// Wiznet RP2040 boards use Ethernet instead.
#elif defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_WIZNET_5100S_EVB_PICO)
  #define ELEMENTIC_HAS_WIFI 1
  #define ELEMENTIC_WIFI_DRIVER ELEMENTIC_WIFI_DRIVER_PICOW

// Arduino Opta (core defines ARDUINO_OPTA or similar; keep it permissive)
#elif defined(ARDUINO_OPTA) || defined(ARDUINO_PORTENTA_C33) || defined(ARDUINO_ARCH_MBED)
  // If your Opta target uses WiFi API, enable it
  #define ELEMENTIC_HAS_WIFI 1
  #define ELEMENTIC_WIFI_DRIVER ELEMENTIC_WIFI_DRIVER_OPTA

#else
  #define ELEMENTIC_HAS_WIFI 0
  #define ELEMENTIC_WIFI_DRIVER ELEMENTIC_WIFI_DRIVER_NONE
#endif

// ---- Ethernet capability (compile it only where it exists) ----
// Default: no Ethernet
#define ELEMENTIC_HAS_ETHERNET 0
#define ELEMENTIC_ETH_DRIVER   ELEMENTIC_ETH_DRIVER_NONE

// AVR (Uno/Mega/Controllino + Wiznet shields)
#if !ELEMENTIC_HAS_WIFI
  // If it isn't a WiFi platform, you likely want Ethernet on classic Arduino setups.
  // If you have a non-network device, you can disable this by defining ELEMENTIC_FORCE_NO_ETHERNET.
  #ifndef ELEMENTIC_FORCE_NO_ETHERNET
    #undef  ELEMENTIC_HAS_ETHERNET
    #undef  ELEMENTIC_ETH_DRIVER
    #define ELEMENTIC_HAS_ETHERNET 1
    #define ELEMENTIC_ETH_DRIVER   ELEMENTIC_ETH_DRIVER_WIZNET
  #endif
#endif

// Teensy 4.1
#if defined(TEENSYDUINO)
  #undef  ELEMENTIC_HAS_ETHERNET
  #undef  ELEMENTIC_ETH_DRIVER
  #define ELEMENTIC_HAS_ETHERNET 1
  #define ELEMENTIC_ETH_DRIVER   ELEMENTIC_ETH_DRIVER_TEENSY
#endif

// Wiznet W5100S-EVB-Pico
#if defined(ARDUINO_WIZNET_5100S_EVB_PICO)
  #undef  ELEMENTIC_HAS_ETHERNET
  #undef  ELEMENTIC_ETH_DRIVER
  #define ELEMENTIC_HAS_ETHERNET 1
  #define ELEMENTIC_ETH_DRIVER   ELEMENTIC_ETH_DRIVER_WIZNET
#endif

// Portenta / Portenta Machine Control (library/core dependent)
#if defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_PORTENTA_H7_M4) || defined(ARDUINO_PORTENTA_H7) || defined(ARDUINO_PORTENTA_MACHINE_CONTROL)
  #undef  ELEMENTIC_HAS_ETHERNET
  #undef  ELEMENTIC_ETH_DRIVER
  #define ELEMENTIC_HAS_ETHERNET 1
  #define ELEMENTIC_ETH_DRIVER   ELEMENTIC_ETH_DRIVER_PORTENTA
#endif

// Opta Ethernet (if you want Ethernet API compiled for Opta too)
#if defined(ARDUINO_OPTA)
  #undef  ELEMENTIC_HAS_ETHERNET
  #undef  ELEMENTIC_ETH_DRIVER
  #define ELEMENTIC_HAS_ETHERNET 1
  #define ELEMENTIC_ETH_DRIVER   ELEMENTIC_ETH_DRIVER_OPTA
#endif

// ------------------------------------------------------------
// 3) Detect whether network at all
// ------------------------------------------------------------
#define ELEMENTIC_HAS_NETWORK (ELEMENTIC_HAS_WIFI || ELEMENTIC_HAS_ETHERNET)

// ------------------------------------------------------------
// 4) Optional sanity checks (platform-based)
// ------------------------------------------------------------
#if ELEMENTIC_HAS_WIFI && (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_NONE)
  #error "WiFi enabled but no WiFi driver selected (platform mapping issue)."
#endif

#if ELEMENTIC_HAS_ETHERNET && (ELEMENTIC_ETH_DRIVER == ELEMENTIC_ETH_DRIVER_NONE)
  #error "Ethernet enabled but no Ethernet driver selected (platform mapping issue)."
#endif
