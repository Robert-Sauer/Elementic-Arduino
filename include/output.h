#pragma once
#include <stdint.h>
#include "mqttstrings.h"
// extern const char onoffset[];
// extern const char onoffstatus[];
// extern const char dimvalue_set[];
// extern const char dimvalue_status[];
// extern const char color_status[];
// extern const char color_set[];
// extern const char defaultvalue_status[];
// extern const char defaultvalue_set[];

// // Max counts should be defined somewhere shared
// // #define OUTPUTCHANNELS ...
// // #define TOTAL_RELAYS ...
// // #define MAX_MQTT_TOPIC_LENGTH ...
// // #define MAX_NAME_LENGTH ...
// // #define MAX_CHANNELS_PER_OUTPUT ...

// // MQTT
// extern char MQTT_Output[][28];           // [OUTPUTCHANNELS + 1][28]

// // Output value arrays
// extern uint8_t OutputValueActual[];      // [OUTPUTCHANNELS + 1]
// extern uint8_t OutputValueMemory[];      // [OUTPUTCHANNELS + 1]
// extern uint8_t OutputDefaultValue[];     // [OUTPUTCHANNELS + 1]
// extern uint8_t OutputMinValue[];         // [OUTPUTCHANNELS + 1]
// extern uint8_t OutputMaxValue[];         // [OUTPUTCHANNELS + 1]

// // Types, pins, relays
// extern uint8_t OutputType[];             // [OUTPUTCHANNELS + 1]
// extern uint8_t OutputPin[];              // [OUTPUTCHANNELS + 1]
// extern uint8_t OutputRelay[];            // [OUTPUTCHANNELS + 1]
// extern uint8_t OutputChannel[];          // [OUTPUTCHANNELS + 1]
// extern char     MQTT_Name[][10];         // [OUTPUTCHANNELS + 1][10]

// // Status flags
// extern bool MQTTUpdate[];                // [OUTPUTCHANNELS + 1]
// extern bool OutputRelayUpdate;
// extern uint8_t  OutputCounter;
// extern int  DMXstartupTimer;

// // Relay control
// extern uint8_t Relay[];                  // [TOTAL_RELAYS]

// // MQTT temp strings
// extern char msgmqtt[];                   // [80]
// extern char msgmqtt2[];                  // [80]

// // Doorbell
// extern bool DoorBellEnabled;
// extern uint8_t DoorBellRingOutput;
// extern uint8_t DoorBellLightOutput;
// extern int DoorBellCounter;

// // General burst protection
// extern int MQTTBurstProtection;


