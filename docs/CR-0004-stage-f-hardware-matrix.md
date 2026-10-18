# CR-0004 Stage F manual hardware matrix

No row below has been executed during this software-only Stage F pass. Compilation is not hardware support. Record tester, date, exact board revision, USB/network adapter, firmware SHA and evidence links for every checked row.

| Target/profile candidate | Local compile | Required physical checks | Current status |
|---|---|---|---|
| Controllino Maxi / ATmega2560 / W5100-class Ethernet | Pass; 57,566-byte flash, 6,909-byte RAM (84.3%) | W5100 detection and boot latency; validated shared MAC on wire; DHCP/static IP; unplug/replug; schema-1 capture; two-slot save with power cuts; offline factory reset; relay-safe boot | Manual validation required. Example compiled MAC starts with multicast bit and must visibly enter provisioning-required state rather than start networking. RAM headroom is a risk. |
| ESP8266 ESP-12E / station Wi-Fi | v1 and v2 pass; v2 318,431-byte flash, 35,912-byte RAM | `wifi_set_macaddr` readback; association; invalid-MAC correction; provisioning power cut; save/reload/reset; split/combined serial frames; v1 Android and v2 Android sessions | Manual validation required. |
| ESP32-C6 DevKitM-1 / station Wi-Fi | Pass; 1,055,064-byte flash, 48,576-byte RAM | `esp_wifi_set_mac` readback; association; NVS/EEPROM emulation power cuts; reset; serial framing; reboot metadata | Manual validation required. Compile used the configured pioarduino toolchain only. |
| ESP32 Dev Module / station Wi-Fi | Smoke compile pass; 1,004,911-byte flash, 52,300-byte RAM | Same checks as ESP32-C6 on the exact board/USB bridge; target metadata must not reuse the smoke fixture | Manual validation required. |
| Raspberry Pi Pico / RP2040 (plain) | Pass; 97,256-byte flash, 12,796-byte RAM | EEPROM emulation save/reload/power loss; serial framing; RP UART DMX break and 8N2 output | Compile-compatible; manual validation required. |
| Raspberry Pi Pico W / RP2040 + CYW43 | Pass; 421,888-byte flash, 75,792-byte RAM | Factory-MAC readback; Wi-Fi association/DHCP; MQTT/reconnect; EEPROM emulation; serial framing; RP UART DMX | Compile-compatible; manual validation required. The runtime MAC must equal the CYW43 hardware MAC. |
| Raspberry Pi Pico 2 / RP2350 (plain) | Pass; 95,284-byte flash, 13,492-byte RAM | EEPROM emulation save/reload/power loss; serial framing; RP UART DMX break and 8N2 output | Compile-compatible; manual validation required. |
| Raspberry Pi Pico 2 W / RP2350 + CYW43 | Pass; 417,956-byte flash, 77,124-byte RAM | Factory-MAC readback; Wi-Fi association/DHCP; MQTT/reconnect; EEPROM emulation; serial framing; RP UART DMX | Compile-compatible; manual validation required. The runtime MAC must equal the CYW43 hardware MAC. |
| WIZnet W5100S-EVB-Pico / RP2040 Ethernet | Pass; 142,148-byte flash, 13,488-byte RAM | W5100S reset/SPI pins; configured MAC on wire; deferred DHCP; static IP; MQTT/reconnect; EEPROM emulation; serial framing | Compile-compatible; manual validation required. |
| WIZnet W5100S-EVB-Pico2 / RP2350 Ethernet | Pass; 140,752-byte flash, 14,768-byte RAM | W5100S reset/SPI pins; configured MAC on wire; deferred DHCP; static IP; MQTT/reconnect; EEPROM emulation; serial framing | Compile-compatible; manual validation required. |
| Arduino Uno | Compile reaches size check but fails: 53,744/32,256 flash and 6,294/2,048 RAM | None until capacities/features are reduced and a target profile is approved | Not supported; library-level `atmelavr` metadata must not be interpreted as Uno eligibility. |

## Per-target procedure

1. Capture the pre-update explicit full-sync bytes and the complete EEPROM/NVS image twice; hash both captures and record the installed tag/commit.
2. Update Android first. Confirm ASCII `0x31` and numeric `0x01` v1, headerless messages, incompatible-version latching and blocked writes.
3. Install the exact candidate image without erasing protected provisioning. Verify metadata, zero/full-width Project ID, target revision and build identity.
4. Exercise malformed, oversized, CRC-failed, split and concatenated frames. Confirm no configuration or output side effect occurs before validation.
5. Save alternating schema-2 slots. Interrupt power at header, payload, CRC and commit writes; verify the newest complete slot or compiled-safe defaults are selected.
6. Provision two devices with different locally administered unicast MACs. Read back after power loss, project replacement and factory reset. Confirm an invalid edit restores the last valid address and reports `CORRECTED`.
7. For dual-interface hardware, prove Ethernet preference, Wi-Fi stop-before-Ethernet and Wi-Fi fallback without simultaneous use of the shared address.
8. Verify factory reset offline restores the installed image's original password defaults, preserves unit provisioning and keeps outputs inactive until validated configuration is applied.
9. Repeat save/reset at storage-generation and capacity boundaries. Confirm unknown layouts are offered backup/reset/cancel, never silently repaired.

Hardware support may be recorded only after the exact target/profile row passes this matrix.
