// This contains all code to operate Elementic
// - Transport layer
//   - Ethernet
//   - WIFI
//   - LORA (To Be Developped)
//   - Thread (To Be Developped)
// - Application layer
//  - MQTT
//  - Matter (To Be Developped)

#pragma once
#include <Arduino.h>
#include "Elementic.h"
#include "deviceIdentification.h"   // DEVICEID, ELEMENTIC_HAS_WIFI, ELEMENTIC_HAS_ETHERNET, driver enums

// ============================================================================
// 0) Unified transport booleans you requested
//    - WIFIConnected       : true when WiFi link is up (WL_CONNECTED)
//    - EthernetConnected   : true when Ethernet has a valid IP (and link if available)
//    - networkConnected    : true when either WIFIConnected OR EthernetConnected
// ============================================================================
bool WIFIConnected     = false;
bool EthernetConnected = false;
bool networkConnected  = false;

// Optional: timestamp of last status refresh (useful for debugging / telemetry)
static uint32_t elementic_lastNetStatusMs = 0;


// ============================================================================
// 1) Include the correct WiFi header (or none)
// ============================================================================
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
    #error "ELEMENTIC_HAS_WIFI is true, but no supported WiFi driver header is selected."
  #endif

#endif // ELEMENTIC_HAS_WIFI


// ============================================================================
// 2) Include the correct Ethernet header (or none)
// ============================================================================
#if ELEMENTIC_HAS_ETHERNET
  #include <Ethernet.h>
  static bool g_ethStarted = false;
  static uint32_t g_lastEthBeginMs = 0;
#endif


// ============================================================================
// 3) Small compatibility helpers
// ============================================================================
static inline void elementic_yield() {
  #if defined(ESP8266)
    yield();
  #elif defined(ESP32)
    yield();
  #else
    #if defined(yield)
      yield();
    #endif
  #endif
}


// ============================================================================
// 3b) Network status refresh (updates your booleans)
//    Call this after connect attempts and periodically in networkCheckLoop()
// ============================================================================
#if ELEMENTIC_HAS_ETHERNET
static inline bool elementic_eth_has_valid_ip() {
  if (!g_ethStarted) return false;           // ✅ prevents crash on Controllino/AVR
  IPAddress ip = Ethernet.localIP();         // safe now
  if (ip[0] == 0 || ip[0] == 255) return false;
  return true;
}
#endif

static inline bool elementic_wifi_is_connected() {
  #if ELEMENTIC_HAS_WIFI
    return (WiFi.status() == WL_CONNECTED);
  #else
    return false;
  #endif
}

static void elementic_refreshNetworkFlags() {
  #if ELEMENTIC_HAS_WIFI
    WIFIConnected = elementic_wifi_is_connected();
  #else
    WIFIConnected = false;
  #endif

  #if ELEMENTIC_HAS_ETHERNET
    EthernetConnected = elementic_eth_has_valid_ip();   // ✅ guarded
  #else
    EthernetConnected = false;
  #endif

  networkConnected = (WIFIConnected || EthernetConnected);
  elementic_lastNetStatusMs = millis();
}



// ============================================================================
// 4) WiFi helpers + connectToWiFi()
// ============================================================================
#if ELEMENTIC_HAS_WIFI

static inline void elementic_wifi_set_station_mode() {
  #if defined(ESP8266) || defined(ESP32)
    WiFi.mode(WIFI_STA);
  #endif
}

static inline void elementic_wifi_set_hostname(const char* host) {
  if (!host || !host[0]) return;

  #if defined(ESP8266)
    WiFi.hostname(host);
  #elif defined(ESP32)
    WiFi.setHostname(host);
  #else
    (void)host;
  #endif
}

static inline void elementic_wifi_disable_persistent() {
  #if defined(ESP8266)
    WiFi.persistent(false);
  #endif
}

// NOTE: This is still a *bounded blocking* connect (max 5s). For MQTT stability on ESP8266,
// prefer calling it only from the watchdog every few seconds (as you do).
static void connectToWiFi() {
  const String ssidStr = GetStringValue("SSID");
  const String passStr = GetPasswordValue("WIFI Password");

  if (ssidStr.length() == 0) {
    logging(LOG_CAUTION, "WiFi: SSID empty");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) return;

  // Hard reset WiFi state (important after many failed attempts)
  WiFi.mode(WIFI_OFF);
  delay(200);

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);     // ESP8266 stability

  // Optional hostname (only if MQTTName is valid)
  if (MQTTName && MQTTName[0]) WiFi.hostname(MQTTName);

  // IMPORTANT: pass c_str()
  WiFi.begin(ssidStr.c_str(), passStr.c_str());

  logging(LOG_INFO, "WiFi: begin()");

  // Wait (bounded) + show result
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 10000UL) {
    delay(250);
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    logging(LOG_INFO, "WiFi: connected");
    // If you want: print IP
    // Serial.println(WiFi.localIP());
  } else {
    // Print connect result for diagnostics
    const wl_status_t st = WiFi.status();
    char buf[64];
    snprintf(buf, sizeof(buf), "WiFi: failed status=%d", (int)st);
    logging(LOG_CAUTION, buf);

    WiFi.disconnect(); // reset attempt
    delay(200);
  }
}

#endif // ELEMENTIC_HAS_WIFI


// ============================================================================
// 5) Ethernet helpers + connectToEthernet()
// ============================================================================
#if ELEMENTIC_HAS_ETHERNET

static inline void elementic_derive_mac_from_deviceid(uint16_t deviceId, uint8_t outMac[6]) {
  outMac[0] = 0x02;               // local admin, unicast
  outMac[1] = 0xEC;               // "ElementiC" vibe
  outMac[2] = 0x00;
  outMac[3] = (uint8_t)((deviceId >> 8) & 0xFF);
  outMac[4] = (uint8_t)(deviceId & 0xFF);
  outMac[5] = 0x01;
}

static inline bool elementic_read_ip(const String& varName, IPAddress& out) {
  byte* p = GetIPValue(varName);
  if (!p) return false;
  out = IPAddress(p[0], p[1], p[2], p[3]);
  if (out[0] == 0 && out[1] == 0 && out[2] == 0 && out[3] == 0) return false;
  return true;
}

static void connectToEthernet(uint16_t deviceId) {
  IPAddress ip, subnet, gw;
  if (!elementic_read_ip("IP Address", ip))  { logging(LOG_WARNING, "Ethernet: missing IP Address"); return; }
  if (!elementic_read_ip("Subnet", subnet))  { logging(LOG_WARNING, "Ethernet: missing Subnet");     return; }
  if (!elementic_read_ip("Gateway", gw))     { logging(LOG_WARNING, "Ethernet: missing Gateway");    return; }

  uint8_t mac[6];
  elementic_derive_mac_from_deviceid(deviceId, mac);
  IPAddress dns = gw;

  logging(LOG_INFO, "Starting Ethernet...");

  #if (ELEMENTIC_ETH_DRIVER == ELEMENTIC_ETH_DRIVER_WIZNET) || (ELEMENTIC_ETH_DRIVER == ELEMENTIC_ETH_DRIVER_OPTA)
    Ethernet.begin(mac, ip, dns, gw, subnet);
    delay(200);
  #elif (ELEMENTIC_ETH_DRIVER == ELEMENTIC_ETH_DRIVER_TEENSY)
    Ethernet.begin(mac, ip, dns, gw, subnet);
    delay(200);
  #elif (ELEMENTIC_ETH_DRIVER == ELEMENTIC_ETH_DRIVER_PORTENTA)
    Ethernet.begin(mac, ip, dns, gw, subnet);
    delay(200);
  #else
    #error "Ethernet driver selected but connectToEthernet() has no implementation for it."
  #endif

  // ✅ mark stack as initialized
  g_ethStarted = true;
  g_lastEthBeginMs = millis();
}


#endif // ELEMENTIC_HAS_ETHERNET


// ============================================================================
// 6) Unified entry point: setupNetwork() / connectNetwork()
// ============================================================================
void setupNetwork() {
  #if ELEMENTIC_HAS_NETWORK
    DefineString("Mac Address", "809B209BFFB4", PriorityCounter++);
    DefineIP("IP Address", factoryip[0], factoryip[1], factoryip[2], factoryip[3], PriorityCounter++);
    DefineIP("Gateway",   factorygateway[0], factorygateway[1], factorygateway[2], factorygateway[3], PriorityCounter++);
    DefineIP("Subnet",    factorysubnet[0], factorysubnet[1], factorysubnet[2], factorysubnet[3], PriorityCounter++);
    DefineString("Host name", "TempHostname", PriorityCounter++);
  #endif

  #if ELEMENTIC_HAS_WIFI
    DefineString("SSID", "Areanet", PriorityCounter++);
    DefinePassword("WIFI Password", "1933l25331", PriorityCounter++);
  #endif

  #if ELEMENTIC_HAS_ETHERNET
 
    // Nothing else here right now
  #endif
  
  elementic_refreshNetworkFlags();
}

static inline uint16_t elementic_getDeviceId() {
  return ELEMENTIC_DEVICEID;
}

void connectNetwork() {
  const uint16_t deviceId = elementic_getDeviceId();

  if (deviceId == 0) {
    logging(LOG_CAUTION, "connectNetwork(): invalid or missing Device ID");
    elementic_refreshNetworkFlags();
    return;
  }

  // Prefer WiFi first if supported; otherwise ethernet
  #if ELEMENTIC_HAS_WIFI
    connectToWiFi();
    elementic_refreshNetworkFlags();
    if (WIFIConnected) return;
  #endif

  #if ELEMENTIC_HAS_ETHERNET
    connectToEthernet(deviceId);
  #endif

  #if !ELEMENTIC_HAS_WIFI && !ELEMENTIC_HAS_ETHERNET
    logging(LOG_CAUTION, "connectNetwork(): No WiFi or Ethernet compiled for this DEVICEID");
  #endif

  elementic_refreshNetworkFlags();
}


// ============================================================================
// 7) networkCheckLoop() - watchdog (updates booleans every cycle)
// ============================================================================
void networkCheckLoop() {
  static uint32_t lastNetCheckMs = 0;
  const uint32_t now = millis();

  if ((uint32_t)(now - lastNetCheckMs) < 5000UL) {
    return;
  }
  lastNetCheckMs = now;

  // Refresh current flags first (before taking action)
  elementic_refreshNetworkFlags();

  // Device ID from EEPROM (needed for Ethernet MAC derivation etc.)
  const int rawId = GetIntValue("Device ID");
  const uint16_t deviceId = (rawId > 0 && rawId <= 65535) ? (uint16_t)rawId : 0;

  // -------------------------
  // WiFi reconnect
  // -------------------------
  #if ELEMENTIC_HAS_WIFI
    if (!WIFIConnected) {
      connectToWiFi();
      elementic_refreshNetworkFlags();
    }
  #endif

  // -------------------------
  // Ethernet reconnect (W5100/W5x00)
  // -------------------------
  #if ELEMENTIC_HAS_ETHERNET
  if (!elementic_eth_has_valid_ip()) {
    // only re-init if you have a valid deviceId
    if (deviceId != 0) {
      logging(LOG_CAUTION, "Ethernet IP invalid or Ethernet not started. Reinitializing Ethernet...");
      connectToEthernet(deviceId);
    }
  }
#endif

  // -------------------------
  // Ethernet reconnect (other Ethernet drivers)
  // -------------------------
  #if ELEMENTIC_HAS_ETHERNET && (ELEMENTIC_ETH_DRIVER != ELEMENTIC_ETH_DRIVER_WIZNET)
    elementic_refreshNetworkFlags();
    if (!EthernetConnected && deviceId != 0) {
      logging(LOG_CAUTION, "Ethernet IP invalid. Reinitializing Ethernet...");
      connectToEthernet(deviceId);
    }
    elementic_refreshNetworkFlags();
  #endif
}
