# Elementic

Elementic is an Arduino library for PlatformIO projects. It provides a shared runtime for Elementic devices with serial configuration, dynamic variables, inputs, outputs, MQTT communication, network setup, EEPROM storage, logging, and DMX output support.

## Supported Targets

The current codebase compiles for these tested software environments:

- Controllino / AVR boards with Ethernet
- ESP8266, including ESP-12E
- ESP32, including ESP32-C6
- RP2040, including Raspberry Pi Pico and Pico W
- RP2350, including Raspberry Pi Pico 2 and Pico 2 W
- WIZnet W5100S-EVB-Pico over Ethernet

An environment compile is not a hardware-support claim. Arduino Uno exceeds
its flash/RAM limits. RP2040/RP2350 builds pass the compile matrix, but still
need the physical checks recorded in the
[Stage F hardware matrix](docs/CR-0004-stage-f-hardware-matrix.md).

The library uses compile-time board detection in `deviceIdentification.h` to select WiFi and Ethernet support.

## Installation

When published to the PlatformIO Registry, add Elementic to `lib_deps`:

```ini
lib_deps =
    Elementic
```

For local development, use `lib_extra_dirs` from your project:

```ini
lib_extra_dirs = ../0001 - Elementic Library
```

## Basic Usage

Define the device ID and include `Elementic.h`.

```cpp
#include <Arduino.h>

extern const uint16_t ELEMENTIC_DEVICEID = 20;

#include <Elementic.h>

void setup() {
  ElementicSetup();
}

void loop() {
  ElementicLoop();
}
```

For a manual PlatformIO project, set optional capacity overrides project-wide so the sketch and every Library translation unit allocate the same arrays:

```ini
build_flags =
    -DELEMENTIC_MAX_INPUTS=15
    -DELEMENTIC_MAX_OUTPUTS=15
```

Elementic-generated projects use two protected files. `include/ProjectVariablesAutomated.h` selects project features for every Library translation unit and `src/ProjectVariablesAutomated.cpp` registers generic defaults plus input/output definitions. `Elementic.h` includes the generated header automatically; generated PlatformIO projects additionally use `-include include/ProjectVariablesAutomated.h` so dependency-library translation units receive the same contract.

`ELEMENTIC_MAX_INPUTS` and `ELEMENTIC_MAX_OUTPUTS` size the fixed input/output arrays and therefore directly control SRAM use. Manual sketches may omit them and use the defaults in `ElementicLimits.h`; AVR, ESP32, ESP8266 and RP2040 each have an explicit default profile. The historical `InputChannels` and `OutputChannels` overrides remain supported when they are likewise defined project-wide.

CR-0004 metadata uses the full-width `ELEMENTIC_FIRMWARE_PROJECT_ID`; legacy
`projectid` remains a v1 hint only. Manual projects can copy the
[metadata ABI adapter](extras/manual/ElementicBuildMetadata.cpp). The default
wire producer is numeric v1. Select v2 independently with
`-DELEMENTIC_WIRE_PROTOCOL_VERSION=2` only after compatible Android clients
have been distributed.

## Examples

This package includes examples for:

- `examples/controllino-maxi`
- `examples/esp8266-esp12e`
- `examples/esp32-c6`

The examples are based on existing Elementic projects, but WiFi and MQTT names, usernames, servers, and passwords have been removed. Fill in your own values before connecting to a real network.

## Generated project contract (1.1.0)

A generated project header provides compile-time selection:

```cpp
#define ELEMENTIC_PROJECT_VARIABLES_VERSION 2
#define ELEMENTIC_FEATURE_WIFI 0
#define ELEMENTIC_FEATURE_ETHERNET 1
#define ELEMENTIC_FEATURE_MQTT 1
#define ELEMENTIC_MAX_INPUTS 8
#define ELEMENTIC_MAX_OUTPUTS 12
#define ELEMENTIC_INPUT_NAME_MAX_BYTES 9
#define ELEMENTIC_INPUT_TOPIC_MAX_BYTES 13
#define ELEMENTIC_OUTPUT_NAME_MAX_BYTES 9
#define ELEMENTIC_OUTPUT_TOPIC_MAX_BYTES 13
```

The generated capacity and text limits come from the selected Device profile. Lower values reserve less SRAM; the generated project is rejected if its configured input or output count exceeds the corresponding capacity.

## Version 1.1.1 SRAM optimization

Version 1.1.1 keeps constant Elementic diagnostics and printf-style format
strings in program flash on AVR instead of copying them into SRAM at startup.
Runtime buffers still use the original
`const char*` logging API, while literal call sites use the flash-aware
`ELEMENTIC_LOG`, `ELEMENTIC_SNPRINTF`, and `ELEMENTIC_SPRINTF` helpers.

The V1 and V2 wire formats, MQTT payload text, EEPROM layout, and public
runtime logging overload remain unchanged. ESP8266, ESP32, RP2040, and RP2350
map these helpers to their normal memory model. Generated project names,
topics, and generic-variable defaults are deliberately outside this first
release and remain a possible later optimization.

Its generated source registers runtime defaults:

```cpp
void genericsetupvariables() {
  DefineString("Mac Address", "02:00:00:00:00:01", PriorityCounter++);
  DefineBool("DHCP", true, PriorityCounter++);
  DefineBool("MQTTEnabled", true, PriorityCounter++);
  // Other enabled feature and project variables follow.
}

void inputsetupvariables();
void outputsetupvariables();
```

`ELEMENTIC_FEATURE_MQTT` is the compiled project capability. The dynamic
`MQTTEnabled` value can temporarily disable MQTT without rebuilding. The MAC
address deliberately remains text in generated code and is converted only by
the Library's validated hardware adapter. Headerless 1.x projects using the old
`factory*` and `MQTT*` constants remain supported as a compatibility path.

Pico W and Pico 2 W use the CYW43 factory MAC because the Arduino-Pico Wi-Fi
API does not expose a supported station-MAC setter. At startup, Elementic
replaces the runtime `Mac Address` value with the address actually used by the
radio and emits a caution when it differs from the configured value. ESP and
Ethernet targets continue to use the configured shared MAC.

Compiled defaults, including password literals, are deliberately retained for
offline factory reset. Unit MAC provisioning is stored separately and survives
that reset. The [EEPROM evidence](docs/CR-0004-stage-f-eeprom-evidence.md)
records the exact legacy layout accepted by the bounded reader.

## License

Elementic is released under the MIT License.
