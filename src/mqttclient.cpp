#include <Arduino.h>
#include <PubSubClient.h>
#include "Elementic.h"


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



void reconnect() {
  Serial.println("Attempting MQTT connection...");
  SendSystem(MQTT, 2);
  Serial.printf(
    "[MQTT] connect(Name=\"%s\", User=\"%s\", Pass=\"%s\")\r\n",
    GetStringValue("MQTTName").c_str(),
    GetStringValue("MQTTUsername").c_str(),
    GetPasswordValue("MQTTPassword").c_str()
);
  IPAddress brokerIp;
  if (parseIP(GetStringValue("MQTTServer"), brokerIp)) {
    SendSystem(ETHERNET,1);
    mqttClient.setServer(brokerIp, 1883);
  } else {
    mqttClient.setServer(GetStringValue("MQTTServer").c_str(), 1883);
  }
  mqttClient.connect(GetStringValue("MQTTName").c_str(), GetStringValue("MQTTUsername").c_str(), GetPasswordValue("MQTTPassword").c_str());
  Serial.println("After connect string");
  if (mqttClient.connected()) {
    Serial.println("Connected");
    MQTTSubscribe();
    SendSystem(MQTT, 1);
    mqttClient.setKeepAlive(60);
  } else {
    SendSystem(MQTT, 0);
    Serial.println("Connection failed");
    Serial.print("Connection failed, state = ");
    Serial.println(mqttClient.state());
  }
}

void MQTTSetup(){

mqttClient.setCallback(callback);
reconnect();


}

void MQTTLoop(){
    mqttClient.loop();



}
