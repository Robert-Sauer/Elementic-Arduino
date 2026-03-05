#pragma once
#include <Arduino.h>
extern const uint16_t ELEMENTIC_DEVICEID;   // user defines in sketch
#include <stdint.h>
#include <stdbool.h>

#include <Client.h>          // basis-klasse voor WiFiClient / EthernetClient
#include <PubSubClient.h>
#include <string.h>


extern Client&      netClient;   // implemented once per sketch
extern PubSubClient mqttClient;  // same for every board


/* Constants */

#ifndef OutputChannels
#define OutputChannels 5   // default
#endif

#ifndef InputChannels
#define InputChannels 10    // default
#endif

#define DMX_MAX           10
#define TOTAL_STRINGS     6
#define TOTAL_BYTES       2
#define TOTAL_INTS        2
#define TOTAL_IPS         3
#define TOTAL_PASSWORDS   2
#define MAX_STRING_LENGTH 64

#define MAXNAMELENGTH     15
#define MAXTOPICLENGTH    28
#define INOUT_MATRIX_COLS  5

/* Enums */
enum SystemType {
    IDLE = 0, BOOTING, ETHERNET, MQTT, ERROR,
    UPTIME, COMMAND, EEPROMBYTES, RAMBYTES, DEVICETYPE,
    NUMOUTPUTS, MAXOUTPUTS, NUMINPUTS, MAXINPUTS, DELETEOUTPUT, DELETEINPUT, MAXCHARNAME, MAXCHARTOPIC
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

/* Global PROGMEM Int */

extern const int device_type    PROGMEM;

/* Global variables */
extern uint8_t OutputPin[OutputChannels+1];
extern uint8_t OutputChannel[OutputChannels+1];
extern uint8_t OutputRelay[OutputChannels+1];
extern uint8_t OutputType[OutputChannels+1];
extern uint8_t Relay[10];
extern uint8_t TotalRelays;
extern uint8_t DMXValue[255];
extern uint8_t DMXTotal;
extern uint8_t OutputValueMemory[OutputChannels+1];
extern uint8_t OutputValueActual[OutputChannels+1];
extern uint8_t OutputMinValue[OutputChannels+1];
extern uint8_t OutputMaxValue[OutputChannels+1];
extern uint8_t OutputDefaultValue[OutputChannels+1];
extern uint8_t OutputRedValue[OutputChannels+1];
extern uint8_t OutputGreenValue[OutputChannels+1];
extern uint8_t OutputBlueValue[OutputChannels+1];
extern uint8_t OutputRedMemory[OutputChannels+1];
extern uint8_t OutputGreenMemory[OutputChannels+1];
extern uint8_t OutputBlueMemory[OutputChannels+1];
extern bool MQTTUpdate[OutputChannels+1];
extern int OutputSignalCountdown[OutputChannels+1];
extern int blinkingTimer;
extern bool OutputRelayUpdate;
extern uint8_t OutputCounter;
extern int SwitchCounter;
extern uint8_t SwitchPin[InputChannels+1];
extern uint8_t SwitchPressedExternal[InputChannels+1];
extern uint8_t SwitchPressed[InputChannels+1];
extern bool SwitchDimDirection[InputChannels+1];
extern int SwitchTimer[InputChannels+1];
extern uint8_t InOutMatrixCouter[InputChannels+1];
extern uint8_t InOutMatrix[InputChannels+1][INOUT_MATRIX_COLS];
extern char MQTT_Output[OutputChannels+1][MAXTOPICLENGTH];
extern char MQTT_Input[InputChannels+1][MAXTOPICLENGTH];
// extern char MQTT_Name[OutputChannels][10]; //27-11-2025 overbodig??
extern char Output_Name[OutputChannels+1][MAXNAMELENGTH];
extern char Input_Name[InputChannels+1][MAXNAMELENGTH];
extern uint8_t dimvalue0;
extern uint8_t state;
extern uint8_t DMXBuffer[DMX_MAX];
extern uint8_t incomingByte;
extern uint8_t incomingdata[500];
extern int data_index;
extern uint8_t databuffersize;
extern int databufferposition;
extern int SerialCommand;
extern int DeleteInput;
extern int DeleteOutput;
extern int EEPROMStorage;
extern int DMXstartupTimer;
extern int MQTTBurstProtection;
extern char msgmqtt[80];
extern char msgmqtt2[80];
extern int loopcounter;
extern int loopcounter2;
extern int loopcounter3;

extern bool logtoMQTT;
extern bool logtoSerial;
extern bool WIFIConnected;
extern bool EthernetConnected;
extern bool networkConnected;



extern const char MQTTName[];
extern const char MQTTServer[];
extern const char MQTTUsername[];
extern const char MQTTPassword[] ;

extern const uint8_t     factorymac[];
extern const uint8_t      factoryip[];
extern const uint8_t factorygateway[]; 
extern const uint8_t  factorysubnet[]; 
extern const char   factoryhostname[]; 

/* Dynamic variable system */
extern const int totalVariables;
extern DynamicVariable variables[];
extern byte PriorityCounter;

/* Function declarations */
extern void input();
extern void output();
extern void serialloop();
extern void ElementicLoop();
extern void serialsetup();
extern void SendOutputValueSerial(byte id, byte value);
extern void SendOutputValueMQTT(byte id);
extern void SendSystem(SystemType systemTypeVal, int value);
extern void SendSystemInBatch(SystemType systemTypeVal, int value);
extern void DefineString(const String& varName, const char val[20], int priority);
extern void DefinePassword(const String& varName, const char* val, int priority);
extern void DefineIP(const String& varName, byte ip1, byte ip2, byte ip3, byte ip4, int priority);

extern void ReadAllDataFromEEPROM();
extern void SendAllData(bool ER, bool SD, bool MEM);
extern void DMXwrite(int channel, uint8_t value);
extern void DMXflush();
extern void outputsetup();
extern void reconnect();
extern void reconnect();
extern void MQTTSubscribe();
extern void MQTTLoop();
extern void MQTTSetup();
extern void callback(char* topic, byte* payload, unsigned int length) ;
extern void ElementicSetup();
extern void outputsetupvariables();
extern void inputsetupvariables();
extern void genericsetupvariables();
extern void connectNetwork();
extern int freeRam();
// extern void logging(LogLevel level, char text[50]);
extern void serialwrite(byte i);
extern void networkCheckLoop();
extern void connectNetwork();
extern void setupNetwork();




static inline bool startsWith(const char* s, const char* prefix) {
  if (!s || !prefix) return false;
  size_t n = strlen(prefix);
  return strncmp(s, prefix, n) == 0;
}

#if defined(ARDUINO_ARCH_AVR)

  #include <avr/pgmspace.h>

  static inline bool streq_P(const char* ram, PGM_P pgm) {
    return strcmp_P(ram, pgm) == 0;
  }

#elif defined(ESP8266)

  #include <pgmspace.h>

  static inline bool streq_P(const char* ram, PGM_P pgm) {
    return strcmp_P(ram, pgm) == 0;
  }

#else

  // On ESP32 and most other modern cores PROGMEM is RAM-mapped
  #ifndef PROGMEM
    #define PROGMEM
  #endif

  static inline bool streq_P(const char* ram, const char* pgm) {
    return strcmp(ram, pgm) == 0;
  }

#endif




/* Helpers */
extern int findVariableIndexByPriority(int targetPriority);
extern String GetStringValue(const String& varName);
extern byte GetByteValue(const String& varName);
extern int GetIntValue(const String& varName);
extern byte* GetIPValue(const String& varName);
extern String GetPasswordValue(const String& varName);
extern int GetPriority(const String& varName);

enum LogLevel : uint8_t { LOG_INFO=0, LOG_CAUTION=1, LOG_WARNING=2 };
//extern void logging(LogLevel level, char text[50]); 2026-02-14
extern void logging(LogLevel level, const char* text);
extern bool logtoMQTT;
extern bool logtoSerial;