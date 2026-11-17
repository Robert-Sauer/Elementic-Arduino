/*
 * logging.cpp
 *
 * Implements centralized logging helpers for serial and MQTT output with
 * consistent severity tagging. On AVR, literal messages can remain in flash
 * through the __FlashStringHelper overload instead of consuming static SRAM.
 */

#include <Arduino.h>
#include <string.h>
#include "Elementic.h"

namespace {
#if ELEMENTIC_USE_MQTT

void buildLogTopic() {
    const String mqttName = GetStringValue("MQTTName");
    const size_t nameLength = min(mqttName.length(), sizeof(msgmqtt) - 5U);
    memcpy(msgmqtt, mqttName.c_str(), nameLength);
    size_t position = nameLength;
    msgmqtt[position++] = '/';
    msgmqtt[position++] = 'L';
    msgmqtt[position++] = 'O';
    msgmqtt[position++] = 'G';
    msgmqtt[position] = '\0';
}

size_t writeSeverityPrefix(LogLevel level) {
    msgmqtt2[0] = '[';
    switch (level) {
        case LOG_INFO:
            msgmqtt2[1] = 'L'; msgmqtt2[2] = 'O'; msgmqtt2[3] = 'G';
            break;
        case LOG_CAUTION:
            msgmqtt2[1] = 'C'; msgmqtt2[2] = 'A'; msgmqtt2[3] = 'U';
            break;
        case LOG_WARNING:
            msgmqtt2[1] = 'W'; msgmqtt2[2] = 'A'; msgmqtt2[3] = 'R';
            break;
        default:
            msgmqtt2[1] = 'E'; msgmqtt2[2] = 'R'; msgmqtt2[3] = 'R';
            break;
    }
    msgmqtt2[4] = ']';
    msgmqtt2[5] = ':';
    msgmqtt2[6] = ' ';
    msgmqtt2[7] = '\0';
    return 7U;
}

void appendRamText(const char* text, size_t position) {
    if (text == 0 || position >= sizeof(msgmqtt2)) return;
    const size_t available = sizeof(msgmqtt2) - position - 1U;
    strncpy(msgmqtt2 + position, text, available);
    msgmqtt2[sizeof(msgmqtt2) - 1U] = '\0';
}

void buildRamLogPayload(LogLevel level, const char* text) {
    if (text == msgmqtt2) {
        // Many existing diagnostics format their text in msgmqtt2. Preserve
        // that text before adding the severity prefix in the same buffer.
        const size_t prefixLength = 7U;
        const size_t maxTextLength = sizeof(msgmqtt2) - prefixLength - 1U;
        size_t textLength = 0U;
        while (textLength < maxTextLength && msgmqtt2[textLength] != '\0') {
            ++textLength;
        }
        memmove(msgmqtt2 + prefixLength + 1U, msgmqtt2, textLength);
        writeSeverityPrefix(level);
        memmove(msgmqtt2 + prefixLength,
                msgmqtt2 + prefixLength + 1U,
                textLength);
        msgmqtt2[prefixLength + textLength] = '\0';
        return;
    }

    appendRamText(text, writeSeverityPrefix(level));
}

bool mqttLogSinkReady() {
    #if ELEMENTIC_USE_MQTT
      // Diagnostics may use an already-connected, softly enabled MQTT
      // session. Logging itself never starts or restores a connection.
      return logtoMQTT && elementicMqttEnabled() && mqttClient.connected();
    #else
      return false;
    #endif
}

#if defined(ARDUINO_ARCH_AVR)
void appendFlashText(const __FlashStringHelper* text, size_t position) {
    if (text == 0 || position >= sizeof(msgmqtt2)) return;
    const char* flash = reinterpret_cast<const char*>(text);
    const size_t available = sizeof(msgmqtt2) - position - 1U;
    size_t copied = 0U;
    while (copied < available) {
        const char value = static_cast<char>(pgm_read_byte(flash + copied));
        if (value == '\0') break;
        msgmqtt2[position + copied] = value;
        ++copied;
    }
    msgmqtt2[position + copied] = '\0';
}
#endif

#endif  // ELEMENTIC_USE_MQTT
}  // namespace

void logging(LogLevel level, const char* text) {
    if (text == 0) return;
    // Serial first: callers are allowed to format into msgmqtt2, which is also
    // the MQTT log payload buffer and is rewritten below.
    if (logtoSerial) sendSerialLog(level, text);
    #if ELEMENTIC_USE_MQTT
    if (mqttLogSinkReady()) {
        buildRamLogPayload(level, text);
        buildLogTopic();
        mqttClient.publish(msgmqtt, msgmqtt2);
    }
    #endif
}

#if defined(ARDUINO_ARCH_AVR)
void logging(LogLevel level, const __FlashStringHelper* text) {
    if (text == 0) return;
    if (logtoSerial) sendSerialLog(level, text);
    #if ELEMENTIC_USE_MQTT
    if (mqttLogSinkReady()) {
        buildLogTopic();
        appendFlashText(text, writeSeverityPrefix(level));
        mqttClient.publish(msgmqtt, msgmqtt2);
    }
    #endif
}
#endif
