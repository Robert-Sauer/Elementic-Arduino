#include <Elementic.h>

const int device_type = 12;
const long projectid = 0;

void genericsetupvariables() {
  DefineString("Mac Address", "020000000001", PriorityCounter++);
  DefineBool("DHCP", true, PriorityCounter++);
  DefineIP("IP Address", 0, 0, 0, 0, PriorityCounter++);
  DefineIP("Gateway", 0, 0, 0, 0, PriorityCounter++);
  DefineIP("Subnet", 255, 255, 255, 0, PriorityCounter++);
  DefineString("Hostname", "elementic-rp-test", PriorityCounter++);
  #if ELEMENTIC_FEATURE_WIFI
    DefineString("SSID", "", PriorityCounter++);
    DefinePassword("WIFI Password", "", PriorityCounter++);
  #endif
  DefineBool("MQTTEnabled", false, PriorityCounter++);
  DefineString("MQTTName", "elementic-rp-test", PriorityCounter++);
  DefineString("MQTTServer", "", PriorityCounter++);
  DefineString("MQTTUsername", "", PriorityCounter++);
  DefinePassword("MQTTPassword", "", PriorityCounter++);
}

void inputsetupvariables() {
  SwitchCounter = 0;
}

void outputsetupvariables() {
  OutputCounter = 0;
}
