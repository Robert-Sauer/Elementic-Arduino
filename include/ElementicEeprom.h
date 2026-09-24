#pragma once

#include <Arduino.h>

#if defined(__has_include)
  #if __has_include(<EEPROM.h>)
    #include <EEPROM.h>
    #define ELEMENTIC_HAS_EEPROM_ADAPTER 1
  #else
    #define ELEMENTIC_HAS_EEPROM_ADAPTER 0
  #endif
#else
  #include <EEPROM.h>
  #define ELEMENTIC_HAS_EEPROM_ADAPTER 1
#endif

#if !ELEMENTIC_HAS_EEPROM_ADAPTER
// A serial-only target without an EEPROM/NVS adapter remains usable with
// compiled defaults. Saves fail explicitly; no RAM-only persistence is
// advertised as durable storage.
class ElementicUnavailableEeprom {
 public:
  size_t length() const { return 0U; }
  uint8_t read(size_t) const { return 0xFFU; }
  void write(size_t, uint8_t) {}
  bool commit() { return false; }
  bool begin(size_t) { return false; }
};

static ElementicUnavailableEeprom EEPROM;
#endif
