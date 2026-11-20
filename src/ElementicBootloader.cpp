#include "Elementic.h"
#include "ElementicBootloader.h"

#if ELEMENTIC_BOOTLOADER_SERASIDIS_HID
#if !defined(ARDUINO_ARCH_STM32) || !defined(STM32F103xB) || !defined(USBD_USE_CDC)
#error "Serasidis entry requires STM32F103C8/CB with native USB CDC"
#endif
#if ELEMENTIC_WIRE_PROTOCOL_VERSION != 2
#error "Software bootloader entry requires Elementic wire protocol V2"
#endif
#include "usbd_cdc_if.h"

namespace {
bool pending = false;
unsigned long requestedAt = 0;
}

uint8_t elementicBootloaderMethod() {
    // A normal ST-Link image at flash base must never claim this capability.
    if (SCB->VTOR != 0x08000800UL) return ELEMENTIC_BOOTLOADER_NONE;
    const uint32_t stack = *reinterpret_cast<volatile const uint32_t*>(0x08000000UL);
    const uint32_t reset = *reinterpret_cast<volatile const uint32_t*>(0x08000004UL);
    if (!elementicSerasidisVectorsValid(SCB->VTOR, stack, reset)) return ELEMENTIC_BOOTLOADER_NONE;
    return ELEMENTIC_BOOTLOADER_SERASIDIS_F103;
}

uint8_t elementicRequestBootloader() {
    if (elementicBootloaderMethod() == ELEMENTIC_BOOTLOADER_NONE) return 2U;
    if (pending || SerialCommand != -1) return 3U;
    requestedAt = millis();
    pending = true;
    return 0U;
}

bool elementicBootloaderPending() { return pending; }

bool elementicBootloaderLoop() {
    if (!pending) return false;
    // RESULT is queued by the parser first. Unlike USBSerial::flush(), this
    // wait is bounded even if the host disappears or stops reading.
    const unsigned long elapsed = millis() - requestedAt;
    if (elapsed < 30UL) return true;
    if (CDC_TransmitQueue_ReadSize(&TransmitQueue) != 0) {
        if (elapsed < 250UL) return true;
        pending = false;
        ELEMENTIC_LOG(LOG_WARNING, "Bootloader entry cancelled: USB response not drained");
        return false;
    }
    // Serasidis 2.2.2 consumes and clears only DR4. Never reset the backup
    // domain: RTC settings and other backup registers must be preserved.
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN);
    SET_BIT(PWR->CR, PWR_CR_DBP);
    WRITE_REG(BKP->DR4, 0x424CU);
    CLEAR_BIT(PWR->CR, PWR_CR_DBP);
    __DSB();
    NVIC_SystemReset();
    return true;
}
#else
uint8_t elementicBootloaderMethod() { return ELEMENTIC_BOOTLOADER_NONE; }
uint8_t elementicRequestBootloader() { return 2U; }
bool elementicBootloaderPending() { return false; }
bool elementicBootloaderLoop() { return false; }
#endif
