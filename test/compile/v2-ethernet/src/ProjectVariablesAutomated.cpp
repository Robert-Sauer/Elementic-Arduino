#include <Elementic.h>

const int device_type = 2;
const long projectid = 0;

const char GenericMacAddress[] = "02:00:00:00:00:01";
const bool GenericDHCP = true;
const uint8_t GenericIPAddress[4] = { 0, 0, 0, 0 };
const uint8_t GenericGateway[4] = { 0, 0, 0, 0 };
const uint8_t GenericSubnet[4] = { 255, 255, 255, 0 };
const char GenericHostname[] = "elementic-test";
const bool GenericMQTTEnabled = true;
const char GenericMQTTName[] = "elementic-test";
const char GenericMQTTServer[] = "";
const char GenericMQTTUsername[] = "";
const char GenericMQTTPassword[] = "";

void genericsetupvariables() {
  DefineString("Mac Address", GenericMacAddress, PriorityCounter++);
  DefineBool("DHCP", GenericDHCP, PriorityCounter++);
  DefineIP("IP Address", GenericIPAddress[0], GenericIPAddress[1], GenericIPAddress[2], GenericIPAddress[3], PriorityCounter++);
  DefineIP("Gateway", GenericGateway[0], GenericGateway[1], GenericGateway[2], GenericGateway[3], PriorityCounter++);
  DefineIP("Subnet", GenericSubnet[0], GenericSubnet[1], GenericSubnet[2], GenericSubnet[3], PriorityCounter++);
  DefineString("Hostname", GenericHostname, PriorityCounter++);
  DefineBool("MQTTEnabled", GenericMQTTEnabled, PriorityCounter++);
  DefineString("MQTTName", GenericMQTTName, PriorityCounter++);
  DefineString("MQTTServer", GenericMQTTServer, PriorityCounter++);
  DefineString("MQTTUsername", GenericMQTTUsername, PriorityCounter++);
  DefinePassword("MQTTPassword", GenericMQTTPassword, PriorityCounter++);
}

void inputsetupvariables() {
  SwitchCounter = 0;
}

void outputsetupvariables() {
  OutputCounter = 0;
}
