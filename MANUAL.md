# Elementic Library Manual

## Introduction

Elementic is made so a user can start a working device with very little code. The library handles the serial interface, MQTT, EEPROM storage, inputs, outputs, and generic variables.

For a normal Elementic project, the user only needs this in `src/main.cpp`:

```cpp
#include <Arduino.h>

extern const uint16_t ELEMENTIC_DEVICEID = 8; // ESP32-C6

#include <Elementic.h>

void setup() {
  ElementicSetup();
}

void loop() {
  ElementicLoop();

  // Your own code can be added here.
}
```

`ElementicSetup()` starts the Elementic system.

`ElementicLoop()` keeps Elementic running. It should stay inside `loop()` and should be called as often as possible.

The user can add their own simple code after `ElementicLoop()`. That is the easiest place to use Elementic variables such as inputs, outputs, and generic variables.

## Compatibility

Elementic is an Arduino library for PlatformIO projects.

Supported in the current library:

- ESP32
- ESP8266
- AVR boards
- RP2040: Pico, Pico W and W5100S-EVB-Pico
- RP2350: Pico 2 and Pico 2 W

Support is target-profile specific. A successful compile alone is not proof of
EEPROM, MAC override, network or DMX behavior. Consult
`docs/CR-0004-stage-f-hardware-matrix.md` before selecting a production board.

Pico W and Pico 2 W use their CYW43 factory MAC. The Arduino-Pico Wi-Fi API
does not provide a supported station-MAC setter, so Elementic reports the real
hardware address in the runtime `Mac Address` variable instead of claiming that
a configured override was applied. WIZnet Ethernet targets continue to apply
the configured MAC through `Ethernet.begin()`.

The generated-code contract also requires metadata ABI 1 and an idempotent
`elementicLoadFactoryDefaults()`. Manual projects should start from
`extras/manual/ElementicBuildMetadata.cpp`. Numeric wire v1 is the default;
wire v2 is an independent build selection, not a Library/Git version alias.

Required libraries:

- `PubSubClient`
- `Ethernet`

For a manual PlatformIO project, set the maximum number of inputs and outputs in `platformio.ini` so the values apply while compiling both the sketch and the Library:

```ini
build_flags =
    -DELEMENTIC_MAX_INPUTS=15
    -DELEMENTIC_MAX_OUTPUTS=15
```

These limits set the size of the fixed arrays in SRAM. Use values that are large enough for the project without reserving unnecessary channels. Website-generated projects receive the limits from the selected Device profile. Manual projects use explicit defaults for AVR, ESP32, ESP8266 or RP2040 from `ElementicLimits.h` when the macros are omitted. The historical `OutputChannels` and `InputChannels` names remain valid project-wide overrides.

## Functions

### Basic Functions

The two most important functions are:

```cpp
ElementicSetup();
ElementicLoop();
```

Use `ElementicSetup()` once in `setup()`.

Use `ElementicLoop()` in every `loop()`.

Example:

```cpp
void setup() {
  ElementicSetup();
}

void loop() {
  ElementicLoop();

  // Own code here.
}
```

### Using Outputs In Your Own Code

Elementic outputs can be controlled with:

```cpp
OutputValueActual[outputNumber]
```

Output numbers start at `1`.

Example: turn output 1 on:

```cpp
void loop() {
  ElementicLoop();

  OutputValueActual[1] = 1;
}
```

Example: turn output 1 off:

```cpp
void loop() {
  ElementicLoop();

  OutputValueActual[1] = 0;
}
```

Example: set dimmer output 2 to half power:

```cpp
void loop() {
  ElementicLoop();

  OutputValueActual[2] = 128;
}
```

For on/off outputs, use:

- `0` = off
- `1` = on

For dimming outputs, use a value between:

- `0` = off
- `255` = full power

When possible, change `OutputValueActual[]` and let Elementic do the real output handling. That keeps MQTT, serial, memory, and output status working together.

### Reading Outputs

The current output value can also be read from `OutputValueActual[]`.

Example: check if output 1 is on:

```cpp
void loop() {
  ElementicLoop();

  if (OutputValueActual[1] > 0) {
    Serial.println("Output 1 is on");
  }
}
```

### Using Inputs In Your Own Code

Elementic inputs can be read with:

```cpp
SwitchPressed[inputNumber]
```

Input numbers start at `1`.

Example: use input 1 in your own code:

```cpp
void loop() {
  ElementicLoop();

  if (SwitchPressed[1]) {
    Serial.println("Input 1 is pressed");
  }
}
```

You can also trigger an Elementic input from your own code with:

```cpp
SwitchPressedExternal[inputNumber]
```

Example: simulate input 1:

```cpp
void loop() {
  ElementicLoop();

  SwitchPressedExternal[1] = 1; // pressed
}
```

Example: release simulated input 1:

```cpp
void loop() {
  ElementicLoop();

  SwitchPressedExternal[1] = 0; // released
}
```

This is useful when another sensor or condition in your own code should behave like an Elementic input.

### Using Generic Variables

Generic variables are named Elementic values. They can be configured and stored by Elementic, and then used in your own code.

Common read functions:

```cpp
GetStringValue("Name");
GetByteValue("Name");
GetIntValue("Name");
GetPasswordValue("Name");
GetBoolValue("Name");
GetIPValue("Name");
GetPriority("Name");
```

Common define functions:

```cpp
DefineString("Name", "Living room", PriorityCounter++);
DefineByte("Brightness", 128, PriorityCounter++);
DefineInt("DelayMs", 500, PriorityCounter++);
DefineIP("Controller", 192, 168, 1, 10, PriorityCounter++);
DefinePassword("AccessKey", "secret", PriorityCounter++);
DefineBool("AutoLight", true, PriorityCounter++);
```

Define functions are normally used in `genericsetupvariables()`. They create the variables that Elementic can later read, store, send, or update.

In Website-generated projects, `include/ProjectVariablesAutomated.h` is included automatically by `Elementic.h` and selects Wi-Fi, Ethernet and MQTT at compile time. It also defines `ELEMENTIC_MAX_INPUTS`, `ELEMENTIC_MAX_OUTPUTS` and the four name/topic byte limits before the Library allocates its fixed arrays. Reducing these values saves SRAM, especially on AVR. The generated `genericsetupvariables()` then supplies the corresponding runtime defaults. `MQTTEnabled` is a normal Boolean setting for temporarily disabling MQTT; it does not replace the `ELEMENTIC_FEATURE_MQTT` compile-time flag. `Mac Address` is passed to `DefineString` as text and is validated/canonicalized before the Library converts it to hardware bytes.

Example:

```cpp
void genericsetupvariables() {
  DefineString("Room", "Kitchen", PriorityCounter++);
  DefineByte("Brightness", 128, PriorityCounter++);
  DefineInt("DelayMs", 500, PriorityCounter++);
  DefineIP("Controller", 192, 168, 1, 10, PriorityCounter++);
  DefinePassword("AccessKey", "secret", PriorityCounter++);
  DefineBool("AutoLight", true, PriorityCounter++);
}
```

Common set functions:

```cpp
SetStringValue("Room", "Office");
SetByteValue("Brightness", 200);
SetIntValue("DelayMs", 1000);
SetIPValue("Controller", 192, 168, 1, 20);
SetPasswordValue("AccessKey", "new-secret");
SetBoolValue("AutoLight", false);
```

Set functions change an existing variable by name. They return `true` when the variable exists and has the expected type. They return `false` when the variable is missing or has a different type.

Example:

```cpp
void loop() {
  ElementicLoop();

  if (SwitchPressed[1]) {
    SetBoolValue("AutoLight", true);
    SetByteValue("Brightness", 255);
  }
}
```

Example: read a text variable:

```cpp
void loop() {
  ElementicLoop();

  String roomName = GetStringValue("Room");

  if (roomName == "Kitchen") {
    Serial.println("This device is in the kitchen");
  }
}
```

Example: read a true/false variable:

```cpp
void loop() {
  ElementicLoop();

  bool autoLight = GetBoolValue("AutoLight");

  if (autoLight) {
    OutputValueActual[1] = 1;
  }
}
```

Example: read an IP address:

```cpp
void loop() {
  ElementicLoop();

  byte* controllerIP = GetIPValue("Controller");

  Serial.print(controllerIP[0]);
  Serial.print(".");
  Serial.print(controllerIP[1]);
  Serial.print(".");
  Serial.print(controllerIP[2]);
  Serial.print(".");
  Serial.println(controllerIP[3]);
}
```

### Simple Combined Example

This example keeps the standard Elementic setup, then adds a small action in `loop()`.

If input 1 is pressed and the generic variable `AutoLight` is true, output 1 is turned on.

```cpp
#include <Arduino.h>

extern const uint16_t ELEMENTIC_DEVICEID = 8; // ESP32-C6

#include <Elementic.h>

void setup() {
  ElementicSetup();
}

void loop() {
  ElementicLoop();

  if (SwitchPressed[1] && GetBoolValue("AutoLight")) {
    OutputValueActual[1] = 1;
  }
}
```

### Useful Variables

Outputs:

- `OutputValueActual[1]`: wanted value of output 1
- `OutputValueActual[2]`: wanted value of output 2
- `OutputCounter`: number of outputs

Inputs:

- `SwitchPressed[1]`: true when input 1 is pressed
- `SwitchPressedExternal[1]`: software trigger for input 1
- `SwitchCounter`: number of inputs

Generic variables:

- `GetStringValue("Name")`: read a text value
- `GetByteValue("Name")`: read a byte value
- `GetIntValue("Name")`: read an integer value
- `GetPasswordValue("Name")`: read a password value
- `GetBoolValue("Name")`: read a true/false value
- `GetIPValue("Name")`: read an IP address
- `GetPriority("Name")`: read the variable priority
- `DefineByte("Name", value, priority)`: define a byte variable
- `DefineInt("Name", value, priority)`: define an integer variable
- `SetStringValue("Name", value)`: update a text variable
- `SetByteValue("Name", value)`: update a byte variable
- `SetIntValue("Name", value)`: update an integer variable
- `SetIPValue("Name", a, b, c, d)`: update an IP address variable
- `SetPasswordValue("Name", value)`: update a password variable
- `SetBoolValue("Name", value)`: update a true/false variable
