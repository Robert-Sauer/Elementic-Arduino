#include <Elementic.h>

extern const uint16_t ELEMENTIC_DEVICEID = 2;
extern const int device_type = 2;
extern const long projectid = 0;

void genericsetupvariables() {
#if ELEMENTIC_USE_NETWORK
  DefineString("Mac Address", "020000000001", PriorityCounter++);
  DefineBool("DHCP", true, PriorityCounter++);
  DefineBool("EthernetEnabled", TEST_SOFT_ENABLED, PriorityCounter++);
  DefineIP("IP Address", 0, 0, 0, 0, PriorityCounter++);
  DefineIP("Gateway", 0, 0, 0, 0, PriorityCounter++);
  DefineIP("Subnet", 255, 255, 255, 0, PriorityCounter++);
  DefineString("Hostname", "composition-test", PriorityCounter++);
#endif
#if ELEMENTIC_USE_MQTT
  DefineBool("MQTTEnabled", TEST_SOFT_ENABLED, PriorityCounter++);
  DefineString("MQTTName", "composition-test", PriorityCounter++);
  DefineString("MQTTServer", "", PriorityCounter++);
  DefineString("MQTTUsername", "", PriorityCounter++);
  DefinePassword("MQTTPassword", "", PriorityCounter++);
#endif
  DefineString("Testvariable", "Retained", PriorityCounter++);
}

void inputsetupvariables() {
  SwitchCounter = 1;
  SwitchPin[1] = 0;
  strcpy(Input_Name[1], "InputA");
#if ELEMENTIC_USE_MQTT
  strcpy(MQTT_Input[1], "test/input");
#endif
  InOutMatrix[1][0] = 1;
  InOutMatrixCouter[1] = 1;
}

void outputsetupvariables() {
  OutputCounter = 1;
  OutputType[1] = 1;
  strcpy(Output_Name[1], "OutputA");
#if ELEMENTIC_USE_MQTT
  strcpy(MQTT_Output[1], "test/output");
#endif
}

void setup() { ElementicSetup(); }
void loop() { ElementicLoop(); }
