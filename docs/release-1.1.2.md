# Elementic 1.1.2 release notes

## Unassigned input safety

Elementic 1.x uses numeric pin value `0` as the current unassigned-pin
sentinel. The input scanner now skips these entries before calling
`digitalRead()`. This aligns input scanning with input initialization and with
the existing output and relay handling.

The change prevents an unassigned input from accidentally observing GPIO 0.
On AVR targets such as ATmega2560, digital pin 0 is also Serial RX and could
otherwise turn incoming Elementic protocol traffic into false switch events.

## Compatibility

- Wire protocols V1 and V2 are unchanged.
- EEPROM/storage layouts are unchanged.
- Generated projects and existing stored configurations remain compatible.
- GPIO 0 is temporarily unavailable as an Elementic input pin.
- A future coordinated contract revision will use a distinct unassigned-pin
  sentinel, provisionally `255` in the Library runtime, so GPIO 0 can become a
  valid explicit pin without increasing the existing `uint8_t` pin arrays.
