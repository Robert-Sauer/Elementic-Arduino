#pragma once

// =============================================================================
// Solution 2: PLATFORM-based compilation (Arduino-IDE friendly)
// - Compile WiFi on WiFi-capable platforms (ESP8266/ESP32/RP2040 Pico W/Opta)
// - Compile Ethernet on Ethernet-capable platforms (AVR/Wiznet, Teensy, Portenta, Opta)
// - DEVICEID is runtime/user-facing only (e.g. ELEMENTIC_DEVICEID variable), not required
// =============================================================================


// ------------------------------------------------------------
// 0) Optional: keep these lists from Excel for runtime logic
//    (No longer used for compile-time capability flags)
// ------------------------------------------------------------
#define DEVICE_HAS_WIFI(ID) ( \
    ((ID) == 4)  || ((ID) == 6)  || ((ID) == 7)  || ((ID) == 8)  || \
    ((ID) == 9)  || ((ID) == 10) || ((ID) == 12) || ((ID) == 19) || \
    ((ID) == 20) || ((ID) == 21) || ((ID) == 24) || ((ID) == 25) || \
    ((ID) == 26) || ((ID) == 27) || ((ID) == 28) || ((ID) == 29) || \
    ((ID) == 30) || ((ID) == 31) \
)

#define DEVICE_HAS_ETHERNET(ID) ( \
    ((ID) == 2)  || ((ID) == 3)  || ((ID) == 4)  || ((ID) == 5)  || \
    ((ID) == 11) || ((ID) == 15) || ((ID) == 17) \
)


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
// 2) PLATFORM detection → compile-time feature flags
// ------------------------------------------------------------

// ---- WiFi capability (compile it only where it exists) ----
#if defined(ESP8266)
  #define ELEMENTIC_HAS_WIFI 1
  #define ELEMENTIC_WIFI_DRIVER ELEMENTIC_WIFI_DRIVER_ESP8266

#elif defined(ESP32)
  #define ELEMENTIC_HAS_WIFI 1
  #define ELEMENTIC_WIFI_DRIVER ELEMENTIC_WIFI_DRIVER_ESP32

// RP2040 Pico W / Pico 2W: depends on core. Both common cores define ARDUINO_ARCH_RP2040.
// Pico W boards often define ARDUINO_RASPBERRY_PI_PICO_W or similar.
// This branch is permissive: it compiles WiFi if RP2040 is used and WiFi.h exists in your core.
#elif defined(ARDUINO_ARCH_RP2040)
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
