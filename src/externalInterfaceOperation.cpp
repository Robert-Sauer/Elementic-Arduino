/*
 * externalInterfaceOperation.cpp
 *
 * Fast-boot network handling for Elementic.
 *
 * Goals:
 * - Device must boot and become operational without network.
 * - Wi-Fi is always non-blocking.
 * - W5500/W5200 may use linkStatus() to skip pointless Ethernet starts.
 * - W5100 cannot reliably report link state; to protect boot time, DHCP on W5100
 *   is deferred out of setup().
 */

#include <Arduino.h>
#include "Elementic.h"
#include "deviceIdentification.h"

bool WIFIConnected     = false;
bool EthernetConnected = false;
bool networkConnected  = false;

static uint32_t elementic_lastNetStatusMs = 0;

#if ELEMENTIC_HAS_WIFI
  #if (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_ESP8266)
    #include <ESP8266WiFi.h>
    extern "C" {
      #include <user_interface.h>
    }
  #elif (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_ESP32)
    #include <WiFi.h>
    #include <esp_wifi.h>
  #elif (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_PICOW)
    #include <WiFi.h>
  #elif (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_OPTA)
    #include <WiFi.h>
  #else
    #error "Bad WiFi driver"
  #endif

  static bool g_wifiBeginStarted = false;
  static uint32_t g_lastWiFiBeginMs = 0;
  static uint32_t g_lastWiFiRetryMs = 0;
#endif

#if ELEMENTIC_HAS_ETHERNET
  #include <Ethernet.h>
  #include <SPI.h>
  static bool g_ethStarted = false;
  static uint32_t g_lastEthBeginMs = 0;
  static uint32_t g_lastEthRetryMs = 0;
  static bool g_ethDhcpDeferred = false;
  static bool g_ethHardwarePrepared = false;
#endif

static inline uint16_t elementic_getDeviceId() {
  return ELEMENTIC_DEVICEID;
}

static void elementic_mac_to_string(const uint8_t mac[6], char out[13]) {
  snprintf(out, 13, "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

#if ELEMENTIC_HAS_WIFI && (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_PICOW)
static bool elementic_use_cyw43_hardware_mac(uint8_t mac[6]) {
  if (WiFi.macAddress(mac) == 0) {
    elementicMacStatus = ELEMENTIC_MAC_ADAPTER_UNSUPPORTED;
    logging(LOG_WARNING, "WiFi hardware MAC unavailable");
    return false;
  }

  char hardwareMac[13];
  elementic_mac_to_string(mac, hardwareMac);
  const int macIndex = findVariableIndexByName("Mac Address");
  if (macIndex < 0 || variables[macIndex].type != DYNAMIC_STRING) {
    elementicMacStatus = ELEMENTIC_MAC_PROVISIONING_REQUIRED;
    logging(LOG_CAUTION, "MAC variable unavailable");
    return false;
  }

  const bool configuredMacDiffers =
      strcmp(variables[macIndex].value.strVal, hardwareMac) != 0;
  if (!SetStringValue("Mac Address", hardwareMac) ||
      !elementicApplySharedMac(false)) {
    logging(LOG_WARNING, "WiFi hardware MAC rejected");
    return false;
  }

  if (configuredMacDiffers) {
    logging(LOG_CAUTION, "WiFi using hardware MAC");
  }
  return true;
}
#endif

#if ELEMENTIC_HAS_WIFI
static inline bool elementic_wifi_is_connected() {
  return (WiFi.status() == WL_CONNECTED);
}
#else
static inline bool elementic_wifi_is_connected() {
  return false;
}
#endif

#if ELEMENTIC_HAS_ETHERNET
static void elementic_prepare_ethernet_hardware() {
  if (g_ethHardwarePrepared) return;

  #if defined(ARDUINO_WIZNET_5100S_EVB_PICO) || defined(ARDUINO_WIZNET_5100S_EVB_PICO2) || defined(ARDUINO_WIZNET_5500_EVB_PICO) || defined(ARDUINO_WIZNET_5500_EVB_PICO2)
    const uint8_t wiznetMiso = 16;
    const uint8_t wiznetCs   = 17;
    const uint8_t wiznetSclk = 18;
    const uint8_t wiznetMosi = 19;
    const uint8_t wiznetRst  = 20;

    SPI.setRX(wiznetMiso);
    SPI.setCS(wiznetCs);
    SPI.setSCK(wiznetSclk);
    SPI.setTX(wiznetMosi);

    pinMode(wiznetRst, OUTPUT);
    digitalWrite(wiznetRst, LOW);
    delayMicroseconds(500);
    digitalWrite(wiznetRst, HIGH);
    delay(200);

    Ethernet.init(wiznetCs);
  #endif

  g_ethHardwarePrepared = true;
}

static inline bool elementic_eth_has_valid_ip() {
  if (!g_ethStarted) return false;

  IPAddress ip = Ethernet.localIP();
  if (ip[0] == 0 || ip[0] == 255) return false;

  return true;
}

static inline bool elementic_read_ip(const String& varName, IPAddress& out) {
  byte* p = GetIPValue(varName);
  if (!p) return false;
  out = IPAddress(p[0], p[1], p[2], p[3]);
  return true;
}

static inline EthernetHardwareStatus elementic_eth_hw() {
  elementic_prepare_ethernet_hardware();
  return Ethernet.hardwareStatus();
}

static inline bool elementic_eth_is_w5500() {
  return (elementic_eth_hw() == EthernetW5500);
}

static inline bool elementic_eth_is_w5200() {
  return (elementic_eth_hw() == EthernetW5200);
}

static inline bool elementic_eth_is_w5100() {
  return (elementic_eth_hw() == EthernetW5100);
}

static inline bool elementic_eth_has_link_api() {
  return (elementic_eth_is_w5200() || elementic_eth_is_w5500());
}

static inline bool elementic_eth_link_is_on() {
  if (!elementic_eth_has_link_api()) return true;
  return (Ethernet.linkStatus() == LinkON);
}

static inline bool elementic_eth_should_try_in_setup(bool dhcpEnabled) {
  if (elementic_eth_has_link_api()) {
    return elementic_eth_link_is_on();
  }

  // W5100: no reliable link detect. DHCP can block badly when cable/network is absent.
  // To protect boot speed, defer DHCP out of setup().
  if (dhcpEnabled && elementic_eth_is_w5100()) {
    return false;
  }

  return true;
}
#else
static inline bool elementic_eth_has_valid_ip() {
  return false;
}
#endif

static void elementic_refreshNetworkFlags() {
  WIFIConnected = elementic_wifi_is_connected();

  #if ELEMENTIC_HAS_ETHERNET
    EthernetConnected = elementic_eth_has_valid_ip();
    #if ELEMENTIC_HAS_ETHERNET
      if (EthernetConnected && elementic_eth_has_link_api() && !elementic_eth_link_is_on()) {
        EthernetConnected = false;
      }
    #endif
  #else
    EthernetConnected = false;
  #endif

  networkConnected = (WIFIConnected || EthernetConnected);
  elementic_lastNetStatusMs = millis();
}

static void elementic_update_network_variable_status() {
  #if ELEMENTIC_HAS_NETWORK
    const bool dhcpEnabled = GetBoolValue("DHCP");

    ModifyVariableStatus(VARIABLE_DYNAMIC, 0, "IP Address", STATUS_GRAYED,   dhcpEnabled);
    ModifyVariableStatus(VARIABLE_DYNAMIC, 0, "Subnet",     STATUS_GRAYED,   dhcpEnabled);
    ModifyVariableStatus(VARIABLE_DYNAMIC, 0, "Gateway",    STATUS_GRAYED,   dhcpEnabled);

    ModifyVariableStatus(VARIABLE_DYNAMIC, 0, "IP Address", STATUS_READONLY, dhcpEnabled);
    ModifyVariableStatus(VARIABLE_DYNAMIC, 0, "Subnet",     STATUS_READONLY, dhcpEnabled);
    ModifyVariableStatus(VARIABLE_DYNAMIC, 0, "Gateway",    STATUS_READONLY, dhcpEnabled);
  #endif
}

#if ELEMENTIC_HAS_WIFI
static void connectToWiFi() {
  if (EthernetConnected) return;
  const String ssidStr = GetStringValue("SSID");
  const String passStr = GetPasswordValue("WIFI Password");

  if (ssidStr.length() == 0) {
    logging(LOG_CAUTION, "WiFi no SSID");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    g_wifiBeginStarted = true;
    return;
  }

  WiFi.mode(WIFI_OFF);
  delay(50);
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);

  uint8_t mac[6];
  #if (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_PICOW)
    // Arduino-Pico exposes the CYW43 factory MAC but no supported setter.
    // Keep the runtime variable aligned with the address actually used.
    if (!elementic_use_cyw43_hardware_mac(mac)) return;
  #else
    if (!elementicApplySharedMac(false) || !elementicGetEffectiveMac(mac)) {
      logging(LOG_WARNING, "WiFi MAC provisioning required");
      return;
    }
  #endif
  #if (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_ESP8266)
    if (!wifi_set_macaddr(STATION_IF, mac)) {
      elementicMacStatus = ELEMENTIC_MAC_ADAPTER_UNSUPPORTED;
      logging(LOG_WARNING, "WiFi MAC rejected");
      return;
    }
  #elif (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_ESP32)
    if (esp_wifi_set_mac(WIFI_IF_STA, mac) != ESP_OK) {
      elementicMacStatus = ELEMENTIC_MAC_ADAPTER_UNSUPPORTED;
      logging(LOG_WARNING, "WiFi MAC rejected");
      return;
    }
  #elif (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_PICOW)
    // The CYW43 driver already uses its factory MAC.
  #else
    elementicMacStatus = ELEMENTIC_MAC_ADAPTER_UNSUPPORTED;
    logging(LOG_WARNING, "WiFi MAC override unsupported");
    return;
  #endif

  #if defined(ESP8266) || defined(ESP32)
    WiFi.setAutoReconnect(true);
  #endif

  #if defined(ESP8266)
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
  #endif

  const String mqttName = GetStringValue("MQTTName");
  if (mqttName.length() > 0) {
    #if defined(ESP8266)
      WiFi.hostname(mqttName.c_str());
    #elif defined(ESP32) || (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_PICOW)
      WiFi.setHostname(mqttName.c_str());
    #endif
  }

  WiFi.begin(ssidStr.c_str(), passStr.c_str());
  g_wifiBeginStarted = true;
  g_lastWiFiBeginMs = millis();
  g_lastWiFiRetryMs = millis();
  logging(LOG_INFO, "WiFi begin");
}
#endif

#if ELEMENTIC_HAS_ETHERNET
static void connectToEthernet() {
  elementic_prepare_ethernet_hardware();

  uint8_t mac[6];
  if (!elementicApplySharedMac(false) || !elementicGetEffectiveMac(mac)) {
    logging(LOG_WARNING, "ETH MAC provisioning required");
    return;
  }

  #if ELEMENTIC_HAS_WIFI
    if (WiFi.status() == WL_CONNECTED) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      WIFIConnected = false;
    }
  #endif

  const bool dhcpEnabled = GetBoolValue("DHCP");

  if (elementic_eth_has_link_api() && !elementic_eth_link_is_on()) {
    logging(LOG_CAUTION, "ETH link off");
    g_ethStarted = false;
    return;
  }

  logging(LOG_INFO, dhcpEnabled ? "ETH start DHCP" : "ETH start static");
  

  if (dhcpEnabled) {
    // Keep DHCP fast-fail, but do not touch Wiznet retransmission registers
    // before Ethernet.begin() initializes the chip and SS handling.
    int result = Ethernet.begin(mac, 1000, 250);

    if (result == 0) {
      logging(LOG_WARNING, "ETH dhcp fail");
      g_ethStarted = false;
      g_lastEthBeginMs = millis();
      return;      
    }
    if (result != 0){
      logging(LOG_INFO, "ETH DHCP successful");

    }
  } else {
    IPAddress ip, subnet, gw;
    if (!elementic_read_ip("IP Address", ip))  { logging(LOG_WARNING, "ETH miss IP");  g_ethStarted = false; return; }
    if (!elementic_read_ip("Subnet", subnet))  { logging(LOG_WARNING, "ETH miss mask"); g_ethStarted = false; return; }
    if (!elementic_read_ip("Gateway", gw))     { logging(LOG_WARNING, "ETH miss gw");   g_ethStarted = false; return; }

    IPAddress dns = gw;
    Ethernet.begin(mac, ip, dns, gw, subnet);
  }

  delay(100);

  g_ethStarted = true;
  g_lastEthBeginMs = millis();
  g_lastEthRetryMs = millis();
  g_ethDhcpDeferred = false;
  elementic_refreshNetworkFlags();

  if (EthernetConnected) logging(LOG_INFO, "ETH ok");
  else                   logging(LOG_CAUTION, "ETH no ip");
}
#endif

void setupNetwork() {
  #if ELEMENTIC_PROJECT_VARIABLES_VERSION < 2
    #if ELEMENTIC_HAS_NETWORK
      char macAddress[13];
      elementic_mac_to_string(factorymac, macAddress);
      DefineString("Mac Address", macAddress, PriorityCounter++);
      DefineBool("DHCP", factoryDHCP, PriorityCounter++);
      DefineIP("IP Address", factoryip[0], factoryip[1], factoryip[2], factoryip[3], PriorityCounter++);
      DefineIP("Gateway",   factorygateway[0], factorygateway[1], factorygateway[2], factorygateway[3], PriorityCounter++);
      DefineIP("Subnet",    factorysubnet[0], factorysubnet[1], factorysubnet[2], factorysubnet[3], PriorityCounter++);
      DefineString("Hostname", factoryhostname, PriorityCounter++);
    #endif

    #if ELEMENTIC_HAS_WIFI
      DefineString("SSID", factorySSID, PriorityCounter++);
      DefinePassword("WIFI Password", factoryWIFIPassword, PriorityCounter++);
    #endif
  #endif

  elementic_refreshNetworkFlags();
  elementic_update_network_variable_status();
}

void connectNetwork() {
  const uint16_t deviceId = elementic_getDeviceId();

  if (deviceId == 0) {
    logging(LOG_CAUTION, "Net bad dev id");
    elementic_refreshNetworkFlags();
    return;
  }

  #if ELEMENTIC_USE_ETHERNET
    {
      const bool dhcpEnabled = GetBoolValue("DHCP");

      if (elementic_eth_should_try_in_setup(dhcpEnabled)) {
        connectToEthernet();
      } else {
        g_ethDhcpDeferred = true;
        g_lastEthRetryMs = millis();
        logging(LOG_INFO, "ETH dhcp defer");
      }
      elementic_refreshNetworkFlags();
    }
  #endif

  #if ELEMENTIC_USE_WIFI
    if (!EthernetConnected) {
      connectToWiFi();
      elementic_refreshNetworkFlags();
    }
  #endif

  elementic_update_network_variable_status();
}

void networkCheckLoop() {
  static uint32_t lastNetCheckMs = 0;

  #if ELEMENTIC_USE_ETHERNET
    static bool dhcpInitialized = false;
    static bool lastDhcpEnabled = false;
  #endif

  #if ELEMENTIC_USE_NETWORK
    if (DynamicVariablesUpdated) {
      DynamicVariablesUpdated = false;
      elementic_update_network_variable_status();

      #if ELEMENTIC_USE_ETHERNET
        const bool dhcpEnabled = GetBoolValue("DHCP");
        if (!dhcpInitialized) {
          dhcpInitialized = true;
          lastDhcpEnabled = dhcpEnabled;
        } else if (dhcpEnabled != lastDhcpEnabled) {
          lastDhcpEnabled = dhcpEnabled;
          g_ethStarted = false;
          g_ethDhcpDeferred = false;
          logging(LOG_INFO, "DHCP changed");
        }
      #endif
    }
  #endif

  const uint32_t now = millis();
  if ((uint32_t)(now - lastNetCheckMs) < 5000UL) return;
  lastNetCheckMs = now;

  elementic_refreshNetworkFlags();

  const uint16_t deviceId = elementic_getDeviceId();
  if (deviceId == 0) return;

  #if ELEMENTIC_USE_ETHERNET
    const bool dhcpEnabled = GetBoolValue("DHCP");

    if (!dhcpInitialized) {
      dhcpInitialized = true;
      lastDhcpEnabled = dhcpEnabled;
    }
  #endif

  #if ELEMENTIC_USE_WIFI
    if (!WIFIConnected) {
      if (!g_wifiBeginStarted || (uint32_t)(now - g_lastWiFiRetryMs) >= 10000UL) {
        g_lastWiFiRetryMs = now;
        connectToWiFi();
        elementic_refreshNetworkFlags();
        if (WIFIConnected) return;
      }
    }
  #endif

  #if ELEMENTIC_USE_ETHERNET
    if (!EthernetConnected) {
      // W5500/W5200: safe to check cable state first.
      if (elementic_eth_has_link_api() && !elementic_eth_link_is_on()) {
        logging(LOG_CAUTION, "ETH link off");
        g_ethStarted = false;
        return;
      }

      // W5100 + DHCP: do not hammer begin() every 5s. It can block when no network.
      if (dhcpEnabled && elementic_eth_is_w5100()) {
        const uint32_t deferMs = g_ethDhcpDeferred ? 3000UL : 6000UL;
        if ((uint32_t)(now - g_lastEthRetryMs) < deferMs) {
          return;
        }
        g_lastEthRetryMs = now;
        logging(LOG_INFO, "ETH dhcp retry");
        connectToEthernet();
        elementic_refreshNetworkFlags();
        return;
      }

      if ((uint32_t)(now - g_lastEthRetryMs) < 10000UL) {
        return;
      }
      g_lastEthRetryMs = now;
      logging(LOG_CAUTION, "ETH retry");
      connectToEthernet();
      elementic_refreshNetworkFlags();
    }
  #endif
}
