/*
 * ramusage.cpp
 *
 * Implements a cross-platform helper that reports approximate free RAM for the supported microcontroller families.
 */

#include "Elementic.h"
#if defined(ARDUINO_ARCH_AVR)

extern char *__brkval;
extern char __bss_end;

int freeRam() {
    char stack;
    if (__brkval == 0)
        return &stack - &__bss_end;
    return &stack - __brkval;
}

#elif defined(ESP32)

#include <esp_heap_caps.h>
int freeRam() {
    return heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
}

#elif defined(ESP8266)

extern "C" {
#include "user_interface.h"
}

int freeRam() {
    return system_get_free_heap_size();
}

#elif defined(ARDUINO_ARCH_SAMD)

#include <malloc.h>
int freeRam() {
    struct mallinfo mi = mallinfo();
    return mi.fordblks;
}

#elif defined(ARDUINO_RASPBERRY_PI_PICO) || defined(ARDUINO_ARCH_RP2040)

#include <pico/malloc.h>
int freeRam() {
    return mallinfo().fordblks;
}

#elif defined(ARDUINO_ARCH_STM32)

extern char _end;
extern char _estack;

int freeRam() {
    char stack;
    return &stack - &_end;
}

#else   // ---------- THIS MUST RETURN INT ----------

int freeRam() {
    return -1;
}

#endif
