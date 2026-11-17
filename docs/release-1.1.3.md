# Elementic 1.1.3 release notes

Release date: **17 November 2026**.

Publication authorized for GitHub and the PlatformIO Registry. Extended physical
acceptance remains tracked below and must not be inferred from publication.
STM32 (`ststm32`) is now included in the package's supported platform metadata;
the offline STM32F103C8 Blue Pill build and Windows USB CDC checks are recorded.
This does not add STM32/ST-Link flashing to the Android app.

This version separates compile-time inclusion from runtime enablement for
Wi-Fi, Ethernet and MQTT. Matter, WLED/addressable LEDs and 1-Wire remain
future extensions. The wire producer version and EEPROM schema remain
independent of the Library version and have not been incremented.

## Feature contract

| Feature state | Driver/client and dedicated buffers | Network activity | Settings and topics in new EEPROM saves |
| --- | --- | --- | --- |
| Hard excluded | Omitted | None for this feature | Omitted |
| Compiled in, soft disabled | Retained | Stopped | Retained |
| Compiled in, soft enabled | Retained | Allowed | Retained |

Select the compiled feature set in the project-wide header or build flags:

```cpp
#define ELEMENTIC_PROJECT_VARIABLES_VERSION 2
#define ELEMENTIC_FEATURE_WIFI 0
#define ELEMENTIC_FEATURE_ETHERNET 1
#define ELEMENTIC_FEATURE_MQTT 1
```

The effective macros combine board/driver capability and the project choice:
`ELEMENTIC_USE_WIFI`, `ELEMENTIC_USE_ETHERNET`,
`ELEMENTIC_USE_NETWORK` and `ELEMENTIC_USE_MQTT`.
`ELEMENTIC_FEATURE_COMPOSITION_VERSION=1` identifies this implementation to
generated source, allowing the same generator to retain older Library support.

Runtime switches are ordinary persisted Bool variables named `WiFiEnabled`,
`EthernetEnabled` and `MQTTEnabled`. Missing switches retain the historical
enabled fallback. The generator supplies missing transport switches, guarded
so older Libraries do not consume additional Bool slots. Explicit false
values are preserved. The Bool pool reserves one additional slot per compiled
transport; the generic String/IP/password pools remain available for user
variables and are not automatically reduced.

Ethernet soft-off stops Elementic's connection attempts and reports the
transport unavailable; it does not promise to power down a PHY. Wi-Fi
soft-off disconnects the station and disables automatic reconnection.
MQTT soft-off disconnects the existing client. Connection cleanup remains
reachable from `ElementicLoop()` even after a switch becomes false.

Existing board-driver support and the current single-transport project policy
continue to apply. Declaring a feature does not add a driver to an unsupported
board or introduce automatic multi-interface failover.

## Actual replacement sites

These snippets use existing implementation names. The earlier illustrative
`writeOutputTopic()`, `writeInputTopic()`, `setupWiFi()` and
`setupEthernet()` procedures were not real functions.

### Drivers and clients

Before, detected hardware could still compile driver paths that the project
had excluded. Driver includes, helpers and retry state in
`externalInterfaceOperation.cpp` now use the effective feature macros:

```cpp
#if ELEMENTIC_USE_ETHERNET
#include <Ethernet.h>
#include <SPI.h>
// Ethernet helpers and state are defined within this branch.
#endif
```

`elementic_defaults.cpp` now constructs a WiFiClient or EthernetClient only
when MQTT actually needs it. The former offline NullClient is removed.
In `Elementic.cpp` the actual MQTT allocation is conditional:

```cpp
#if ELEMENTIC_USE_MQTT
PubSubClient mqttClient(netClient);
#endif
```

Topic arrays, MQTT update flags and topic suffix constants are conditional in
both their definitions and public declarations. A hard-off build therefore
has no MQTT client or PubSubClient packet buffer. Compatibility procedures
remain callable as small no-op/false implementations where appropriate.

### Network execution

`setupNetwork()`, `connectNetwork()` and `networkCheckLoop()` have effective
compile guards. The existing connection helpers also enforce runtime state:

```cpp
static void connectToWiFi() {
  if (!elementicWifiEnabled()) return;
  // Existing Wi-Fi connection implementation follows.
}

static void connectToEthernet() {
  if (!elementicEthernetEnabled()) return;
  // Existing Ethernet connection implementation follows.
}
```

`networkCheckLoop()` checks switch transitions before retry delays, clears
stale connection state and makes re-enablement eligible for reconnect.
Wi-Fi uses Hostname independently of MQTT, with the historical MQTTName
fallback only when MQTT is compiled in.

`MQTTSetupClient()` initializes its callback and packet capacity even when
MQTT starts softly disabled. `MQTTLoop()` owns disconnect/reconnect handling.
All normal input/output publish paths check runtime availability through
`elementicMqttPublish()` or `SendOutputValueMQTT()`. Local GPIO, relay,
dimming and serial feedback paths remain operational with MQTT excluded.

### EEPROM topics

In `ElementicStorage.cpp::emitConfiguration()`, the real record writers are:

```cpp
#if ELEMENTIC_USE_MQTT
emitText(writer, 0U, 7U, item,
         MQTT_Output[item], sizeof(MQTT_Output[item]));
#endif

#if ELEMENTIC_USE_MQTT
emitText(writer, 1U, 2U, item,
         MQTT_Input[item], sizeof(MQTT_Input[item]));
#endif
```

These writes depend on compile-time inclusion, never on the soft switch.
Modern and legacy readers consume excluded topic records without accessing
removed arrays. Legacy generic records now resolve the stored name and type
against registered variables, as schema 2 already does. This prevents a
removed feature or shifted registration index from restoring an unrelated
value or recreating a removed setting. Readers consume the original field
width even when a saved name/value is truncated or ignored.

The standard network keys are reserved by the framework. Define* registration
ignores keys belonging to hard-excluded features, including calls left in
older generated/manual code. New saves serialize the current registry.
The provisioning region and per-unit identity layout remain intact.

### Serial V1/V2

Runtime snapshots omit excluded MQTT topic records and report effective topic
limits as zero. V2 INFO reads MQTT status through a hard-off-safe helper and
does not advertise MQTT external output types 5/6 in an MQTT-free build.
Nonempty unsupported topic writes are rejected; empty topic writes are
accepted as no-ops for compatibility with existing App add/edit flows.
Legacy readers retain field alignment and existing field identifiers.

The saved-EEPROM snapshot remains a view of the actual saved image, including
historical records until the user explicitly saves the new configuration.
A hard-off save discards those old feature-only fields; reintroducing the
feature later cannot recover fields discarded by that save. The project
provides the backup defaults.

### Central logging

All active diagnostics, including legacy diagnostics, pass through
`logging(LogLevel, const char*)` or its AVR flash-string overload.
Serial framing stays in `sendSerialLog()`. The MQTT sink and its formatting
helpers compile only with MQTT and publish only when logging is enabled,
MQTT is softly enabled, and an MQTT session already exists. Logging never
opens a network or broker connection. Protected values are redacted.

The shared `msgmqtt` and `msgmqtt2` scratch buffers remain because serial
diagnostics use them independently of MQTT.

### USB CDC frame completion

Protocol announcements and V2 frames now check the byte count returned by
`Serial.write(buffer, length)`. A positive partial write is completed before a
new frame may start, with a 250 ms upper bound. This is required for STM32duino
USB CDC, where a temporarily busy Android host can cause a short write. An
initial zero-byte write still fails immediately, so firmware without an
attached host does not wait for that bound. Android 1.4 supplies the paired
receiver resynchronization behavior for an already-truncated stream.

## Dependencies and generated projects

The package no longer installs Ethernet and PubSubClient unconditionally.
Network projects must list the required libraries explicitly in their
PlatformIO Build Configuration, as the bundled network examples already do:

```ini
lib_deps =
    robertsauer/Elementic@1.1.3
    arduino-libraries/Ethernet@^2.0.0
    knolleary/PubSubClient@^2.8.0
lib_ldf_mode = chain
```

Omit Ethernet for Wi-Fi-only projects; omit PubSubClient without MQTT; omit
both for offline projects. Wi-Fi comes from the selected Arduino core. The Library requests standard
LDF scanning (chain) because conditional scanning can miss core Wi-Fi headers.
The compiler guards own firmware composition; an installed unused dependency
may still produce an unlinked archive. It does not contribute clients or
buffers to the firmware.
Use the exact dependency `robertsauer/Elementic@1.1.3` for reproducible builds.

Existing immutable server Build Configurations are not rewritten or
automatically upgraded. An administrator chooses a successor configuration
with the appropriate dependencies. Existing snapshots may still explicitly
install an unused package; compile guards prevent its code and objects from
entering Elementic's firmware paths. This work does not remove dependencies
needed by user-owned code.

The Laravel generator guards feature-specific defaults and topic assignments.
Older Libraries bypass those new guards and do not receive extra transport
switches. Manual projects that directly reference topic arrays must similarly
guard those references when opting into hard MQTT exclusion.

For tests/examples nested inside the symlinked Library repository, keep
`PLATFORMIO_LIBDEPS_DIR` outside that repository. Otherwise PlatformIO can
classify nested dependency headers as belonging to Elementic itself.

## Verification and remaining hardware validation

The 17 November publication check passes all six AVR composition builds,
the 15 existing host tests (including four ELF symbol checks), ESP8266 V2,
ESP32-C6, Pico 2 W, WIZnet W5100S-EVB-Pico and the new compile-only
`bluepill_offline` fixture. The Blue Pill
fixture uses STM32 platform 20.0.0 / Arduino core 3.0.0 with USB CDC enabled,
normal soft library compatibility and no network dependencies: 8272 bytes
static RAM and 47460 bytes flash. This fixture's capacities/defaults differ
from the earlier project-0012 hardware image; memory figures are not an
identical-project comparison. No board is programmed by the publication check.

The six-variant AVR fixture is `test/compile/feature-composition`: offline
V1/V2, Ethernet-only, Ethernet+MQTT V1/V2, and both runtime switches false.
Its inputs and outputs are real configured items, with equal capacity limits
across variants. Binary tests in
`test/host/test_feature_composition_builds.py` inspect the resulting ELF
symbols for removed clients/arrays and retained soft-off storage.

Software compilation is not hardware validation. The STM32F103C8 offline
project and all six AVR composition environments compile after the USB CDC
change, and the Android 1.4 unit suite covers truncated-frame recovery. The
final rebuilt Blue Pill-to-Android live test remains required. Step 5 must also exercise
switching off/on from Android, no connection attempts while disabled, input
and relay operation offline, EEPROM store/restart in soft-off state,
historical schema-1/schema-2 migration, saved snapshot behavior and reconnect
after restore/factory reset. Measure heap and stack peaks on representative
hardware. The existing software matrix is recorded in the CR-0004 evidence;
remaining physical checks continue after this explicitly authorized publication.

Publication does not automatically update immutable server Build Configurations.
Select a successor configuration with this exact Library version and the
appropriate optional dependencies when testing server builds. Android 1.4 is
the paired receiver implementation for truncated V2 frame recovery.
