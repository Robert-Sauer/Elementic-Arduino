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
  #elif (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_ESP32)
    #include <WiFi.h>
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
  static bool g_ethStarted = false;
  static uint32_t g_lastEthBeginMs = 0;
  static uint32_t g_lastEthRetryMs = 0;
  static bool g_ethDhcpDeferred = false;
#endif

static inline uint16_t elementic_getDeviceId() {
  return ELEMENTIC_DEVICEID;
}

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
static inline bool elementic_eth_has_valid_ip() {
  if (!g_ethStarted) return false;

  IPAddress ip = Ethernet.localIP();
  if (ip[0] == 0 || ip[0] == 255) return false;

  return true;
}

static inline void elementic_derive_mac_from_deviceid(uint16_t deviceId, uint8_t outMac[6]) {
  outMac[0] = 0x02;
  outMac[1] = 0xEC;
  outMac[2] = 0x00;
  outMac[3] = (uint8_t)((projectid >> 8) & 0xFF);
  outMac[4] = (uint8_t)(projectid & 0xFF);
  outMac[5] = 0x01;
}

static inline bool elementic_read_ip(const String& varName, IPAddress& out) {
  byte* p = GetIPValue(varName);
  if (!p) return false;
  out = IPAddress(p[0], p[1], p[2], p[3]);
  return true;
}

static inline EthernetHardwareStatus elementic_eth_hw() {
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
  WiFi.setAutoReconnect(true);

  #if defined(ESP8266)
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
  #endif

  if (MQTTName && MQTTName[0]) {
    #if defined(ESP8266)
      WiFi.hostname(MQTTName);
    #elif defined(ESP32)
      WiFi.setHostname(MQTTName);
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
static void connectToEthernet(uint16_t deviceId) {
  uint8_t mac[6];
  elementic_derive_mac_from_deviceid(deviceId, mac);

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
      logging(LOG_WARNING, "ETH dhcp successfull");

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
  #if ELEMENTIC_HAS_NETWORK
    DefineString("Mac Address", "809B209BFFB4", PriorityCounter++);
    DefineBool("DHCP", false, PriorityCounter++);
    DefineIP("IP Address", factoryip[0], factoryip[1], factoryip[2], factoryip[3], PriorityCounter++);
    DefineIP("Gateway",   factorygateway[0], factorygateway[1], factorygateway[2], factorygateway[3], PriorityCounter++);
    DefineIP("Subnet",    factorysubnet[0], factorysubnet[1], factorysubnet[2], factorysubnet[3], PriorityCounter++);
    DefineString("Hostname", factoryhostname, PriorityCounter++);
  #endif

  #if ELEMENTIC_HAS_WIFI
    DefineString("SSID", factorySSID, PriorityCounter++);
    DefinePassword("WIFI Password", factoryWIFIPassword, PriorityCounter++);
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

  #if ELEMENTIC_HAS_WIFI
    connectToWiFi();
    elementic_refreshNetworkFlags();
  #endif

  #if ELEMENTIC_HAS_ETHERNET
    if (!networkConnected) {
      const bool dhcpEnabled = GetBoolValue("DHCP");

      if (elementic_eth_should_try_in_setup(dhcpEnabled)) {
        connectToEthernet(deviceId);
      } else {
        g_ethDhcpDeferred = true;
        g_lastEthRetryMs = millis();
        logging(LOG_INFO, "ETH dhcp defer");
      }
      elementic_refreshNetworkFlags();
    }
  #endif

  elementic_update_network_variable_status();
}

void networkCheckLoop() {
  static uint32_t lastNetCheckMs = 0;
  static bool dhcpInitialized = false;
  static bool lastDhcpEnabled = false;

  #if ELEMENTIC_HAS_NETWORK
    if (DynamicVariablesUpdated) {
      DynamicVariablesUpdated = false;

      const bool dhcpEnabled = GetBoolValue("DHCP");
      elementic_update_network_variable_status();

      #if ELEMENTIC_HAS_ETHERNET
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

  const bool dhcpEnabled = GetBoolValue("DHCP");

  #if ELEMENTIC_HAS_ETHERNET
    if (!dhcpInitialized) {
      dhcpInitialized = true;
      lastDhcpEnabled = dhcpEnabled;
    }
  #endif

  #if ELEMENTIC_HAS_WIFI
    if (!WIFIConnected) {
      if (!g_wifiBeginStarted || (uint32_t)(now - g_lastWiFiRetryMs) >= 10000UL) {
        g_lastWiFiRetryMs = now;
        connectToWiFi();
        elementic_refreshNetworkFlags();
        if (WIFIConnected) return;
      }
    }
  #endif

  #if ELEMENTIC_HAS_ETHERNET
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
        connectToEthernet(deviceId);
        elementic_refreshNetworkFlags();
        return;
      }

      if ((uint32_t)(now - g_lastEthRetryMs) < 10000UL) {
        return;
      }
      g_lastEthRetryMs = now;
      logging(LOG_CAUTION, "ETH retry");
      connectToEthernet(deviceId);
      elementic_refreshNetworkFlags();
    }
  #endif
}
