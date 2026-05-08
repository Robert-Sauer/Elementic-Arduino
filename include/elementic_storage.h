/*
 * elementic_storage.h
 *
 * Provides a byte-addressed persistent storage abstraction for Elementic.
 * AVR/ESP8266 use EEPROM directly; ESP32 uses a Preferences-backed RAM mirror.
 */

#pragma once

#include <Arduino.h>

#if defined(ESP32)
#include <Preferences.h>

class ElementicStorageClass {
public:
    ElementicStorageClass();
    ~ElementicStorageClass();

    bool begin(size_t size);
    uint8_t read(int address) const;
    void write(int address, uint8_t value);
    bool commit();
    int length() const;
    bool isBlank(size_t checkBytes = 32) const;

private:
    Preferences prefs;
    uint8_t* buffer;
    size_t bufferSize;
    bool initialized;
    bool dirty;
};

extern ElementicStorageClass ElementicStorage;

#else
#include <EEPROM.h>
#endif
