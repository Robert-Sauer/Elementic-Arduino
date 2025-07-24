#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#include <Client.h>          // basis-klasse voor WiFiClient / EthernetClient
#include <PubSubClient.h>


extern Client&      netClient;   // implemented once per sketch
extern PubSubClient mqttClient;  // same for every board

/* Constants */
#define OutputChannels    24
#define InputChannels     19
#define DMX_MAX           10
#define TOTAL_STRINGS     5
#define TOTAL_BYTES       2
#define TOTAL_INTS        2
#define TOTAL_IPS         3
#define TOTAL_PASSWORDS   1
#define MAX_STRING_LENGTH 15

/* Enums */
enum SystemType {
    IDLE = 0, BOOTING, ETHERNET, MQTT, ERROR,
    UPTIME, COMMAND, EEPROMBYTES, RAMBYTES, DEVICETYPE
};

enum CommandType {
    REBOOT = 0, RAMTOEEPROM, EEPROMTORAM, FACTORYSETTINGS, CLEARERROR
};

enum VariableType {
    DYNAMIC_STRING = 0, DYNAMIC_BYTE, DYNAMIC_INT, DYNAMIC_IP, DYNAMIC_PASSWORD
};

/* Variable structures */
union VariableValue {
    char strVal[MAX_STRING_LENGTH];
    uint8_t byteVal;
    int intVal;
    uint8_t ipVal[4];
    char passVal[MAX_STRING_LENGTH];
};

struct DynamicVariable {
    VariableType type;
    String name;
    VariableValue value;
    int priority;
};

/* Global PROGMEM strings */
extern const char onoffset[]            PROGMEM;
extern const char onoffstatus[]         PROGMEM;
extern const char dimvalue_set[]        PROGMEM;
extern const char dimvalue_status[]     PROGMEM;
extern const char color_status[]        PROGMEM;
extern const char color_set[]           PROGMEM;
extern const char defaultvalue_status[] PROGMEM;
extern const char defaultvalue_set[]    PROGMEM;

/* Global variables */
extern uint8_t OutputPin[OutputChannels];
extern uint8_t OutputChannel[OutputChannels];
extern uint8_t OutputRelay[OutputChannels];
extern uint8_t OutputType[OutputChannels];
extern uint8_t Relay[10];
extern uint8_t TotalRelays;
extern uint8_t DMXValue[255];
extern uint8_t DMXTotal;
extern uint8_t OutputValueMemory[OutputChannels];
extern uint8_t OutputValueActual[OutputChannels];
extern uint8_t OutputMinValue[OutputChannels];
extern uint8_t OutputMaxValue[OutputChannels];
extern uint8_t OutputDefaultValue[OutputChannels];
extern uint8_t OutputRedValue[OutputChannels];
extern uint8_t OutputGreenValue[OutputChannels];
extern uint8_t OutputBlueValue[OutputChannels];
extern uint8_t OutputRedMemory[OutputChannels];
extern uint8_t OutputGreenMemory[OutputChannels];
extern uint8_t OutputBlueMemory[OutputChannels];
extern bool MQTTUpdate[OutputChannels];
extern bool OutputRelayUpdate;
extern uint8_t OutputCounter;
extern int SwitchCounter;
extern uint8_t SwitchPin[InputChannels];
extern uint8_t SwitchPressedExternal[InputChannels];
extern uint8_t SwitchPressed[InputChannels];
extern bool SwitchDimDirection[InputChannels];
extern int SwitchTimer[InputChannels];
extern uint8_t InOutMatrixCouter[InputChannels];
extern uint8_t InOutMatrix[InputChannels][5];
extern char MQTT_Output[OutputChannels][28];
extern char MQTT_Input[InputChannels][23];
extern char MQTT_Name[OutputChannels][10];
extern uint8_t dimvalue0;
extern uint8_t state;
extern uint8_t DMXBuffer[DMX_MAX];
extern uint8_t incomingByte;
extern uint8_t incomingdata[500];
extern int data_index;
extern uint8_t databuffersize;
extern int databufferposition;
extern int SerialCommand;
extern int EEPROMStorage;
extern int DMXstartupTimer;
extern int MQTTBurstProtection;
extern char msgmqtt[80];
extern char msgmqtt2[80];

extern const char MQTTName[];
extern const char MQTTServer[];
extern const char MQTTUsername[];
extern const char MQTTPassword[] ;

/* Dynamic variable system */
extern const int totalVariables;
extern DynamicVariable variables[];

/* Function declarations */
extern void input();
extern void output();
extern void serialloop();
extern void serialsetup();
extern void SendSystem(SystemType systemTypeVal, int value);
extern void DefineString(const String& varName, const char val[20], int priority);
extern void DefinePassword(const String& varName, const char* val, int priority);
extern void ReadAllDataFromEEPROM();
extern void SendAllData(bool ER, bool SD, bool MEM);
extern void DMXwrite(int channel, uint8_t value);
extern void DMXflush();
extern void outputsetup();
extern void reconnect();
extern void reconnect();
extern void MQTTSubscribe();
extern void callback(char* topic, byte* payload, unsigned int length) ;
extern void ElementicSetup();
extern void outputsetupvariables();
extern void inputsetupvariables();



/* Helpers */
extern int findVariableIndexByPriority(int targetPriority);
extern String GetStringValue(const String& varName);
extern byte GetByteValue(const String& varName);
extern int GetIntValue(const String& varName);
extern byte* GetIPValue(const String& varName);
extern String GetPasswordValue(const String& varName);
extern int GetPriority(const String& varName);
