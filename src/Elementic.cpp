/*****************************************************************
 *  Elementic.cpp  –  enige bronbestand dat alle globale
 *  variabelen definitief **definieert** én de stub-functies bevat
 *****************************************************************/

#include <Arduino.h>
#include <EEPROM.h>
#include "Elementic.h"

// tell the CPP that netClient is created elsewhere
extern Client& netClient;
PubSubClient mqttClient(netClient);   // ONE definition

unsigned long previousMillis = 0;
const unsigned long interval = 2000; // 2 seconds in milliseconds


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
uint8_t  OutputPin[OutputChannels]          = {0};
uint8_t  OutputChannel[OutputChannels]      = {0};
uint8_t  OutputRelay[OutputChannels]        = {0};
uint8_t  OutputType[OutputChannels]         = {0};
uint8_t  Relay[10]                          = {0};
uint8_t  TotalRelays                        = 0;

uint8_t  DMXValue[255]                      = {0};
uint8_t  DMXTotal                           = 0;

uint8_t  OutputValueMemory[OutputChannels]  = {0};
uint8_t  OutputValueActual[OutputChannels]  = {0};
uint8_t  OutputMinValue[OutputChannels]     = {0};
uint8_t  OutputMaxValue[OutputChannels]     = {0};
uint8_t  OutputDefaultValue[OutputChannels] = {0};
uint8_t  OutputRedValue[OutputChannels]     = {0};
uint8_t  OutputGreenValue[OutputChannels]   = {0};
uint8_t  OutputBlueValue[OutputChannels]    = {0};
uint8_t  OutputRedMemory[OutputChannels]    = {0};
uint8_t  OutputGreenMemory[OutputChannels]  = {0};
uint8_t  OutputBlueMemory[OutputChannels]   = {0};

bool     MQTTUpdate[OutputChannels]         = {false};
bool     OutputRelayUpdate                  = false;

uint8_t  OutputCounter                      = 0;
int  SwitchCounter                      = 0;
uint8_t  SwitchPin[InputChannels]           = {0};

uint8_t  SwitchPressedExternal[InputChannels] = {0};
uint8_t  SwitchPressed[InputChannels]         = {0};
bool     SwitchDimDirection[InputChannels]    = {false};
int  SwitchTimer[InputChannels]           = {0};

uint8_t  InOutMatrixCouter[InputChannels]     = {0};
uint8_t  InOutMatrix[InputChannels][5]        = {{0}};

char     MQTT_Output[OutputChannels][28]      = {{0}};
char     MQTT_Input[InputChannels][23]        = {{0}};
char     MQTT_Name[OutputChannels][10]        = {{0}};

uint8_t  dimvalue0           = 0;
uint8_t  state               = 0;
uint8_t  DMXBuffer[DMX_MAX]  = {0};

uint8_t  incomingByte              = 0;
uint8_t  incomingdata[500]         = {0};
int      data_index                = 0;
uint8_t  databuffersize            = 0;
int      databufferposition        = 0;
int      SerialCommand             = 0;
int      EEPROMStorage             = 0;

int      DMXstartupTimer           = 0;
int      MQTTBurstProtection       = 0;
char msgmqtt[80];
char msgmqtt2[80];


/* ──────────────────────────────
 *  Dynamic-variable tabel
 * ────────────────────────────── */
 const int totalVariables = TOTAL_STRINGS + TOTAL_BYTES + TOTAL_INTS
                          + TOTAL_IPS     + TOTAL_PASSWORDS;

/* Helpers */
int findVariableIndexByPriority(int targetPriority);
String GetStringValue(const String& varName);
byte GetByteValue(const String& varName);
int GetIntValue(const String& varName);
byte* GetIPValue(const String& varName);
String GetPasswordValue(const String& varName);
int GetPriority(const String& varName);



DynamicVariable variables[totalVariables] = {};   // leeg init; zelf vullen in setup()


void ElementicSetup(){
  #if defined(ESP32) || defined(ESP8266)
    EEPROM.begin(2048);     // Only needed on ESP platforms
  #endif
  outputsetupvariables();
  inputsetupvariables();
  for (uint8_t i = 0; i < TotalRelays; i++) //TotalRelays
    {
        pinMode(Relay[i], OUTPUT);
    }
}


 void callback(char* topic, byte* payload, unsigned int length) {
  String payloadprocessed;
  for (uint8_t i = 0; i < length; i++) {
   payloadprocessed.concat((char)payload[i]);
  }

  //color_set
  //tempvalue_set
  //temptime_set
  //flash_set

  for (int i=1; i <= OutputCounter; i++){
    if (OutputType[i]==1||OutputType[i]==2||OutputType[i]==3||OutputType[i]==4||OutputType[i]==6){ //01-04-2020 - 6 toegevoegd   
      sprintf(msgmqtt, "%s%s", MQTT_Output[i], dimvalue_set); // Deze gebruiken als test, daarna uitzetten
      if(String(MQTT_Output[i]).equals(String(topic).substring(0,strlen(MQTT_Output[i])))){
        if(String(onoffset).equals(String(topic).substring(strlen(MQTT_Output[i]),strlen(topic)))){//onoff
          //Serial.print("Match onoff: ");
          //Serial.println(payloadprocessed.toInt());
          // Type toevoegen, defaultvalue bij dimbare lamp
          if (payloadprocessed.equals(String("ON"))){
            if(OutputType[i]==1||OutputType[i]==5)
              {
              OutputValueActual[i]=1;
              }
            else
              {
              OutputValueActual[i]=OutputDefaultValue[i];
              }
            }
          else{
            OutputValueActual[i]=0;
            }
          }
        if(String(dimvalue_set).equals(String(topic).substring(strlen(MQTT_Output[i]),strlen(topic)))){//dimvalue
          //Serial.print("Voor lamp: ");
          //Serial.println(i);
          //Serial.print("Match dimvalue: ");
          //Serial.print((char)lowByte(payloadprocessed.toInt()), DEC);
          //Serial.println("END");
          if ((int)payloadprocessed.toInt()>0&&(int)payloadprocessed.toInt()<OutputMinValue[i]){
            OutputValueActual[i] = OutputMinValue[i];
            }
          if ((int)payloadprocessed.toInt()>OutputMaxValue[i]){
            OutputValueActual[i] = OutputMaxValue[i];
            }
          if ((int)payloadprocessed.toInt()<=OutputMaxValue[i]&&(int)payloadprocessed.toInt()>=OutputMinValue[i]){
            OutputValueActual[i] = (int) payloadprocessed.toInt();
            }
          if ((int)payloadprocessed.toInt()==0){
            OutputValueActual[i] = 0;
            }
          MQTTUpdate[i] = true; // 29-03-2020 Toegevoegd na verplaatsen mqtt vanaf output
          }
        // Dimvalue status for 6
        if(String(dimvalue_status).equals(String(topic).substring(strlen(MQTT_Output[i]),strlen(topic)))){//dimvalue 01-04-2020 deze loop toegevoegd
          //Serial.print("Voor lamp: ");
          //Serial.println(i);
          //Serial.print("Match dimvalue: ");
          //Serial.print((char)lowByte(payloadprocessed.toInt()), DEC);
          //Serial.println("END");
          if(OutputType[i]==6){
            if ((int)payloadprocessed.toInt()>0&&(int)payloadprocessed.toInt()<OutputMinValue[i]){
              OutputValueActual[i] = OutputMinValue[i];
              }
            if ((int)payloadprocessed.toInt()>OutputMaxValue[i]){
              OutputValueActual[i] = OutputMaxValue[i];
              }
            if ((int)payloadprocessed.toInt()<=OutputMaxValue[i]&&(int)payloadprocessed.toInt()>=OutputMinValue[i]){
              OutputValueActual[i] = (int) payloadprocessed.toInt();
              }
            if ((int)payloadprocessed.toInt()==0){
              OutputValueActual[i] = 0;
              }
            OutputValueMemory[i] = OutputValueActual[i];
            //MQTTUpdate[i] = true; // 29-03-2020 Toegevoegd na verplaatsen mqtt vanaf output
            }
          }
        //color_set
        // if(String(color_set).equals(String(topic).substring(strlen(MQTT_Output[i]),strlen(topic)))){//dimvalue
        //   uint8_t firstIndex = payloadprocessed.indexOf(',');
        //   uint8_t lastIndex = payloadprocessed.lastIndexOf(',');
        //   uint8_t rgb_red = payloadprocessed.substring(0, firstIndex).toInt();
        //   if (rgb_red < 0 || rgb_red > 255) {
        //     return;
        //     } 
        //   else {
        //     OutputRedValue[i] = rgb_red;
        //     }
        //   uint8_t rgb_green = payloadprocessed.substring(firstIndex + 1, lastIndex).toInt();
        //   if (rgb_green < 0 || rgb_green > 255) {
        //     return;
        //     }
        //   else {
        //     OutputGreenValue[i] = rgb_green;
        //   }
        //   uint8_t rgb_blue = payloadprocessed.substring(lastIndex + 1).toInt();
        //   if (rgb_blue < 0 || rgb_blue > 255) {
        //     return;
        //     } 
        //   else {
        //     OutputBlueValue[i] = rgb_blue;
        //     }
        //   }

        //tempvalue_set
        //temptime_set
        //flash_set
        //Default value 09-02-2020
         if(String(defaultvalue_set).equals(String(topic).substring(strlen(MQTT_Output[i]),strlen(topic)))){//dimvalue
          if ((int)payloadprocessed.toInt()>0&&(int)payloadprocessed.toInt()<OutputMinValue[i]){
            OutputDefaultValue[i] = OutputMinValue[i];
            }
          if ((int)payloadprocessed.toInt()>OutputMaxValue[i]){
            OutputDefaultValue[i] = OutputMaxValue[i];
            }
          if ((int)payloadprocessed.toInt()<=OutputMaxValue[i]&&(int)payloadprocessed.toInt()>=OutputMinValue[i]){
            OutputDefaultValue[i] = (int) payloadprocessed.toInt(); //11-02-2020 Hier zat een fout!!
            // Waarom gingen de lampen niet continu naar de default value als hier actual stond?
            }
          if ((int)payloadprocessed.toInt()==0){
            //Do nothing
            }
          }
        }
      }
    }
  }

void ElementicLoop(){
  serialloop();
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    //procedure every interval
  }

  switch (SerialCommand) { // RAM/EEPROM Status, consider generic loop for Elementic
  case 1:
    sprintf(msgmqtt, "%s%s", GetStringValue("MQTTName").c_str(), "/LOG");
    mqttClient.publish(msgmqtt, "Store Data to EEPROM");
    SendAllData(true, false, true);
    break;
  case 2:
    sprintf(msgmqtt, "%s%s", GetStringValue("MQTTName").c_str(), "/LOG");
    mqttClient.publish(msgmqtt, "Restore Data from EEPROM");
    ReadAllDataFromEEPROM();
    break;
  case 3:
  case 4:
    break;
  }
  SerialCommand = 0;
  }