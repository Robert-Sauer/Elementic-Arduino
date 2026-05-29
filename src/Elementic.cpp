/*
 * Elementic.cpp
 *
 * Defines the shared global storage for the library and contains the core setup and runtime coordination helpers.
 */

#include <Arduino.h>
#include <EEPROM.h>
#include "Elementic.h"

// tell the CPP that netClient is created elsewhere
extern Client& netClient;
PubSubClient mqttClient(netClient);   // ONE definition

unsigned long previousMillis = 0;
const unsigned long interval = 2000; // 2 seconds in milliseconds

const unsigned long InputInterval  = 4; //milliseconds
const unsigned long OutputInterval = 10; //milliseconds
const unsigned long CheckNetworkInterval = 5000; //milliseconds
const unsigned long DMXInterval = 10; //milliseconds;

//const int device_type = 0;

namespace {
void ClearEEPROMForFactorySettings() {
  const int eepromLength = EEPROM.length();
  for (int i = 0; i < eepromLength; i++) {
    EEPROM.write(i, 0xFF);
  }

  #if defined(ESP8266) || defined(ESP32)
    EEPROM.commit();
  #endif

  EEPROMStorage = 0;
}

void RestartDevice() {
  delay(100);

  #if defined(ESP8266) || defined(ESP32)
    ESP.restart();
  #elif defined(ARDUINO_ARCH_AVR)
    void (*resetFunc)(void) = 0;
    resetFunc();
  #elif defined(NVIC_SystemReset)
    NVIC_SystemReset();
  #endif
}
}

// Internal timers
unsigned long LastInputInterval  = 0;
unsigned long LastOutputInterval = 0;
unsigned long LastOutput10msInterval = 0;
unsigned long LastCheckNetworkInterval = 0;
unsigned long LastDMXInterval = 0;


/* ──────────────────────────────
 *  PROGMEM-constanten
 * ────────────────────────────── */
const char onoffset[]            PROGMEM = "/onoff_set";
const char onoffstatus[]         PROGMEM = "/onoff_status";
const char dimvalue_set[]        PROGMEM = "/dimvalue_set";
const char dimvalue_status[]     PROGMEM = "/dimvalue_status";
const char color_status[]        PROGMEM = "/color_status";
const char color_set[]           PROGMEM = "/color_set";
const char defaultvalue_status[] PROGMEM = "/defaultvalue_status";
const char defaultvalue_set[]    PROGMEM = "/defaultvalue_set";

/* ──────────────────────────────
 *  Globale data – **definities**
 * ────────────────────────────── */
uint8_t  OutputPin[OutputChannels+1]          = {0};
uint8_t  OutputChannel[OutputChannels+1]      = {0};
uint8_t  OutputRelay[OutputChannels+1]        = {0};
uint8_t  OutputType[OutputChannels+1]         = {0};
uint8_t  Relay[10]                          = {0};
uint8_t  TotalRelays                        = 0;

uint8_t  DMXValue[255]                      = {0};
uint8_t  DMXTotal                           = 0;

uint8_t  OutputValueMemory[OutputChannels+1]      = {0};
uint8_t  OutputValueActual[OutputChannels+1]      = {0};
uint8_t  OutputMinValue[OutputChannels+1]         = {0};
uint8_t  OutputMaxValue[OutputChannels+1]         = {0};
uint8_t  OutputDefaultValue[OutputChannels+1]     = {0};
uint8_t  OutputRedValue[OutputChannels+1]         = {0};
uint8_t  OutputGreenValue[OutputChannels+1]       = {0};
uint8_t  OutputBlueValue[OutputChannels+1]        = {0};
uint8_t  OutputRedMemory[OutputChannels+1]        = {0};
uint8_t  OutputGreenMemory[OutputChannels+1]      = {0};
uint8_t  OutputBlueMemory[OutputChannels+1]       = {0};
int      OutputSignalCountdown[OutputChannels+1]  = {0};
int      blinkingTimer                          = 50; //value per 10 ms

bool     MQTTUpdate[OutputChannels+1]         = {false};
uint8_t  OutputStatus[OutputChannels+1] = {0};
bool     OutputRelayUpdate                  = false;

uint8_t  OutputCounter                      = 0;
int  SwitchCounter                      = 0;
uint8_t  SwitchPin[InputChannels+1]           = {0};

uint8_t  SwitchPressedExternal[InputChannels+1] = {0};
uint8_t  SwitchPressed[InputChannels+1]         = {0};
bool     SwitchDimDirection[InputChannels+1]    = {false};
int      SwitchTimer[InputChannels+1]           = {0};

uint8_t  InOutMatrixCouter[InputChannels+1]     = {0};
uint8_t  InOutMatrix[InputChannels+1][INOUT_MATRIX_COLS]        = {{0}};
uint8_t  InputStatus[InputChannels+1]          = {0};

char     MQTT_Output[OutputChannels+1][MAXTOPICLENGTH]      = {{0}};
char     MQTT_Input[InputChannels+1][MAXTOPICLENGTH]        = {{0}};
// char     MQTT_Name[OutputChannels][10]        = {{0}}; //stil required?

char Output_Name[OutputChannels+1][MAXNAMELENGTH]  = {{0}};
char Input_Name[InputChannels+1][MAXNAMELENGTH] = {{0}};

uint8_t  dimvalue0           = 0;
uint8_t  state               = 0;
uint8_t  DMXBuffer[DMX_MAX]  = {0};

uint8_t  incomingByte              = 0;
uint8_t  incomingdata[500]         = {0};
int      data_index                = 0;
uint8_t  databuffersize            = 0;
int      databufferposition        = 0;
int      SerialCommand             = 0;
int      DeleteInput               = 0;
int      DeleteOutput              = 0;
int      EEPROMStorage             = 0;
int      DMXstartupTimer           = 0;
int      MQTTBurstProtection       = 0;
char msgmqtt[80];
char msgmqtt2[80];
byte PriorityCounter = 1;
long lastReconnectAttempt;

/* ──────────────────────────────
 *  Dynamic-variable tabel
 * ────────────────────────────── */
 const int totalVariables = TOTAL_STRINGS + TOTAL_BYTES + TOTAL_INTS
                          + TOTAL_IPS     + TOTAL_PASSWORDS + TOTAL_BOOLS;

/* Helpers */
int findVariableIndexByPriority(int targetPriority);
String GetStringValue(const String& varName);
byte GetByteValue(const String& varName);
int GetIntValue(const String& varName);
byte* GetIPValue(const String& varName);
String GetPasswordValue(const String& varName);
int GetPriority(const String& varName);
bool GetBoolValue(const String& varName);

uint8_t DynamicVariableStatus[TOTAL_DYNAMICVARIABLES];
DynamicVariable variables[totalVariables] = {};   // leeg init; zelf vullen in setup()
bool DynamicVariablesUpdated = false;

void ElementicSetup(){
  Serial.begin(115200);

  // Give USB-serial time to stabilize
  delay(100);

  // Clear any boot garbage still in RX buffer
  while (Serial.available()) {
    Serial.read();
  }

  #if defined(ESP32) || defined(ESP8266)
    EEPROM.begin(2048);     // Only needed on ESP platforms
  #endif

  serialsetup();
  setupNetwork();
  logging(LOG_INFO,"Booting. . .");

  #if defined(ESP8266)
    snprintf(msgmqtt2, sizeof(msgmqtt2), "Moving output: %s", ESP.getResetReason().c_str());
    logging(LOG_INFO,msgmqtt2);
  #endif

  SendSystem(BOOTING, 1);
  SendSystem(IDLE, 0);
  SendSystem(MQTT, 0);

  DMXstart();

  genericsetupvariables();
  MQTTSetupVariables();
  outputsetupvariables();
  inputsetupvariables();
  ReadAllDataFromEEPROM();
  setrelays();
  SendSystem(DEVICETYPE, ELEMENTIC_DEVICEID);
  connectNetwork();
  // logging(LOG_INFO,"After connect network. . .");
  MQTTSetupClient();
  // logging(LOG_INFO,"After MQTT Setup. . .");

  SendSystem(BOOTING, 0);
  SendSystem(IDLE, 1);
  SendSystem(EEPROMBYTES, EEPROMStorage);
  SendSystem(RAMBYTES, freeRam());
  SendSystem(PROJECTID, (int)projectid);
  
}

 // ---- Helpers (put at top of the .cpp with your callback) ----

// ---- Your callback (drop-in replacement) ----

void ElementicLoop(){
  serialloop();
  networkCheckLoop();

  if (networkConnected) {
    MQTTLoop();
  }

  unsigned long currentMillis = millis();

  switch (SerialCommand) { // RAM/EEPROM Status, consider generic loop for Elementic
  case RAMTOEEPROM:
    // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", GetStringValue("MQTTName").c_str(), "/LOG");
    // mqttClient.publish(msgmqtt, "Store Data to EEPROM");
    logging(LOG_INFO,"Store Data to EEPROM");
    SendAllData(true, false, true);
    break;
  case EEPROMTORAM:
    // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", GetStringValue("MQTTName").c_str(), "/LOG");
    // mqttClient.publish(msgmqtt, "Restore Data from EEPROM");
    logging(LOG_INFO,"Restore Data from EEPROM");
    ReadAllDataFromEEPROM();
    break;
  case FACTORYSETTINGS:
    logging(LOG_INFO,"Factory settings: clear EEPROM and restart");
    ClearEEPROMForFactorySettings();
    SendSystem(EEPROMBYTES, EEPROMStorage);
    SerialCommand = 0;
    RestartDevice();
    break;
  case CLEARERROR:
    break;
  }

  if (DeleteOutput>0){
    if (DeleteOutput < 1 || DeleteOutput > OutputCounter) return;

    //int i0 = DeleteOutput - 1; // 0-based
    snprintf(msgmqtt2, sizeof(msgmqtt2), "Deleting output: %d", DeleteOutput);
    logging(LOG_INFO,msgmqtt2);

    for (int i = DeleteOutput; i < OutputCounter; i++)
    {
        // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/LOG2");
        snprintf(msgmqtt2, sizeof(msgmqtt2), "Moving output: %d", i);
        logging(LOG_INFO,msgmqtt2);
        // Shift text arrays
        strcpy(Output_Name[i],        Output_Name[i + 1]);
        strcpy(MQTT_Output[i],        MQTT_Output[i + 1]);

        // Shift numeric arrays
        OutputPin[i]          = OutputPin[i + 1];
        OutputChannel[i]      = OutputChannel[i + 1];
        OutputRelay[i]        = OutputRelay[i + 1];
        OutputType[i]         = OutputType[i + 1];
        OutputValueMemory[i]  = OutputValueMemory[i + 1];
        OutputValueActual[i]  = OutputValueActual[i + 1];
        OutputMinValue[i]     = OutputMinValue[i + 1];
        OutputMaxValue[i]     = OutputMaxValue[i + 1];
        OutputDefaultValue[i] = OutputDefaultValue[i + 1];
        OutputRedValue[i]     = OutputRedValue[i + 1];
        OutputGreenValue[i]   = OutputGreenValue[i + 1];
        OutputBlueValue[i]    = OutputBlueValue[i + 1];
        OutputRedMemory[i]    = OutputRedMemory[i + 1];
        OutputGreenMemory[i]  = OutputGreenMemory[i + 1];
        OutputBlueMemory[i]   = OutputBlueMemory[i + 1];
        MQTTUpdate[i]         = MQTTUpdate[i + 1];
    }

    // Clean last entry (optional)
    Output_Name[OutputCounter][0] = '\0';
    MQTT_Output[OutputCounter][0] = '\0';
    OutputPin[OutputCounter] = 0;
    OutputChannel[OutputCounter] = 0;
    OutputRelay[OutputCounter] = 0;
    OutputType[OutputCounter] = 0;
    OutputValueMemory[OutputCounter] = 0;
    OutputValueActual[OutputCounter] = 0;
    OutputMinValue[OutputCounter] = 0;
    OutputMaxValue[OutputCounter] = 0;
    OutputDefaultValue[OutputCounter] = 0;
    OutputRedValue[OutputCounter] = 0;
    OutputGreenValue[OutputCounter] = 0;
    OutputBlueValue[OutputCounter] = 0;
    OutputRedMemory[OutputCounter] = 0;
    OutputGreenMemory[OutputCounter] = 0;
    OutputBlueMemory[OutputCounter] = 0;
    MQTTUpdate[OutputCounter] = 0;

    OutputCounter--;
    DeleteOutput = 0;
  }
  if (DeleteInput>0){
        if (DeleteInput < 1 || DeleteInput > SwitchCounter) return;

    for (int i = DeleteInput; i < SwitchCounter ; i++)
    {
      //  snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/LOG2");
        snprintf(msgmqtt2, sizeof(msgmqtt2), "Moving input: %d", i);
        logging(LOG_INFO,msgmqtt2);
        strcpy(Input_Name[i],   Input_Name[i + 1]);
        strcpy(MQTT_Input[i],  MQTT_Input[i + 1]);
        SwitchPin[i]             = SwitchPin[i + 1];
        SwitchPressedExternal[i] = SwitchPressedExternal[i + 1];
        SwitchPressed[i]         = SwitchPressed[i + 1];
        SwitchDimDirection[i]    = SwitchDimDirection[i + 1];
        SwitchTimer[i]           = SwitchTimer[i + 1];
        InOutMatrixCouter[i]     = InOutMatrixCouter[i + 1];
        memcpy(InOutMatrix[i], InOutMatrix[i + 1], 5); // reconsider 5, must be in line with the maximum in the declaration
    }

    Input_Name[SwitchCounter][0]         = '\0';
    MQTT_Input[SwitchCounter][0]         = '\0';
    SwitchPin[SwitchCounter]             = 0;
    SwitchPressedExternal[SwitchCounter] = 0;
    SwitchPressed[SwitchCounter]         = 0;
    SwitchDimDirection[SwitchCounter]    = 0;
    SwitchTimer[SwitchCounter]           = 0;
    InOutMatrixCouter[SwitchCounter]     = 0;
    memset(InOutMatrix[SwitchCounter], 0, 5);// reconsider 5, must be in line with the maximum in the declaration

    SwitchCounter--;
    DeleteInput = 0;
  }

  if ((unsigned long)(currentMillis - LastInputInterval) >= InputInterval) {
    LastInputInterval += InputInterval;   // keep in sync even if code runs late
    input();
  }

  if ((unsigned long)(currentMillis - LastOutputInterval) >= OutputInterval) {
    LastOutputInterval += OutputInterval;   // keep in sync even if code runs late
    output();
  }
  if ((unsigned long)(currentMillis - LastOutput10msInterval) >= 10) {
    LastOutput10msInterval += 10;   // keep in sync even if code runs late
    output10ms();
  }
  if ((unsigned long)(currentMillis - LastDMXInterval) >= DMXInterval) {
    LastDMXInterval += DMXInterval;   // keep in sync even if code runs late
    DMXflush();
  }

  if ((unsigned long)(currentMillis - LastCheckNetworkInterval) >= CheckNetworkInterval) {
    LastCheckNetworkInterval += CheckNetworkInterval;   // keep in sync even if code runs late
    SendSystem(IDLE, 1);
    SendSystem(EEPROMBYTES, EEPROMStorage);
    SendSystem(RAMBYTES, freeRam());
    SendSystem(UPTIME, millis()/1000);
  }
  SerialCommand = 0;
  DynamicVariablesUpdated = false;
  }
