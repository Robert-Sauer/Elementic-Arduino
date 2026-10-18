#include <Elementic.h>

const int device_type = 0;
const long projectid = 0;

const char MQTTName[] = "";
const char MQTTServer[] = "";
const char MQTTUsername[] = "";
const char MQTTPassword[] = "";
const uint8_t factorymac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
const bool factoryDHCP = true;
const uint8_t factoryip[4] = {0, 0, 0, 0};
const uint8_t factorygateway[4] = {0, 0, 0, 0};
const uint8_t factorysubnet[4] = {0, 0, 0, 0};
const char factoryhostname[] = "elementic-smoke";
const char factorySSID[] = "";
const char factoryWIFIPassword[] = "";

void genericsetupvariables() {}
void inputsetupvariables() {}
void outputsetupvariables() {}

void elementicLoadFactoryDefaults() {
  elementicResetConfigurationToEmpty();
  setupNetwork();
  genericsetupvariables();
  MQTTSetupVariables();
  outputsetupvariables();
  inputsetupvariables();
  elementicApplySharedMac(false);
}
