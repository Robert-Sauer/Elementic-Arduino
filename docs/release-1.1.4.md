# Elementic 1.1.4: software bootloader entry

Release date: 20 November 2026. The owner authorized GitHub and PlatformIO
publication with the optional bootloader feature disabled by default. Physical
Android CDC-to-HID acceptance remains pending; publication is not a claim that
those hardware checks have passed.

## Changes

- Add generic protocol V2 COMMAND 8 (`ENTER_BOOTLOADER`, argument zero).
- Add optional INFO category 3 / item 0 / field `0x33`: method 0 unavailable,
  method 1 STM32F103 Serasidis HID 2.2.2.
- Implement method 1 only with explicit `ELEMENTIC_BOOTLOADER_SERASIDIS_HID=1`.
  Requires STM32F103C8/CB, native USB CDC, protocol V2 and an application linked
  at `0x08000800`. Plausible bootloader vectors are checked before advertising.
- Queue entry outside the serial parser, return correlated RESULT first, wait
  30–250 ms for CDC transmission, then set `BKP->DR4 = 0x424C` and system-reset.
  If transmission does not drain, cancel entry without writing the marker.
- Keep other backup registers, RTC, EEPROM configuration and flash untouched.
  Ordinary mutations receive BUSY during pending entry. Unsupported targets
  return UNSUPPORTED and compile without STM32 register access or entry state.
- Advance `library.json` and runtime `ELEMENTIC_LIBRARY_VERSION` together.

The generic command, not an STM32-named application command, is the extension
point for future reviewed bootloader methods. No new v1 command, EEPROM schema,
wire version or automatic setting persistence is introduced.

## Enabling a compatible project

Keep the existing HID memory reservations and add:

```ini
upload_protocol = hid
board_build.flash_offset = 0x800
board_upload.maximum_size = 63488
build_flags =
    -DELEMENTIC_BOOTLOADER_SERASIDIS_HID=1
    -DELEMENTIC_WIRE_PROTOCOL_VERSION=2
    -DUSBCON
    -DUSBD_USE_CDC
    -DHAL_PCD_MODULE_ENABLED
    -DEEPROM_FLASH_PAGE_NUMBER=63
```

These are additions to a complete, reviewed C8 offline configuration, not a
standalone platformio.ini. Preserve the existing feature flags and USB identity.
Use the actual repository root for a local `symlink://` dependency. After
publication use `robertsauer/Elementic@1.1.4`.

The stock Serasidis loader must already be installed. Both BOOT pins remain 0
for software entry. It clears the one-shot DR4 marker itself. The first new
firmware must be loaded through the existing manual HID/ST-Link route; a new
APK cannot add this command to older firmware.

## Paired Android behavior

Android 1.4 development checks the live capability and signed runtime identity,
sends command 8 once, checks RESULT sequence, reserves USB during re-enumeration,
and continues after HID permission. Device > Storage has a confirmed Enter
bootloader action. Lost ACK/timeout never triggers an automatic reset retry.
Old firmware keeps manual bootloader entry. HID block acknowledgement remains
distinct from flash read-back verification.

## Testing and release gate

Compile fixtures: `bluepill_serasidis`, `bluepill_offline`, `offline_v2` in
`test/compile/feature-composition`. Native tests include correct/wrong origin,
stack/reset-vector bounds and frozen command bytes. Remaining physical checks
before relying on automatic entry: command ACK, HID arrival, successful write,
CDC reconnect and unchanged EEPROM. Include denied permission, disconnect,
timeout and unsupported firmware. The owner's publication decision does not
mark these checks complete.

Host regression tests pass (10 passed, one optional build test skipped). The
STM32 HID, STM32 offline and AVR offline V2 compile fixtures pass. Native C++
tests were not executable on the release workstation because host gcc/g++ is
not installed.

V1 wire compatibility and V1 EEPROM reading are retained. The proposed V1
removal or automatic reset of old EEPROM data is not part of this release.

Rollback: select Library 1.1.3 in a successor build configuration and restore
the previous APK. Alternatively omit the opt-in and rebuild 1.1.4. No database,
EEPROM or bootloader migration is required.
