/*
 * mqttclient.cpp
 *
 * Implements MQTT connection management, subscriptions, callbacks, and message publishing helpers for supported platforms.
 */

#include <Arduino.h>
#include <PubSubClient.h>
#include "Elementic.h"
#include "deviceIdentification.h"

#if ELEMENTIC_HAS_WIFI
  #if (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_ESP8266)
    #include <ESP8266WiFi.h>
  #elif (ELEMENTIC_WIFI_DRIVER == ELEMENTIC_WIFI_DRIVER_ESP32)
    #include <WiFi.h>
  #else
    #include <WiFi.h>
  #endif
#endif

// Add Ethernet support ONLY if enabled (minor addition)
#if ELEMENTIC_HAS_ETHERNET
  #include <SPI.h>
  #include <Ethernet.h>   // or Ethernet2/Ethernet3 depending on your stack
#endif

bool parseIP(const String& ipStr, IPAddress &ip) {
  // Helper: parse "192.168.1.4" into IPAddress
  int parts[4] = {0,0,0,0};
  if (sscanf(ipStr.c_str(), "%d.%d.%d.%d",
             &parts[0], &parts[1], &parts[2], &parts[3]) == 4) {
    ip = IPAddress(parts[0], parts[1], parts[2], parts[3]);
    return true;
  }
  return false;
}

void reconnect() {//-----------------------Reconnect-----------------------
  static uint32_t lastAttemptMs = 0;
  const uint32_t now = millis();

  // Prevent hammering reconnect (and prevent re-entrancy)
  if (mqttClient.connected()) return;
  if ((uint32_t)(now - lastAttemptMs) < 2000UL) return;
  lastAttemptMs = now;

  logging(LOG_INFO, "Attempting MQTT connection...");
  SendSystem(MQTT, 2);

  // ---- Build stable buffers (avoid String lifetime pitfalls & heap churn) ----
  char server[64];
  char clientId[48];
  char user[48];
  char pass[64];

  strlcpy(server,  GetStringValue("MQTTServer").c_str(),   sizeof(server));
  strlcpy(clientId,GetStringValue("MQTTName").c_str(),     sizeof(clientId));
  strlcpy(user,    GetStringValue("MQTTUsername").c_str(), sizeof(user));
  strlcpy(pass,    GetPasswordValue("MQTTPassword").c_str(), sizeof(pass));

  // Ensure unique-ish clientId (prevents broker kicking sessions with same ID)
  // If MQTTName is already unique per device, you can remove this.
  // Keep it short to avoid packet issues.
  #if defined(ESP8266)
    char uniqueId[64];
    snprintf(uniqueId, sizeof(uniqueId), "%s-%06X", clientId, ESP.getChipId());
    strlcpy(clientId, uniqueId, sizeof(clientId));
  #endif

  // ---- Configure MQTT client BEFORE connect ----
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(5);

  IPAddress brokerIp;
  if (parseIP(server, brokerIp)) {
    SendSystem(ETHERNET, 1);
    mqttClient.setServer(brokerIp, 1883);
  } else {
    mqttClient.setServer(server, 1883);
  }

  // ---- Connect and CHECK result ----
  bool ok = mqttClient.connect(clientId, user, pass);
  if (ok) {
    logging(LOG_INFO, "MQTT Connected");
    SendSystem(MQTT, 1);

    // Subscribe QUICKLY, no heavy work here
    MQTTSubscribe();

    // Optional: publish a small online marker
    // mqttClient.publish("elementic/status", "online", true);

  } else {
    SendSystem(MQTT, 0);
    snprintf(msgmqtt2, sizeof(msgmqtt2), "MQTT connect failed, state=%d", mqttClient.state());
    logging(LOG_INFO, msgmqtt2);

    #if defined(ESP8266)
        snprintf(msgmqtt2, sizeof(msgmqtt2), "Heap after failed connect: %u", ESP.getFreeHeap());
        logging(LOG_INFO, msgmqtt2);
    #endif
  }

  // Give ESP background tasks time
  yield();
}

void MQTTSetupVariables(){
  DefineString("MQTTName", MQTTName, PriorityCounter++);
  DefineString("MQTTServer", MQTTServer, PriorityCounter++);
  DefineString("MQTTUsername", MQTTUsername, PriorityCounter++);
  DefinePassword("MQTTPassword", MQTTPassword, PriorityCounter++);
}

void MQTTSetupClient(){//-----------------------MQTT Setup-----------------------

  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(5);
  // IMPORTANT: check success
  bool ok = mqttClient.setBufferSize(512);
  if (!ok) {
    // Fall back to smaller (or keep default)
    mqttClient.setBufferSize(256);
  }
}

void SendOutputValueMQTT(byte id) {

  if (!mqttClient.connected()) return;
  if (!MQTT_Output[id] || MQTT_Output[id][0] == '\0') return;

  char suffixBuf[32];

  switch (OutputType[id]) {

    case 1: // Lamp aan/uit
    case 5: // Lamp aan/uit (Ext.)
      strncpy_P(suffixBuf, onoffstatus, sizeof(suffixBuf) - 1);
      suffixBuf[sizeof(suffixBuf) - 1] = '\0';
      snprintf(msgmqtt2, sizeof(msgmqtt2), "%s",
               OutputValueActual[id] == 0 ? "OFF" : "ON");
      break;

    case 2: // Lamp PWM
    case 4: // LAMP DMX
    default:
      strncpy_P(suffixBuf, dimvalue_status, sizeof(suffixBuf) - 1);
      suffixBuf[sizeof(suffixBuf) - 1] = '\0';
      snprintf(msgmqtt2, sizeof(msgmqtt2), "%u",
               (unsigned)OutputValueActual[id]);
      break;

    case 3:
    case 6:
      return;  // no publish
  }

  snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[id], suffixBuf);

  mqttClient.publish(msgmqtt, msgmqtt2);
}

void callback(char* topic, byte* payload, unsigned int length) {//-----------------------Callback-----------------------

  // 1) Safe log (OPTIONAL: you can comment out entirely to reduce load)
  // NOTE: fixed format string bug (only one %s)
  snprintf(msgmqtt2, sizeof(msgmqtt2), "Callback topic: %s", topic);
  // logging(LOG_INFO, msgmqtt2);

  // 2) Copy payload to a safe null-terminated buffer
  char pbuf[64];
  unsigned int n = (length < sizeof(pbuf) - 1) ? length : (sizeof(pbuf) - 1);
  memcpy(pbuf, payload, n);
  pbuf[n] = '\0';

  long pval = strtol(pbuf, nullptr, 10);

  // logging(LOG_INFO, "Callback 2");
  // 3) Clamp OutputCounter to prevent OOB even if corrupted
  byte maxOut = OutputCounter;
  if (maxOut > OutputChannels) maxOut = OutputChannels;

  for (byte i = 1; i <= maxOut; ++i) {
    //  snprintf(msgmqtt2, sizeof(msgmqtt2), "Callback Output id: %d of total of %d ids", i, maxOut);
    //  logging(LOG_INFO, msgmqtt2);

    const char* base = MQTT_Output[i];
    if (!base || base[0] == '\0') continue;

    // Topic must start with base
    size_t baseLen = strnlen(base, MAXTOPICLENGTH);
    if (baseLen == 0 || baseLen >= MAXTOPICLENGTH) continue;
    if (strncmp(topic, base, baseLen) != 0) continue;

    const char* suffix = topic + baseLen;
    // snprintf(msgmqtt2, sizeof(msgmqtt2), "Callback suffix: %s", suffix);
    // logging(LOG_INFO, msgmqtt2);

    // ----- on/off (…/onoff_set) -----
    if (streq_P(suffix, onoffset)) {

      // logging(LOG_INFO, "Callback on-off");
      bool turnOn = (strcasecmp(pbuf, "ON") == 0) || (pval > 0);

      if (turnOn) {
        if (OutputType[i] == 1 || OutputType[i] == 5) {
          OutputValueActual[i] = 1;
          SendOutputValueSerial(i, 1);
        } else {
          OutputValueActual[i] = OutputDefaultValue[i];
        }
      } else {
        OutputValueActual[i] = 0;
        SendOutputValueSerial(i, 0);
      }

      MQTTUpdate[i] = true;
      continue;
    }

    // ----- dim value set (…/dimvalue_set) -----
    if (streq_P(suffix, dimvalue_set)) {
      logging(LOG_INFO, "Callback dimvalue");
      int v = (byte)pval;

      if (v <= 0) {
        OutputValueActual[i] = 0;
        SendOutputValueSerial(i, 0);
      } else {
        if (v < (byte)OutputMinValue[i]) v = OutputMinValue[i];
        if (v > (byte)OutputMaxValue[i]) v = OutputMaxValue[i];
        OutputValueActual[i] = (byte)v;
        SendOutputValueSerial(i, v);
      }
      logging(LOG_INFO, "Callback dimvalue end");

      MQTTUpdate[i] = true;
      continue;
    }

    // ----- dim value status (…/dimvalue_status) for type 6 -----
    if (streq_P(suffix, dimvalue_status)) {
      // logging(LOG_INFO, "Dimvalue status");
      if (OutputType[i] == 6) {
        byte v = (byte)pval;
        if (v <= 0) {
          OutputValueActual[i] = 0;
        } else {
          if (v < (byte)OutputMinValue[i]) v = OutputMinValue[i];
          if (v > (byte)OutputMaxValue[i]) v = OutputMaxValue[i];
          OutputValueActual[i] = (byte)v;
        }
        OutputValueMemory[i] = OutputValueActual[i];
      }
      continue;
    }

    // ----- default value set (…/defaultvalue_set) -----
    if (streq_P(suffix, defaultvalue_set)) {
      // logging(LOG_INFO, "Callback default");
      int v = (int)pval;
      if (v > 0) {
        if (v < (byte)OutputMinValue[i]) v = OutputMinValue[i];
        if (v > (byte)OutputMaxValue[i]) v = OutputMaxValue[i];
        OutputDefaultValue[i] = (byte)v;
      }
      continue;
    }
    // logging(LOG_INFO, "Callback end 1");
  }
  // logging(LOG_INFO, "Callback end 2");
}

void MQTTLoop() {

  static uint32_t lastReconnectAttempt = 0;
  static bool lastMqttState = false;

  const uint32_t now = millis();

  // ---------------------------
  // 1. If connected
  // ---------------------------
  if (mqttClient.connected()) {

    // Detect transition to connected
    if (!lastMqttState) {
      logging(LOG_INFO, "MQTT Connected");
      SendSystem(MQTT, 1);
      lastMqttState = true;
    }

    mqttClient.loop();

  }
  // ---------------------------
  // 2. If NOT connected
  // ---------------------------
  else {

    // Detect transition to disconnected
    if (lastMqttState) {
      logging(LOG_CAUTION, "MQTT Disconnected");
      SendSystem(MQTT, 0);
      lastMqttState = false;
    }

    // Attempt reconnect every 5 seconds
    if ((uint32_t)(now - lastReconnectAttempt) >= 5000UL) {
      lastReconnectAttempt = now;
      reconnect();
    }

    return;   // do NOT publish when disconnected
  }

  // ---------------------------
  // 3. Publish pending updates
  // ---------------------------
  uint8_t maxOut = OutputCounter;
  if (maxOut > OutputChannels) maxOut = OutputChannels;

  for (uint8_t i = 1; i <= maxOut; i++) {
    if (MQTTUpdate[i]) {
      SendOutputValueMQTT(i);
      MQTTUpdate[i] = false;
    }

#if defined(ESP8266)
    if ((i & 3) == 0) yield();
#endif
  }
}
