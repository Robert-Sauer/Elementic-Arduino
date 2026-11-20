#pragma once
#include <stdint.h>

#ifndef ELEMENTIC_BOOTLOADER_SERASIDIS_HID
#define ELEMENTIC_BOOTLOADER_SERASIDIS_HID 0
#endif

// Wire IDs are independent of SerialCommand's legacy enum.
enum { ELEMENTIC_COMMAND_ENTER_BOOTLOADER = 8, ELEMENTIC_BOOTLOADER_NONE = 0,
       ELEMENTIC_BOOTLOADER_SERASIDIS_F103 = 1 };
inline bool elementicSerasidisVectorsValid(uint32_t vtor, uint32_t stack, uint32_t reset) {
    return vtor == 0x08000800UL && stack > 0x20000000UL && stack <= 0x20005000UL
            && (stack & 7U) == 0U && (reset & 1U) != 0U
            && (reset & ~1UL) >= 0x08000008UL && (reset & ~1UL) < 0x08000800UL;
}
uint8_t elementicBootloaderMethod();
uint8_t elementicRequestBootloader(); // V2 RESULT status: OK, UNSUPPORTED or BUSY.
bool elementicBootloaderPending();
bool elementicBootloaderLoop();      // true while entry is pending; called outside parser.
