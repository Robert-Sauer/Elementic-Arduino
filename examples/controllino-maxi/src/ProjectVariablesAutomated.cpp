// Example configuration based on an Elementic Controllino Maxi project.
// Fill in MQTT/network values before using this on a real installation.

// Visit http://elementic.app to generate this file automatically based on your project setup, or edit it manually if you prefer.

// It contains the definitions of all global variables used in the Elementic library, as well as the setup functions for your specific project. 
// Make sure to fill in the MQTT and network values before using this on a real setup.


const int device_type = 2; // Controllino Maxi

#include <Elementic.h>

const long projectid = 3;

const char MQTTName[] = "";
const char MQTTServer[] = "";
const char MQTTUsername[] = "";
const char MQTTPassword[] = "";
const uint8_t factorymac[6] = { 0x49, 0xEB, 0xCD, 0x6D, 0xF3, 0x9F };
const uint8_t factoryip[4] = { 192, 168, 1, 202 };
const uint8_t factorygateway[4] = { 192, 168, 1, 1 };
const uint8_t factorysubnet[4] = { 255, 255, 255, 0 };
const char factoryhostname[] = "controllino-example";
const char factorySSID[] = "";
const char factoryWIFIPassword[] = "";

void genericsetupvariables() {
}

void inputsetupvariables() {
  SwitchCounter = 10;

  SwitchPin[1] = 60;
  strncpy(Input_Name[1], "Switch 1", sizeof(Input_Name[1]) - 1);
  strncpy(MQTT_Input[1], "Elem/Switch01", sizeof(MQTT_Input[1]) - 1);
  InOutMatrix[1][0] = 1;
  InOutMatrixCouter[1] = 1;

  SwitchPin[2] = 56;
  strncpy(Input_Name[2], "Switch 2", sizeof(Input_Name[2]) - 1);
  strncpy(MQTT_Input[2], "Elem/Switch02", sizeof(MQTT_Input[2]) - 1);
  InOutMatrix[2][0] = 2;
  InOutMatrixCouter[2] = 1;

  SwitchPin[3] = 57;
  strncpy(Input_Name[3], "Switch 3", sizeof(Input_Name[3]) - 1);
  strncpy(MQTT_Input[3], "Elem/Switch03", sizeof(MQTT_Input[3]) - 1);
  InOutMatrix[3][0] = 3;
  InOutMatrixCouter[3] = 1;

  SwitchPin[4] = 58;
  strncpy(Input_Name[4], "Switch 4", sizeof(Input_Name[4]) - 1);
  strncpy(MQTT_Input[4], "Elem/Switch04", sizeof(MQTT_Input[4]) - 1);
  InOutMatrix[4][0] = 4;
  InOutMatrixCouter[4] = 1;

  SwitchPin[5] = 59;
  strncpy(Input_Name[5], "Switch 5", sizeof(Input_Name[5]) - 1);
  strncpy(MQTT_Input[5], "Elem/Switch05", sizeof(MQTT_Input[5]) - 1);
  InOutMatrix[5][0] = 5;
  InOutMatrixCouter[5] = 1;

  SwitchPin[6] = 62;
  strncpy(Input_Name[6], "Switch 6", sizeof(Input_Name[6]) - 1);
  strncpy(MQTT_Input[6], "Elem/Switch06", sizeof(MQTT_Input[6]) - 1);
  InOutMatrix[6][0] = 6;
  InOutMatrixCouter[6] = 1;

  SwitchPin[7] = 55;
  strncpy(Input_Name[7], "Switch 7", sizeof(Input_Name[7]) - 1);
  strncpy(MQTT_Input[7], "Elem/Switch07", sizeof(MQTT_Input[7]) - 1);
  InOutMatrix[7][0] = 7;
  InOutMatrixCouter[7] = 1;

  SwitchPin[8] = 63;
  strncpy(Input_Name[8], "Switch 8", sizeof(Input_Name[8]) - 1);
  strncpy(MQTT_Input[8], "Elem/Switch08", sizeof(MQTT_Input[8]) - 1);
  InOutMatrixCouter[8] = 0;

  SwitchPin[9] = 54;
  strncpy(Input_Name[9], "Switch 9", sizeof(Input_Name[9]) - 1);
  strncpy(MQTT_Input[9], "Elem/Switch09", sizeof(MQTT_Input[9]) - 1);
  InOutMatrix[9][0] = 8;
  InOutMatrixCouter[9] = 1;

  SwitchPin[10] = 61;
  strncpy(Input_Name[10], "Switch 10", sizeof(Input_Name[10]) - 1);
  strncpy(MQTT_Input[10], "Elem/Switch10", sizeof(MQTT_Input[10]) - 1);
  InOutMatrix[10][0] = 8;
  InOutMatrixCouter[10] = 1;
}

void outputsetupvariables() {
  OutputCounter = 9;

  strncpy(Output_Name[1], "Room 1", sizeof(Output_Name[1]) - 1);
  OutputRelay[1] = 27;
  OutputType[1] = 1;
  OutputMinValue[1] = 8;
  strncpy(MQTT_Output[1], "Elem/Room01", sizeof(MQTT_Output[1]) - 1);

  strncpy(Output_Name[2], "Room 2", sizeof(Output_Name[2]) - 1);
  OutputRelay[2] = 23;
  OutputType[2] = 1;
  strncpy(MQTT_Output[2], "Elem/Room02", sizeof(MQTT_Output[2]) - 1);

  strncpy(Output_Name[3], "Room 3", sizeof(Output_Name[3]) - 1);
  OutputRelay[3] = 24;
  OutputType[3] = 1;
  strncpy(MQTT_Output[3], "Elem/Room03", sizeof(MQTT_Output[3]) - 1);

  strncpy(Output_Name[4], "Room 4", sizeof(Output_Name[4]) - 1);
  OutputRelay[4] = 22;
  OutputType[4] = 1;
  strncpy(MQTT_Output[4], "Elem/Room04", sizeof(MQTT_Output[4]) - 1);

  strncpy(Output_Name[5], "Room 5", sizeof(Output_Name[5]) - 1);
  OutputRelay[5] = 28;
  OutputType[5] = 2;
  OutputPin[5] = 2;
  strncpy(MQTT_Output[5], "Elem/Room05", sizeof(MQTT_Output[5]) - 1);

  strncpy(Output_Name[6], "Room 6", sizeof(Output_Name[6]) - 1);
  OutputRelay[6] = 25;
  OutputType[6] = 1;
  strncpy(MQTT_Output[6], "Elem/Room06", sizeof(MQTT_Output[6]) - 1);

  strncpy(Output_Name[7], "Room 7", sizeof(Output_Name[7]) - 1);
  OutputRelay[7] = 28;
  OutputType[7] = 2;
  OutputMinValue[7] = 8;
  OutputMaxValue[7] = 100;
  OutputDefaultValue[7] = 100;
  OutputPin[7] = 3;
  strncpy(MQTT_Output[7], "Elem/Room07", sizeof(MQTT_Output[7]) - 1);

  strncpy(Output_Name[8], "Room 8", sizeof(Output_Name[8]) - 1);
  OutputRelay[8] = 29;
  OutputType[8] = 4;
  OutputMinValue[8] = 7;
  OutputMaxValue[8] = 255;
  OutputDefaultValue[8] = 100;
  OutputChannel[8] = 1;
  strncpy(MQTT_Output[8], "Elem/Room08", sizeof(MQTT_Output[8]) - 1);

  strncpy(Output_Name[9], "Room 9", sizeof(Output_Name[9]) - 1);
  OutputRelay[9] = 29;
  OutputType[9] = 4;
  OutputMinValue[9] = 7;
  OutputMaxValue[9] = 255;
  OutputDefaultValue[9] = 100;
  OutputChannel[9] = 2;
  strncpy(MQTT_Output[9], "Elem/Room09", sizeof(MQTT_Output[9]) - 1);
}
