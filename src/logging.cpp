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
        snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/LOG");
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
            // compute length safely (max 49, keep room for '\0' if present)
            uint8_t len = 0;
            while (len < 49 && text[len] != '\0') {
                len++;
            }

            Serial.print("S01"); // Initiate
            Serial.print("S02"); // Marker

            serialwrite((uint8_t)4);          // Type01: 4 = Logging
            serialwrite((uint8_t)level);      // Type02: 0 log, 1 caution, 2 warning
            serialwrite((uint8_t)0);          // id (optional)
            serialwrite((uint8_t)len);        // length

            // Write payload bytes
            for (uint8_t i = 0; i < len; i++) {
                serialwrite((uint8_t)text[i]);
            }

            Serial.print("E02");
            Serial.print("E01");
        }
    }
