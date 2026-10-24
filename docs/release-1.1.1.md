# Elementic 1.1.1 release notes

## Goal

Reduce permanent AVR SRAM use without changing the V1 or V2 wire protocol,
MQTT content, EEPROM layout, or existing runtime-buffer APIs.

## Included

- Flash-aware overloads for constant log messages on AVR.
- AVR program-memory format strings through `snprintf_P` and `sprintf_P`.
- Direct byte emission for fixed serial framing markers in runtime events.
- Flash-backed fixed MQTT payloads such as `INITIALIZED`, `PRESSED`, and
  `RELEASED`.
- Existing `logging(LogLevel, const char*)` behavior for runtime-generated
  buffers.

## Not included

- Generated input/output names and MQTT topics in program memory.
- Generated generic-variable labels or default values in program memory.
- Changes to input/output limits or dynamic-variable slot counts.
- MQTT buffer-size or V2 frame-buffer changes.

## Compatibility requirements

The following output remains byte-for-byte compatible:

- V1 `S01` / `S02` / `E02` / `E01` framing.
- V2 diagnostic EVENT TLVs and their 49-byte text limit.
- MQTT severity prefixes: `[LOG]`, `[CAU]`, `[WAR]`, and `[ERR]`.
- Fixed MQTT input/output payloads.

## Release validation

The release was compiled for Controllino Mega V1 and V2, ESP8266 V1 and V2,
ESP32-C6, RP2040, and RP2350 targets. The RP validation includes Pico, Pico W,
Pico 2, Pico 2W, W5100S-EVB-Pico, and W5100S-EVB-Pico2. AVR `.data + .bss`
was compared with 1.1.0. Serial logging, MQTT reconnect, EEPROM restore, and
all diagnostic severities retain their existing public interfaces and wire
representations.

## SRAM measurement

The Controllino Maxi V1 smoke fixture changed from 6,617 bytes of static SRAM
with 1.1.0 to 4,327 bytes with 1.1.1: a reduction of 2,290 bytes (34.6%).
Program flash changed from 54,190 to 58,226 bytes (+4,036 bytes). This is the
intended AVR trade-off: immutable text moves from scarce SRAM to the much
larger program flash.

The Controllino Maxi V2 Ethernet fixture uses 3,317 of 8,192 SRAM bytes. Its
test configuration explicitly defines `ELEMENTIC_WIRE_PROTOCOL_VERSION=2`;
previously the fixture name said V2 while it was actually compiling the
default V1 path.
