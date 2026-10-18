/*
 * logging.cpp
 *
 * Implements centralized logging helpers for serial and MQTT output with consistent severity tagging.
 */

#include <Arduino.h>
#include <string.h>
#include "Elementic.h"
void logging(LogLevel level, const char* text) {
    if(logtoMQTT){
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", GetStringValue("MQTTName").c_str(), "/LOG");
        switch (level){
            case 0:
                snprintf(msgmqtt2, sizeof(msgmqtt2), "[LOG]: %s", text);
            break;
            case 1:
                snprintf(msgmqtt2, sizeof(msgmqtt2), "[CAU]: %s", text);
            break;
            case 2:
                snprintf(msgmqtt2, sizeof(msgmqtt2), "[WAR]: %s", text);
            break;
            default:
                snprintf(msgmqtt2, sizeof(msgmqtt2), "[ERR]: %s", text);
            break;
            }
        mqttClient.publish(msgmqtt, msgmqtt2);
        }
    if(logtoSerial){
            sendSerialLog(level, text);
        }
    }
