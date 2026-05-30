// Example configuration based on an Elementic ESP32-C6 project.
// Fill in MQTT/WiFi values before using this on a real installation.


// Visit http://elementic.app to generate this file automatically based on your project setup, or edit it manually if you prefer.

// It contains the definitions of all global variables used in the Elementic library, as well as the setup functions for your specific project. 
// Make sure to fill in the MQTT and network values before using this on a real setup.

extern const long projectid = 0;
const int device_type = 8;

#include <Elementic.h>

const char MQTTName[] = "";
const char MQTTServer[] = "";
const char MQTTUsername[] = "";
const char MQTTPassword[] = "";
const uint8_t factorymac[6] = { 0x80, 0x9B, 0x20, 0x9B, 0xFF, 0xB9 };
const bool factoryDHCP = true;
const uint8_t factoryip[4] = { 192, 168, 1, 254 };
const uint8_t factorygateway[4] = { 192, 168, 1, 1 };
const uint8_t factorysubnet[4] = { 255, 255, 255, 0 };
const char factoryhostname[] = "esp32-c6-example";
const char factorySSID[] = "";
const char factoryWIFIPassword[] = "";

void genericsetupvariables() {
}

void inputsetupvariables() {
  SwitchCounter = 4;

  SwitchPin[1] = 18;
  SwitchPin[2] = 19;
  SwitchPin[3] = 20;
  SwitchPin[4] = 21;

  strncpy(Input_Name[1], "Switch 1", sizeof(Input_Name[1]) - 1);
  strncpy(Input_Name[2], "Switch 2", sizeof(Input_Name[2]) - 1);
  strncpy(Input_Name[3], "Switch 3", sizeof(Input_Name[3]) - 1);
  strncpy(Input_Name[4], "Switch 4", sizeof(Input_Name[4]) - 1);

  strncpy(MQTT_Input[1], "Test/Switch01", sizeof(MQTT_Input[1]) - 1);
  strncpy(MQTT_Input[2], "Test/Switch02", sizeof(MQTT_Input[2]) - 1);
  strncpy(MQTT_Input[3], "Test/Switch03", sizeof(MQTT_Input[3]) - 1);
  strncpy(MQTT_Input[4], "Test/Switch04", sizeof(MQTT_Input[4]) - 1);

  InOutMatrix[1][0] = 1;
  InOutMatrixCouter[1] = 1;
  InOutMatrix[2][0] = 2;
  InOutMatrixCouter[2] = 1;
  InOutMatrix[3][0] = 3;
  InOutMatrixCouter[3] = 1;
  InOutMatrix[4][0] = 4;
  InOutMatrixCouter[4] = 1;
}

void outputsetupvariables() {
  OutputCounter = 4;

  strncpy(Output_Name[1], "LED Dimmable", sizeof(Output_Name[1]) - 1);
  OutputRelay[1] = 22;
  OutputType[1] = 2;
  OutputMinValue[1] = 3;
  OutputMaxValue[1] = 200;
  OutputDefaultValue[1] = 100;
  OutputPin[1] = 10;
  strncpy(MQTT_Output[1], "Test/LEDDimmable", sizeof(MQTT_Output[1]) - 1);

  strncpy(Output_Name[2], "Light on/off", sizeof(Output_Name[2]) - 1);
  OutputRelay[2] = 23;
  OutputType[2] = 1;
  OutputPin[2] = 11;
  strncpy(MQTT_Output[2], "Test/LightOnOff", sizeof(MQTT_Output[2]) - 1);

  strncpy(Output_Name[3], "Kantoor", sizeof(Output_Name[3]) - 1);
  OutputType[3] = 6;
  OutputMinValue[3] = 3;
  OutputMaxValue[3] = 100;
  OutputDefaultValue[3] = 20;
  strncpy(MQTT_Output[3], "Test/External", sizeof(MQTT_Output[3]) - 1);
}
