# Elementic

Elementic is an Arduino library for PlatformIO projects. It provides a shared runtime for Elementic devices with serial configuration, dynamic variables, inputs, outputs, MQTT communication, network setup, EEPROM storage, logging, and DMX output support.

## Supported Targets

The current codebase is intended for:

- Controllino / AVR boards with Ethernet
- ESP8266, including ESP-12E
- ESP32, including ESP32-C6

The library uses compile-time board detection in `deviceIdentification.h` to select WiFi and Ethernet support.

## Installation

When published to the PlatformIO Registry, add Elementic to `lib_deps`:

```ini
lib_deps =
    rsauer/Elementic
```

For local development, use `lib_extra_dirs` from your project:

```ini
lib_extra_dirs = ../0001 - Elementic Library/Elementic
```

## Basic Usage

Define the device ID and optional channel counts before including `Elementic.h`.

```cpp
#include <Arduino.h>

#define OutputChannels 15
#define InputChannels 15

extern const uint16_t ELEMENTIC_DEVICEID = 20;

#include <Elementic.h>

void setup() {
  ElementicSetup();
}

void loop() {
  ElementicLoop();
}
```

Project-specific variables are usually placed in a separate `ProjectVariablesAutomated.cpp` file. This file provides values such as `projectid`, MQTT defaults, factory network settings, and input/output definitions.

## Examples

This package includes examples for:

- `examples/controllino-maxi`
- `examples/esp8266-esp12e`
- `examples/esp32-c6`

The examples are based on existing Elementic projects, but WiFi and MQTT names, usernames, servers, and passwords have been removed. Fill in your own values before connecting to a real network.

## Required Project Definitions

A project normally provides these symbols:

```cpp
extern const uint16_t ELEMENTIC_DEVICEID;
const long projectid;
const int device_type;

const char MQTTName[];
const char MQTTServer[];
const char MQTTUsername[];
const char MQTTPassword[];

const uint8_t factorymac[6];
const uint8_t factoryip[4];
const uint8_t factorygateway[4];
const uint8_t factorysubnet[4];
const char factoryhostname[];
const char factorySSID[];
const char factoryWIFIPassword[];

void genericsetupvariables();
void inputsetupvariables();
void outputsetupvariables();
```

## License

Elementic is released under the MIT License.
