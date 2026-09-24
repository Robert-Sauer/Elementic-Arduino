/*
 * serial.cpp
 *
 * Implements the Elementic serial protocol, including framing, EEPROM serialization, variable synchronization, and command handling.
 */

#include <Arduino.h>
#include "Elementic.h"
#include "ElementicEeprom.h"
#define MAX_SERIAL_BUFFER 500

namespace {

void putU16Le(uint8_t* destination, uint16_t value) {
    destination[0] = static_cast<uint8_t>(value);
    destination[1] = static_cast<uint8_t>(value >> 8);
}

void putU32Le(uint8_t* destination, uint32_t value) {
    for (uint8_t i = 0; i < 4U; ++i) destination[i] = static_cast<uint8_t>(value >> (i * 8U));
}

void putU64Le(uint8_t* destination, uint64_t value) {
    for (uint8_t i = 0; i < 8U; ++i) destination[i] = static_cast<uint8_t>(value >> (i * 8U));
}

uint16_t getU16Le(const uint8_t* source) {
    return static_cast<uint16_t>(source[0]) |
           static_cast<uint16_t>(static_cast<uint16_t>(source[1]) << 8);
}

void sendProtocolAnnouncement() {
    Serial.write(reinterpret_cast<const uint8_t*>("S011"), 4U);
    Serial.write(static_cast<uint8_t>(ELEMENTIC_WIRE_PROTOCOL_VERSION));
    Serial.write(reinterpret_cast<const uint8_t*>("E01"), 3U);
}

bool sendV2Frame(uint8_t kind, uint16_t sequence, const uint8_t* payload,
                 uint16_t payloadLength) {
    uint8_t frame[256];
    size_t frameLength = 0U;
    const elementic::V2CodecResult result = elementic::encodeV2Frame(
        kind, sequence, payload, payloadLength, frame, sizeof(frame), &frameLength);
    if (result != elementic::V2_CODEC_OK) return false;
    Serial.write(frame, frameLength);
    return true;
}

bool sendV2Tlv(uint8_t kind, uint16_t sequence, uint8_t category,
               uint8_t field, uint16_t item, const uint8_t* value,
               uint16_t length) {
    if (length > 186U || (value == 0 && length != 0U)) return false;
    uint8_t payload[192];
    payload[0] = category;
    payload[1] = field;
    putU16Le(payload + 2U, item);
    putU16Le(payload + 4U, length);
    if (length != 0U) memcpy(payload + 6U, value, length);
    return sendV2Frame(kind, sequence, payload, static_cast<uint16_t>(length + 6U));
}

uint16_t boundedTextLength(const char* value, uint16_t maximum) {
    if (value == 0) return 0U;
    return static_cast<uint16_t>(strnlen(value, maximum));
}

void sendV2End(uint16_t sequence, uint32_t capture, uint32_t generation,
               uint16_t records) {
    uint8_t payload[10];
    putU32Le(payload, capture);
    putU32Le(payload + 4U, generation);
    putU16Le(payload + 8U, records);
    sendV2Frame(0x06U, sequence, payload, sizeof(payload));
}

void sendV2Info(uint16_t sequence) {
    uint16_t count = 0U;
    uint8_t value[16];
    const auto sendBytes = [&](uint8_t field, const uint8_t* bytes, uint16_t length) {
        if (sendV2Tlv(0x02U, sequence, 3U, field, 0U, bytes, length)) ++count;
    };
    const auto sendText = [&](uint8_t field, const char* text, uint16_t maximum) {
        sendBytes(field, reinterpret_cast<const uint8_t*>(text), boundedTextLength(text, maximum));
    };
    putU16Le(value, ELEMENTIC_DEVICEID); sendBytes(9U, value, 2U);
    value[0] = 2U; sendBytes(0x20U, value, 1U);
    sendText(0x21U, ELEMENTIC_LIBRARY_VERSION, 31U);
    sendText(0x22U, ELEMENTIC_APP_VERSION, 31U);
    putU64Le(value, ELEMENTIC_FIRMWARE_PROJECT_ID); sendBytes(0x23U, value, 8U);
    sendText(0x24U, ELEMENTIC_TARGET_PROFILE, 48U);
    putU32Le(value, ELEMENTIC_TARGET_REVISION); sendBytes(0x25U, value, 4U);
    sendText(0x26U, ELEMENTIC_MCU_FAMILY, 16U);
    if (ELEMENTIC_BUILD_ULID[0] != '\0') sendText(0x27U, ELEMENTIC_BUILD_ULID, 26U);
    putU32Le(value, ELEMENTIC_PROJECT_REVISION); sendBytes(0x28U, value, 4U);
    putU16Le(value, ELEMENTIC_CONFIG_SCHEMA); sendBytes(0x2AU, value, 2U);
    putU16Le(value, ELEMENTIC_STORAGE_SCHEMA); sendBytes(0x2BU, value, 2U);
    putU16Le(value, OutputChannels);
    putU16Le(value + 2U, InputChannels);
    putU16Le(value + 4U, totalVariables);
    putU16Le(value + 6U, MAXNAMELENGTH - 1U);
    putU16Le(value + 8U, MAXTOPICLENGTH - 1U);
    putU16Le(value + 10U, MAX_STRING_LENGTH - 1U);
    value[12] = INOUT_MATRIX_COLS;
    sendBytes(0x2CU, value, 13U);
    value[0] = static_cast<uint8_t>(elementicMacStatus); sendBytes(0x2DU, value, 1U);
    value[0] = static_cast<uint8_t>(elementicStorageStatus); sendBytes(0x2EU, value, 1U);
    if (ELEMENTIC_RELEASE_ULID[0] != '\0') sendText(0x2FU, ELEMENTIC_RELEASE_ULID, 26U);
    const uint8_t outputTypes[] = {1, 2, 3, 4, 5, 6, 7};
    sendBytes(0x30U, outputTypes, sizeof(outputTypes));
    const uint8_t inputTypes[] = {1};
    sendBytes(0x32U, inputTypes, sizeof(inputTypes));
    sendV2End(sequence, 0U, 0U, count);
}

uint32_t configurationGeneration = 1U;
uint32_t snapshotCapture = 1U;

struct SavedSnapshotContext {
    uint16_t sequence;
    uint16_t sent;
};

void sendV2Result(uint16_t sequence, uint8_t originalKind, uint8_t status);

bool sendSavedRecord(uint8_t category, uint8_t field, uint16_t item,
                     const uint8_t* value, uint16_t length, void* opaque) {
    SavedSnapshotContext* context = static_cast<SavedSnapshotContext*>(opaque);
    if (!sendV2Tlv(0x05U, context->sequence, category, field, item, value, length)) return false;
    ++context->sent;
    return true;
}

void sendV2Snapshot(uint16_t sequence, uint8_t source) {
    if (source > 1U) {
        sendV2Result(sequence, 0x03U, 1U);
        return;
    }
    if (source == 1U) {
        uint32_t savedGeneration = 0U;
        uint16_t expectedRecords = 0U;
        if (!elementicVisitSavedConfiguration(0, 0, &savedGeneration, &expectedRecords)) {
            uint8_t result[6] = {0x03U, 5U, 0, 0, 0, 0};
            putU32Le(result + 2U, configurationGeneration);
            sendV2Frame(0x12U, sequence, result, sizeof(result));
            return;
        }
        const uint32_t capture = snapshotCapture++;
        uint8_t begin[10];
        putU32Le(begin, capture);
        putU32Le(begin + 4U, savedGeneration);
        begin[8] = source;
        begin[9] = static_cast<uint8_t>(elementicStorageStatus);
        sendV2Frame(0x04U, sequence, begin, sizeof(begin));
        SavedSnapshotContext context = {sequence, 0U};
        if (!elementicVisitSavedConfiguration(sendSavedRecord, &context, 0, 0) ||
            context.sent != expectedRecords) {
            sendV2Result(sequence, 0x03U, 5U);
            return;
        }
        sendV2End(sequence, capture, savedGeneration, context.sent);
        return;
    }
    const uint32_t capture = snapshotCapture++;
    uint8_t begin[10];
    putU32Le(begin, capture);
    putU32Le(begin + 4U, configurationGeneration);
    begin[8] = source;
    begin[9] = static_cast<uint8_t>(elementicStorageStatus);
    sendV2Frame(0x04U, sequence, begin, sizeof(begin));
    uint16_t count = 0U;
    const auto send = [&](uint8_t category, uint8_t field, uint16_t item,
                          const uint8_t* bytes, uint16_t length) {
        if (sendV2Tlv(0x05U, sequence, category, field, item, bytes, length)) ++count;
    };
    uint8_t bytes[10];
    for (uint16_t item = 1U; item <= min(OutputCounter, static_cast<uint8_t>(OutputChannels)); ++item) {
        putU16Le(bytes, OutputPin[item]); send(0, 0, item, bytes, 2);
        bytes[0] = OutputType[item]; send(0, 1, item, bytes, 1);
        putU16Le(bytes, OutputRelay[item]); send(0, 2, item, bytes, 2);
        bytes[0] = OutputMinValue[item]; send(0, 3, item, bytes, 1);
        bytes[0] = OutputMaxValue[item]; send(0, 4, item, bytes, 1);
        bytes[0] = OutputDefaultValue[item]; send(0, 5, item, bytes, 1);
        send(0, 6, item, reinterpret_cast<const uint8_t*>(Output_Name[item]), boundedTextLength(Output_Name[item], MAXNAMELENGTH));
        send(0, 7, item, reinterpret_cast<const uint8_t*>(MQTT_Output[item]), boundedTextLength(MQTT_Output[item], MAXTOPICLENGTH));
        bytes[0] = OutputValueActual[item]; send(0, 8, item, bytes, 1);
        bytes[0] = OutputStatus[item]; send(0, 9, item, bytes, 1);
        putU16Le(bytes, OutputChannel[item]); send(0, 10, item, bytes, 2);
    }
    for (uint16_t item = 1U; item <= static_cast<uint16_t>(min(SwitchCounter, static_cast<int>(InputChannels))); ++item) {
        putU16Le(bytes, SwitchPin[item]); send(1, 0, item, bytes, 2);
        const uint8_t links = min(InOutMatrixCouter[item], static_cast<uint8_t>(INOUT_MATRIX_COLS));
        for (uint8_t i = 0; i < links; ++i) putU16Le(bytes + i * 2U, InOutMatrix[item][i]);
        send(1, 1, item, bytes, links * 2U);
        send(1, 2, item, reinterpret_cast<const uint8_t*>(MQTT_Input[item]), boundedTextLength(MQTT_Input[item], MAXTOPICLENGTH));
        bytes[0] = 1U; send(1, 3, item, bytes, 1);
        send(1, 5, item, reinterpret_cast<const uint8_t*>(Input_Name[item]), boundedTextLength(Input_Name[item], MAXNAMELENGTH));
        bytes[0] = InputStatus[item]; send(1, 6, item, bytes, 1);
    }
    for (int priority = 1; priority < PriorityCounter; ++priority) {
        const int index = findVariableIndexByPriority(priority);
        if (index < 0) continue;
        bytes[0] = static_cast<uint8_t>(variables[index].type); send(2, 0, priority, bytes, 1);
        send(2, 1, priority, reinterpret_cast<const uint8_t*>(variables[index].name.c_str()), variables[index].name.length());
        switch (variables[index].type) {
            case DYNAMIC_STRING: send(2, 2, priority, reinterpret_cast<const uint8_t*>(variables[index].value.strVal), boundedTextLength(variables[index].value.strVal, MAX_STRING_LENGTH)); break;
            case DYNAMIC_PASSWORD: send(2, 2, priority, reinterpret_cast<const uint8_t*>(variables[index].value.passVal), boundedTextLength(variables[index].value.passVal, MAX_STRING_LENGTH)); break;
            case DYNAMIC_BYTE:
            case DYNAMIC_BOOL: bytes[0] = variables[index].value.byteVal; send(2, 2, priority, bytes, 1); break;
            case DYNAMIC_INT: putU16Le(bytes, static_cast<uint16_t>(variables[index].value.intVal)); send(2, 2, priority, bytes, 2); break;
            case DYNAMIC_IP: send(2, 2, priority, variables[index].value.ipVal, 4); break;
        }
        bytes[0] = DynamicVariableStatus[index]; send(2, 5, priority, bytes, 1);
    }
    sendV2End(sequence, capture, configurationGeneration, count);
}

void sendV2Result(uint16_t sequence, uint8_t originalKind, uint8_t status) {
    uint8_t payload[6] = {originalKind, status, 0, 0, 0, 0};
    putU32Le(payload + 2U, configurationGeneration);
    sendV2Frame(0x12U, sequence, payload, sizeof(payload));
}

bool validUtf8NoNul(const uint8_t* value, uint16_t length) {
    uint16_t position = 0U;
    while (position < length) {
        const uint8_t first = value[position++];
        if (first == 0U) return false;
        if (first < 0x80U) continue;
        uint8_t continuation = 0U;
        uint32_t codepoint = 0U;
        if ((first & 0xE0U) == 0xC0U) { continuation = 1U; codepoint = first & 0x1FU; if (codepoint < 2U) return false; }
        else if ((first & 0xF0U) == 0xE0U) { continuation = 2U; codepoint = first & 0x0FU; }
        else if ((first & 0xF8U) == 0xF0U) { continuation = 3U; codepoint = first & 0x07U; }
        else return false;
        if (position + continuation > length) return false;
        for (uint8_t i = 0U; i < continuation; ++i) {
            const uint8_t next = value[position++];
            if ((next & 0xC0U) != 0x80U) return false;
            codepoint = (codepoint << 6) | (next & 0x3FU);
        }
        if ((continuation == 2U && codepoint < 0x800U) ||
            (continuation == 3U && codepoint < 0x10000UL) ||
            (codepoint >= 0xD800U && codepoint <= 0xDFFFU) || codepoint > 0x10FFFFUL) return false;
    }
    return true;
}

struct V2Write {
    uint8_t category;
    uint8_t field;
    uint16_t item;
    uint16_t length;
    const uint8_t* value;
};

bool parseV2Write(const elementic::V2FrameView& frame, V2Write& write) {
    if (frame.payloadLength < 6U) return false;
    write.category = frame.payload[0];
    write.field = frame.payload[1];
    write.item = getU16Le(frame.payload + 2U);
    write.length = getU16Le(frame.payload + 4U);
    write.value = frame.payload + 6U;
    if (write.length != frame.payloadLength - 6U) return false;
    if (write.category == 0U) {
        if (write.item == 0U || write.item > OutputChannels) return false;
        if (write.field == 0U || write.field == 2U || write.field == 10U) return write.length == 2U && getU16Le(write.value) <= 255U;
        if (write.field == 1U) return write.length == 1U && write.value[0] >= 1U && write.value[0] <= 7U;
        if (write.field >= 3U && write.field <= 5U) return write.length == 1U;
        if (write.field == 6U) return write.length <= 63U && write.length < MAXNAMELENGTH && validUtf8NoNul(write.value, write.length);
        if (write.field == 7U) return write.length <= 127U && write.length < MAXTOPICLENGTH && validUtf8NoNul(write.value, write.length);
        if (write.field == 8U) return write.length == 1U;
        return false;
    }
    if (write.category == 1U) {
        if (write.item == 0U || write.item > InputChannels) return false;
        if (write.field == 0U) return write.length == 2U && getU16Le(write.value) <= 255U;
        if (write.field == 1U) {
            if ((write.length & 1U) != 0U || write.length > INOUT_MATRIX_COLS * 2U) return false;
            for (uint16_t i = 0U; i < write.length; i += 2U) {
                const uint16_t output = getU16Le(write.value + i);
                if (output == 0U || output > OutputCounter) return false;
            }
            return true;
        }
        if (write.field == 2U) return write.length <= 127U && write.length < MAXTOPICLENGTH && validUtf8NoNul(write.value, write.length);
        if (write.field == 3U) return write.length == 1U && write.value[0] == 1U;
        if (write.field == 5U) return write.length <= 63U && write.length < MAXNAMELENGTH && validUtf8NoNul(write.value, write.length);
        return false;
    }
    if (write.category == 2U) {
        if (write.field != 2U || write.item == 0U) return false;
        const int index = findVariableIndexByPriority(write.item);
        if (index < 0 || index >= totalVariables) return false;
        switch (variables[index].type) {
            case DYNAMIC_STRING:
            case DYNAMIC_PASSWORD: return write.length <= 127U && write.length < MAX_STRING_LENGTH && validUtf8NoNul(write.value, write.length);
            case DYNAMIC_BYTE: return write.length == 1U;
            case DYNAMIC_BOOL: return write.length == 1U && write.value[0] <= 1U;
            case DYNAMIC_INT: return write.length == 2U;
            case DYNAMIC_IP: return write.length == 4U;
        }
        return false;
    }
    return write.category == 3U && write.field == 41U && write.item == 0U &&
           write.length == 32U && elementicMacStatus == ELEMENTIC_MAC_PROVISIONING_REQUIRED;
}

uint8_t applyV2Write(const V2Write& write) {
    if (write.category == 0U) {
        if (write.item > OutputCounter) {
            if (write.field != 1U) return 1U;
            OutputCounter = static_cast<uint8_t>(write.item);
        }
        switch (write.field) {
            case 0: OutputPin[write.item] = static_cast<uint8_t>(getU16Le(write.value)); break;
            case 1: OutputType[write.item] = write.value[0]; break;
            case 2: OutputRelay[write.item] = static_cast<uint8_t>(getU16Le(write.value)); setrelays(); break;
            case 3: OutputMinValue[write.item] = write.value[0]; break;
            case 4: OutputMaxValue[write.item] = write.value[0]; break;
            case 5: OutputDefaultValue[write.item] = write.value[0]; break;
            case 6: memcpy(Output_Name[write.item], write.value, write.length); Output_Name[write.item][write.length] = '\0'; break;
            case 7: memcpy(MQTT_Output[write.item], write.value, write.length); MQTT_Output[write.item][write.length] = '\0'; break;
            case 8: OutputValueActual[write.item] = write.value[0]; break;
            case 10: OutputChannel[write.item] = static_cast<uint8_t>(getU16Le(write.value)); break;
        }
    } else if (write.category == 1U) {
        if (write.item > static_cast<uint16_t>(SwitchCounter)) {
            if (write.field != 3U) return 1U;
            SwitchCounter = write.item;
        }
        if (write.field == 0U) SwitchPin[write.item] = static_cast<uint8_t>(getU16Le(write.value));
        else if (write.field == 1U) {
            InOutMatrixCouter[write.item] = static_cast<uint8_t>(write.length / 2U);
            for (uint8_t i = 0; i < InOutMatrixCouter[write.item]; ++i) InOutMatrix[write.item][i] = static_cast<uint8_t>(getU16Le(write.value + i * 2U));
        } else if (write.field == 2U) { memcpy(MQTT_Input[write.item], write.value, write.length); MQTT_Input[write.item][write.length] = '\0'; }
        else if (write.field == 5U) { memcpy(Input_Name[write.item], write.value, write.length); Input_Name[write.item][write.length] = '\0'; }
    } else if (write.category == 2U) {
        const int index = findVariableIndexByPriority(write.item);
        switch (variables[index].type) {
            case DYNAMIC_STRING:
                memcpy(variables[index].value.strVal, write.value, write.length); variables[index].value.strVal[write.length] = '\0';
                if (variables[index].name == "Mac Address") {
                    elementicApplySharedMac(false);
                    if (elementicMacStatus == ELEMENTIC_MAC_CORRECTED) return 8U;
                    if (elementicMacStatus == ELEMENTIC_MAC_PROVISIONING_REQUIRED) return 9U;
                }
                break;
            case DYNAMIC_PASSWORD: memcpy(variables[index].value.passVal, write.value, write.length); variables[index].value.passVal[write.length] = '\0'; break;
            case DYNAMIC_BYTE:
            case DYNAMIC_BOOL: variables[index].value.byteVal = write.value[0]; break;
            case DYNAMIC_INT: variables[index].value.intVal = static_cast<int16_t>(getU16Le(write.value)); break;
            case DYNAMIC_IP: memcpy(variables[index].value.ipVal, write.value, 4U); break;
        }
        DynamicVariablesUpdated = true;
    } else {
        char unitUlid[27];
        memcpy(unitUlid, write.value + 6U, 26U);
        unitUlid[26] = '\0';
        if (!elementicProvisionMac(write.value, unitUlid)) return 5U;
    }
    ++configurationGeneration;
    return 0U;
}

uint16_t lastMutationSequence = 0U;
uint32_t lastMutationHash = 0U;
uint8_t lastMutationKind = 0U;
uint8_t lastMutationStatus = 0U;

uint8_t processV2Command(const elementic::V2FrameView& decoded) {
    if (decoded.payloadLength != 3U) {
        sendV2Result(decoded.sequence, decoded.kind, 1U);
        return 1U;
    }
    const uint8_t command = decoded.payload[0];
    const uint16_t argument = getU16Le(decoded.payload + 1U);
    uint8_t status = 0U;
    if ((command <= 4U || command == 7U) && argument != 0U) status = 1U;
    else if (command == 0U) SerialCommand = REBOOT;
    else if (command == 1U) status = elementicSaveConfiguration() ? 0U : 5U;
    else if (command == 2U) {
        elementicLoadFactoryDefaults();
        status = elementicLoadConfiguration() ? 0U : 5U;
        ++configurationGeneration;
    } else if (command == 3U) { status = elementicFactoryReset() ? 0U : 5U; ++configurationGeneration; }
    else if (command == 4U) { /* no stored error latch yet */ }
    else if (command == 5U && argument >= 1U && argument <= OutputCounter) DeleteOutput = argument;
    else if (command == 6U && argument >= 1U && argument <= static_cast<uint16_t>(SwitchCounter)) DeleteInput = argument;
    else if (command == 7U) elementicEnterMacReprovisioning();
    else status = 1U;
    sendV2Result(decoded.sequence, decoded.kind, status);
    return status;
}

void processV2Frame(const uint8_t* frame, size_t frameLength) {
    uint8_t body[elementic::V2_MAX_BODY];
    elementic::V2FrameView decoded = {};
    if (elementic::decodeV2Frame(frame, frameLength, body, sizeof(body), &decoded) != elementic::V2_CODEC_OK ||
        decoded.sequence == 0U) return;
    if (decoded.kind == 0x01U && decoded.payloadLength == 0U) {
        sendV2Info(decoded.sequence);
    } else if (decoded.kind == 0x03U && decoded.payloadLength == 1U) {
        sendV2Snapshot(decoded.sequence, decoded.payload[0]);
    } else if (decoded.kind == 0x10U || decoded.kind == 0x11U) {
        const uint32_t requestHash = elementic::crc32(frame, frameLength);
        if (decoded.sequence == lastMutationSequence) {
            if (requestHash == lastMutationHash && decoded.kind == lastMutationKind) {
                sendV2Result(decoded.sequence, decoded.kind, lastMutationStatus);
            } else sendV2Result(decoded.sequence, decoded.kind, 7U);
            return;
        }
        uint8_t status = 0U;
        if (decoded.kind == 0x10U) {
            V2Write write = {};
            status = parseV2Write(decoded, write) ? applyV2Write(write) : 1U;
            sendV2Result(decoded.sequence, decoded.kind, status);
        } else {
            status = processV2Command(decoded);
        }
        lastMutationSequence = decoded.sequence;
        lastMutationHash = requestHash;
        lastMutationKind = decoded.kind;
        lastMutationStatus = status;
    } else {
        sendV2Result(decoded.sequence, decoded.kind, 2U);
    }
}

bool containsLegacyMarker(const uint8_t* value, uint16_t length) {
    static const char* markers[] = {"S01", "S02", "E01", "E02", "REQ01"};
    for (uint8_t marker = 0; marker < 5U; ++marker) {
        const size_t markerLength = strlen(markers[marker]);
        if (length < markerLength) continue;
        for (uint16_t i = 0; i <= length - markerLength; ++i) {
            if (memcmp(value + i, markers[marker], markerLength) == 0) return true;
        }
    }
    return false;
}

bool validateV1Command(const uint8_t* frame, size_t length) {
    if (frame == 0 || length < 9U || memcmp(frame, "S01", 3U) != 0 ||
        memcmp(frame + length - 3U, "E01", 3U) != 0) return false;
    size_t position = 3U;
    const size_t end = length - 3U;
    while (position < end) {
        if (position + 7U > end || memcmp(frame + position, "S02", 3U) != 0) return false;
        position += 3U;
        const uint8_t category = frame[position++];
        const uint8_t field = frame[position++];
        const uint8_t item = frame[position++];
        const uint8_t declared = frame[position++];
        uint16_t width = declared;
        bool text = false;
        if (category == 0U) {
            if (item == 0U || item > OutputChannels || field > 8U) return false;
            if (field == 6U) { width = declared; text = true; if (width >= MAXNAMELENGTH) return false; }
            else if (field == 7U) { width = declared; text = true; if (width >= MAXTOPICLENGTH) return false; }
            else width = 1U;
        } else if (category == 1U) {
            if (item == 0U || item > InputChannels ||
                !(field == 0U || field == 1U || field == 2U || field == 5U)) return false;
            if (field == 0U) width = 1U;
            else if (field == 1U) { width = declared; if (width > INOUT_MATRIX_COLS) return false; }
            else { width = declared; text = true; if ((field == 2U && width >= MAXTOPICLENGTH) || (field == 5U && width >= MAXNAMELENGTH)) return false; }
        } else if (category == 2U) {
            if (field != 2U) return false;
            const int index = findVariableIndexByPriority(item);
            if (index < 0 || index >= totalVariables) return false;
            switch (variables[index].type) {
                case DYNAMIC_STRING:
                case DYNAMIC_PASSWORD: width = declared; text = true; if (width > 29U || width >= MAX_STRING_LENGTH) return false; break;
                case DYNAMIC_BYTE:
                case DYNAMIC_BOOL: width = 1U; break;
                case DYNAMIC_INT: width = 2U; break;
                case DYNAMIC_IP: width = 4U; break;
            }
            if (declared != 0U && declared != width) return false;
        } else if (category == 3U) {
            if (item != 0U || !(field == 6U || field == 14U || field == 15U)) return false;
            width = 2U;
        } else return false;
        if (position + width + 3U > end || memcmp(frame + position + width, "E02", 3U) != 0) return false;
        if (text && containsLegacyMarker(frame + position, width)) return false;
        position += width + 3U;
    }
    return position == end;
}

}  // namespace

template <typename Func>
void ProcessVariablesByPriority(Func func) {
    for (int id = 0; id < totalVariables; id++) {
        func(id, variables[id]);
    }
}

void serialsetup() {
    Serial.begin(115200);
    // Serial.println("Serial port initiated. . .");
    databuffersize = 255; // kan niet hoger zijn dan 255
    sendProtocolAnnouncement();
}

void serialwrite(byte i) {
    if (databufferposition >= databuffersize) {
        delay(1);
        databufferposition = 0;
    }
    Serial.write(i);
    databufferposition++;
}

void serialPrintLimited(const char* data) {
    size_t dataLen = strlen(data);
    if ((databufferposition + dataLen) >= databuffersize) {
        delay(1);
        databufferposition = 0;
    }
    Serial.print(data);
    databufferposition += dataLen;
}

int DynamicVariableActions(int EP, bool ER, bool SD, bool MEM, DynamicVariable &var,int varId) {
    const byte serialId = (byte)var.priority;   // app protocol id
    const byte storageId = (byte)varId;         // EEPROM/internal storage index

    // Write variable type block ("Type")
    if (SD) { serialPrintLimited("S02"); }
    if (SD) { serialwrite(2); }
    if (SD) { serialwrite(0); }
    if (SD) { serialwrite(serialId); }
    if (SD) { serialwrite(1); }
    if (SD) { serialwrite((byte)var.type); }
    if (ER) {
        EEPROM.write(EP++, (byte)2);
        EEPROM.write(EP++, (byte)0);
        EEPROM.write(EP++, storageId);
        EEPROM.write(EP++, (byte)1);
        EEPROM.write(EP++, (byte)var.type);
    }
    if (SD) { serialPrintLimited("E02"); }

    // Write variable Name block
    size_t nameLen = strlen(var.name.c_str());
    if (SD) { serialPrintLimited("S02"); }
    if (SD) { serialwrite(2); }
    if (SD) { serialwrite(1); }
    if (SD) { serialwrite(serialId); }
    if (SD) { serialwrite((byte)nameLen); }
    if (SD) { Serial.write(var.name.c_str(), nameLen); }
    if (ER) {
        EEPROM.write(EP++, (byte)2);
        EEPROM.write(EP++, (byte)1);
        EEPROM.write(EP++, storageId);
        EEPROM.write(EP++, (byte)nameLen);
        for (size_t i2 = 0; i2 < nameLen; i2++) {
            EEPROM.write(EP++, (byte)var.name.c_str()[i2]);
        }
    }
    if (SD) { serialPrintLimited("E02"); }

    // Write variable Value block
    if (SD) { serialPrintLimited("S02"); }
    if (SD) { serialwrite(2); }
    if (SD) { serialwrite(2); }
    if (SD) { serialwrite(serialId); }
    if (ER) {
        EEPROM.write(EP++, (byte)2);
        EEPROM.write(EP++, (byte)2);
        EEPROM.write(EP++, storageId);
    }

    switch (var.type) {
        case DYNAMIC_STRING: {
            size_t strLen = strlen(var.value.strVal);
            if (SD) { serialwrite((byte)strLen); }
            if (SD) { serialPrintLimited(var.value.strVal); }
            if (ER) {
                EEPROM.write(EP++, (byte)strLen);
                for (size_t i2 = 0; i2 < strLen; i2++) {
                    EEPROM.write(EP++, (byte)var.value.strVal[i2]);
                }
            }
            break;
        }
        case DYNAMIC_BYTE:
            if (SD) { serialwrite(1); }
            if (SD) { serialwrite(var.value.byteVal); }
            if (ER) {
                EEPROM.write(EP++, (byte)1);
                EEPROM.write(EP++, (byte)var.value.byteVal);
            }
            break;
        case DYNAMIC_INT:
            if (SD) { serialwrite(2); }
            if (SD) {
                serialwrite((byte)(var.value.intVal >> 8));
                serialwrite((byte)(var.value.intVal & 0xFF));
            }
            if (ER) {
                EEPROM.write(EP++, (byte)2);
                EEPROM.write(EP++, (byte)(var.value.intVal >> 8));
                EEPROM.write(EP++, (byte)(var.value.intVal & 0xFF));
            }
            break;
        case DYNAMIC_IP:
            if (SD) { serialwrite(4); }
            for (int i = 0; i < 4; i++) {
                if (SD) { serialwrite(var.value.ipVal[i]); }
            }
            if (ER) {
                EEPROM.write(EP++, (byte)4);
                for (int i = 0; i < 4; i++) {
                    EEPROM.write(EP++, (byte)var.value.ipVal[i]);
                }
            }
            break;
        case DYNAMIC_PASSWORD: {
            size_t passLen = strlen(var.value.passVal);
            if (SD) { serialwrite((byte)passLen); }
            if (SD) { serialPrintLimited(var.value.passVal); }
            if (ER) {
                EEPROM.write(EP++, (byte)passLen);
                for (size_t i2 = 0; i2 < passLen; i2++) {
                    EEPROM.write(EP++, (byte)var.value.passVal[i2]);
                }
            }
            break;
        }
        case DYNAMIC_BOOL:
            if (SD) { serialwrite(1); }
            if (SD) { serialwrite(var.value.byteVal ? 1 : 0); }
            if (ER) {
                EEPROM.write(EP++, (byte)1);
                EEPROM.write(EP++, (byte)(var.value.byteVal ? 1 : 0));
            }
            break;
    }
    if (SD) { serialPrintLimited("E02"); }

    return EP;
}

void SendAllData(bool ER, bool SD, bool MEM) { // ER = store to EEPROM, SD = send via Serial, MEM = reserved for MEM
    if (SD) { snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/LOG"); }
    if (ER) { snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/LOG"); }
    int EP = 0; // EEPROM pointer

    if (SD) { serialPrintLimited("S01"); } // Initiate frame
    if (SD) { serialPrintLimited("1"); }    if (ER) { EEPROM.write(EP++, (byte)1); } // Type
    // Wire v1 is numeric 0x01. EEPROM schema 1 remains numeric byte 1 as it
    // always was; correcting the wire byte must not rewrite storage.
    if (SD) { serialwrite(elementic::WIRE_V1); } if (ER) { EEPROM.write(EP++, (byte)1); } // Version

    // TYPE01:3, TYPE02: 10: Number of Outputs, 11: Number of maximum Outputs, 12: Number of Inputs, 13: Number of Maximum Inputs
    // Maximal number of inputs
    if (SD) { serialPrintLimited("S02"); }
    if (SD) { serialwrite(3); }   if (ER) { EEPROM.write(EP++, (byte)3); }  // Type1: input
    if (SD) { serialwrite(13); }  if (ER) { EEPROM.write(EP++, (byte)13); }  // Type2: channel
    if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); } // id = 0 !!!!!!!!!!!!!!!!!!! id = type!!!!
    if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); } // length = 0
    if (SD) { serialwrite(InputChannels); }   if (ER) { EEPROM.write(EP++, (byte)InputChannels); }  // Low byte value
    if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }  // High byte value, considering 255 is the max
    if (SD) { serialPrintLimited("E02"); }

    // Maximal number of outputs
    if (SD) { serialPrintLimited("S02"); }
    if (SD) { serialwrite(3); }   if (ER) { EEPROM.write(EP++, (byte)3); }  // Type1: input
    if (SD) { serialwrite(11); }  if (ER) { EEPROM.write(EP++, (byte)11); }  // Type2: channel
    if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); } // id = 0
    if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); } // length = 0
    if (SD) { serialwrite(OutputChannels); }   if (ER) { EEPROM.write(EP++, (byte)OutputChannels); }  // Low byte value
    if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }  // High byte value, considering 255 is the max
    if (SD) { serialPrintLimited("E02"); }

    // Current number of inputs
    if (SD) { serialPrintLimited("S02"); }
    if (SD) { serialwrite(3); }   if (ER) { EEPROM.write(EP++, (byte)3); }  // Type1: input
    if (SD) { serialwrite(12); }  if (ER) { EEPROM.write(EP++, (byte)12); }  // Type2: channel
    if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); } // id = 0
    if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); } // length = 0
    if (SD) { serialwrite(SwitchCounter); }   if (ER) { EEPROM.write(EP++, (byte)SwitchCounter); }  // Low byte value
    if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }  // High byte value, considering 255 is the max
    if (SD) { serialPrintLimited("E02"); }

    // Current number of outputs
    if (SD) { serialPrintLimited("S02"); }
    if (SD) { serialwrite(3); }   if (ER) { EEPROM.write(EP++, (byte)3); }  // Type1: input
    if (SD) { serialwrite(10); }  if (ER) { EEPROM.write(EP++, (byte)10); }  // Type2: channel
    if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); } // id = 0
    if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); } // length = 0
    if (SD) { serialwrite(OutputCounter); }   if (ER) { EEPROM.write(EP++, (byte)OutputCounter); }  // Low byte value
    if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }  // High byte value, considering 255 is the max
    if (SD) { serialPrintLimited("E02"); }

    // if (SD){ // 01-12-2025 turn this back to above code, this should be in EEPROM, otherwise deleting lines is not possible
    //     SendSystemInBatch(NUMINPUTS, (int)SwitchCounter);
    //     SendSystemInBatch(MAXINPUTS, (int)InputChannels);
    //     SendSystemInBatch(NUMOUTPUTS, (int)OutputCounter);
    //     SendSystemInBatch(MAXOUTPUTS, (int)OutputChannels);
    // }
    SendSystemInBatch(DEVICETYPE, ELEMENTIC_DEVICEID);
    SendSystemInBatch(MAXCHARNAME, (int)MAXNAMELENGTH);
    SendSystemInBatch(MAXCHARTOPIC, (int)MAXTOPICLENGTH);

    // Process inputs
    for (byte i = 1; i <= SwitchCounter; i++) {
        // Write channel for input
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(1); }   if (ER) { EEPROM.write(EP++, (byte)1); }  // Type1: input
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }  // Type2: channel
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }  // id
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }  // length not required
        if (SD) { serialwrite(SwitchPin[i]); }   if (ER) { EEPROM.write(EP++, (byte)SwitchPin[i]); }  // value
        if (SD) { serialPrintLimited("E02"); }

        // Write outputs for input block
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(1); }   if (ER) { EEPROM.write(EP++, (byte)1); }  // Type1: input
        if (SD) { serialwrite(1); }   if (ER) { EEPROM.write(EP++, (byte)1); }  // Type2: Outputs
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }  // id

        // For Serial: write the length of the comma-separated string.
        if (SD) {
            //serialwrite((byte)strlen(InputOutputsString[i])); // 30-04-2025 -- convert to bytes!!!!!!! no CSV!!!! both android as here -- remain line for future JSON implementation
            serialwrite((byte)InOutMatrixCouter[i]);
        }
        // For EEPROM: write the number of outputs.
        if (ER) {
            EEPROM.write(EP++, (byte)InOutMatrixCouter[i]);
        }

        // Serial: write each character of the string.
        if (SD) {
            for (byte k = 0; k < InOutMatrixCouter[i]; k++) {
                serialwrite((byte) InOutMatrix[i][k]);        // value
            }
        }

        // EEPROM: write each output value.
        if (ER) {
            for (byte i2 = 0; i2 < InOutMatrixCouter[i]; i2++) {
                EEPROM.write(EP++, (byte)InOutMatrix[i][i2]);
            }
        }
        if (SD) { serialPrintLimited("E02"); }

        // Write Topic for input
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(1); }   if (ER) { EEPROM.write(EP++, (byte)1); }  // Type1: input
        if (SD) { serialwrite(2); }   if (ER) { EEPROM.write(EP++, (byte)2); }  // Type2: Topic
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }  // id
        size_t mqttInputLen = strlen(MQTT_Input[i]);
        if (SD) { serialwrite((byte)mqttInputLen); }   if (ER) { EEPROM.write(EP++, (byte)mqttInputLen); }
        if (SD) { serialPrintLimited(MQTT_Input[i]); }
        if (ER) {
            for (int i2 = 0; i2 < (int)mqttInputLen; i2++) {
                EEPROM.write(EP++, MQTT_Input[i][i2]);
            }
        }
        if (SD) { serialPrintLimited("E02"); }

        // Write Name for input
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(1); }   if (ER) { EEPROM.write(EP++, (byte)1); }  // Type1: input
        if (SD) { serialwrite(5); }   if (ER) { EEPROM.write(EP++, (byte)5); }  // Type2: Name
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }  // id
        size_t Input_NameLen = strlen(Input_Name[i]);
        if (SD) { serialwrite((byte)Input_NameLen); }   if (ER) { EEPROM.write(EP++, (byte)Input_NameLen); }
        if (SD) { serialPrintLimited(Input_Name[i]); }
        if (ER) {
            for (int i2 = 0; i2 < (int)Input_NameLen; i2++) {
                EEPROM.write(EP++, Input_Name[i][i2]);
            }
        }

        if (SD) { serialPrintLimited("E02"); }
    }

    // Process outputs
    for (byte i = 1; i <= OutputCounter; i++) {
        // Pin
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputPin[i]); }   if (ER) { EEPROM.write(EP++, (byte)OutputPin[i]); } // 10-11-2025
        if (SD) { serialPrintLimited("E02"); }

        // Type
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(1); }   if (ER) { EEPROM.write(EP++, (byte)1); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputType[i]); } if (ER) { EEPROM.write(EP++, (byte)OutputType[i]); }
        if (SD) { serialPrintLimited("E02"); }

        // Relay channel
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(2); }   if (ER) { EEPROM.write(EP++, (byte)2); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputRelay[i]); } if (ER) { EEPROM.write(EP++, (byte)OutputRelay[i]); }
        if (SD) { serialPrintLimited("E02"); }

        // Minvalue
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(3); }   if (ER) { EEPROM.write(EP++, (byte)3); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputMinValue[i]); } if (ER) { EEPROM.write(EP++, (byte)OutputMinValue[i]); }
        if (SD) { serialPrintLimited("E02"); }

        // Maxvalue
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(4); }   if (ER) { EEPROM.write(EP++, (byte)4); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputMaxValue[i]); } if (ER) { EEPROM.write(EP++, (byte)OutputMaxValue[i]); }
        if (SD) { serialPrintLimited("E02"); }

        // Default value
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(5); }   if (ER) { EEPROM.write(EP++, (byte)5); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputDefaultValue[i]); } if (ER) { EEPROM.write(EP++, (byte)OutputDefaultValue[i]); }
        if (SD) { serialPrintLimited("E02"); }

        // Name
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(6); }   if (ER) { EEPROM.write(EP++, (byte)6); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }

        size_t tempLen = strlen(Output_Name[i]);

        if (SD) { serialwrite((byte)tempLen); }
        if (ER) { EEPROM.write(EP++, (byte)tempLen); }
        if (SD) { serialPrintLimited(Output_Name[i]); }
        if (ER) {
            for (int i2 = 0; i2 < (int)tempLen; i2++) {
                EEPROM.write(EP++, Output_Name[i][i2]);
            }
        }
        if (SD) { serialPrintLimited("E02"); }

        // Topic
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(7); }   if (ER) { EEPROM.write(EP++, (byte)7); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        size_t mqttOutputLen = strlen(MQTT_Output[i]);
        if (SD) { serialwrite((byte)mqttOutputLen); }   if (ER) { EEPROM.write(EP++, (byte)mqttOutputLen); }
        if (SD) { serialPrintLimited(MQTT_Output[i]); }
        if (ER) {
            for (int i2 = 0; i2 < (int)mqttOutputLen; i2++) {
                EEPROM.write(EP++, MQTT_Output[i][i2]);
            }
        }
        if (SD) { serialPrintLimited("E02"); }

        // Value
        if (SD) { serialPrintLimited("S02"); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite(8); }   if (ER) { EEPROM.write(EP++, (byte)8); }
        if (SD) { serialwrite(i); }   if (ER) { EEPROM.write(EP++, (byte)i); }
        if (SD) { serialwrite(0); }   if (ER) { EEPROM.write(EP++, (byte)0); }
        if (SD) { serialwrite((byte)OutputValueActual[i]); } if (ER) { EEPROM.write(EP++, (byte)OutputValueActual[i]); }
        if (SD) { serialPrintLimited("E02"); }

        //if (ER) { EEPROM.write(EP, 0xFF); } // End marker for EEPROM block
    }

    // Process dynamic variables by priority
    ProcessVariablesByPriority([&EP, ER, SD, MEM](int id, DynamicVariable &var) {
        // id is now available here alongside var
        EP = DynamicVariableActions(EP, ER, SD, MEM, var, id);
    });

    if (ER) {
        EEPROM.write(EP++, 0xFF); // Write a final end marker
        EEPROM.write(EP++, 0xFF); // Write a final end marker
        EEPROM.write(EP++, 0xFF); // Write a final end marker
        #if defined(ESP32) || defined(ESP8266)
            EEPROM.commit();       // Only needed on ESP platforms
        #endif
    }

    // End frame marker

    if (SD) { serialPrintLimited("E01"); }
}

void clearIncomingData() {
    memset(incomingdata, 0, sizeof(incomingdata));
    data_index = 0;
    Serial.flush();
}

void ReadAllDataFromEEPROM() {
    int EP = 0; // EEPROM pointer
    byte type, type01, type02, id, length, version;

    snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/LOG");
    // mqttClient.publish(msgmqtt, "Reading data from EEPROM...");
    logging(LOG_INFO,"Reading data from EEPROM. . .");

    // Read type and version
    type = EEPROM.read(EP++);
    version = EEPROM.read(EP++);

    char debugBuf[100];
    sprintf(debugBuf, "Type: %d, Version: %d", type, version);
    // mqttClient.publish(msgmqtt, debugBuf);

    if (type != 1 || version != 1) {
        // mqttClient.publish(msgmqtt, "Invalid EEPROM data! Aborting.");
        return;
    }

    // Loop with a safeguard against overrun
    while (EP < EEPROM.length() && EEPROM.read(EP) != 0xFF) {
        //sprintf(debugBuf, "Processing block at EP = %d", EP);
        //mqttClient.publish(msgmqtt, debugBuf);

        type01 = EEPROM.read(EP++);
        type02 = EEPROM.read(EP++);
        id     = EEPROM.read(EP++);
        length = EEPROM.read(EP++);

        //sprintf(debugBuf, "Block header - type01: %d, type02: %d, id: %d, length: %d",
        //        type01, type02, id, length);
        //mqttClient.publish(msgmqtt, debugBuf);

        byte i2 = 0;
        switch (type01) {
            case 0: // Output
                if (id == 0 || id > OutputChannels) {
                    EP += length;
                    snprintf(debugBuf, sizeof(debugBuf),
                             "EEPROM output id %u out of range; skipped", id);
                    logging(LOG_CAUTION, debugBuf);
                    break;
                }
                if (OutputCounter < id) { OutputCounter = id; } // removed 27-11-2025 since id is stored as well
                switch (type02) {
                    case 0: { // channel
                        byte val = EEPROM.read(EP++);
                        OutputPin[id] = val;
                        // sprintf(debugBuf, "OutputChannel[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 1: { // Type
                        byte val = EEPROM.read(EP++);
                        OutputType[id] = val;
                        // sprintf(debugBuf, "OutputType[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 2: { // Relay channel
                        byte val = EEPROM.read(EP++);
                        OutputRelay[id] = val;
                        // sprintf(debugBuf, "OutputRelay[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 3: { // minValue
                        byte val = EEPROM.read(EP++);
                        OutputMinValue[id] = val;
                        // sprintf(debugBuf, "OutputMinValue[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 4: { // maxValue
                        byte val = EEPROM.read(EP++);
                        OutputMaxValue[id] = val;
                        // sprintf(debugBuf, "OutputMaxValue[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    case 5: { // DefaultValue
                        byte val = EEPROM.read(EP++);
                        OutputDefaultValue[id] = val;
                        // sprintf(debugBuf, "OutputDefaultValue[%d] = %d", id, val);
                        //mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }

                    case 6: { // Name 14-12-2025
                        const uint16_t dstCap = sizeof(Output_Name[id]);     // destination capacity
                        uint16_t dstPos = 0;

                        // Read exactly "length" bytes from EEPROM, but store only what fits
                        for (uint16_t k = 0; k < (uint16_t)length; k++) {
                            char c = (char)EEPROM.read(EP++);

                            if (dstPos < dstCap - 1) {   // keep room for '\0'
                                Output_Name[id][dstPos++] = c;
                            }
                            // else: discard extra bytes, but still advance EP (done above)
                        }

                        Output_Name[id][dstPos] = '\0';
                        break;
                    }

                    case 7: { // Topic 14-12-2025
                        const uint16_t dstCap = sizeof(MQTT_Output[id]);
                        uint16_t dstPos = 0;

                        for (uint16_t k = 0; k < (uint16_t)length; k++) {
                            char c = (char)EEPROM.read(EP++);

                            if (dstPos < dstCap - 1) {
                                MQTT_Output[id][dstPos++] = c;
                            }
                        }

                        MQTT_Output[id][dstPos] = '\0';
                        break;
                    }

                    case 8: { // Value
                        byte val = EEPROM.read(EP++);
                        OutputValueActual[id] = val;
                        // sprintf(debugBuf, "Output ValueActual[%d] = %d", id, val);
                        // mqttClient.publish(msgmqtt, debugBuf);
                        break;
                    }
                    default:
                        EP += length;
                        sprintf(debugBuf, "Unknown output block. Skipped %d bytes.", length);
                        logging(LOG_CAUTION,debugBuf);

                        break;
                }
                break;
            case 1: // Input
                if (id == 0 || id > InputChannels) {
                    EP += length;
                    snprintf(debugBuf, sizeof(debugBuf),
                             "EEPROM input id %u out of range; skipped", id);
                    logging(LOG_CAUTION, debugBuf);
                    break;
                }
                //if (SwitchCounter < id) { SwitchCounter = id; } // removed 27-11-2025 since id is stored as well
                switch (type02) {
                    case 0: { // channel/pin
                        byte val = EEPROM.read(EP++);
                        SwitchPin[id] = val;
                        sprintf(debugBuf, "SwitchPin[%d] = %d", id, val);
                        logging(LOG_INFO,debugBuf);
                        break;
                    }
                    case 1: { // Outputs (multiple bytes)
                        InOutMatrixCouter[id] = length;
                        char matrixBuf[50] = "";
                        for (i2 = 0; i2 < length && i2 < sizeof(matrixBuf)-1; i2++) {
                            InOutMatrix[id][i2] = (byte) EEPROM.read(EP++);
                            char temp[5];
                            sprintf(temp, "%d ", InOutMatrix[id][i2]);
                            strcat(matrixBuf, temp);
                        }
                        sprintf(debugBuf, "InOutMatrix[%d] = %s", id, matrixBuf);
                        logging(LOG_INFO,debugBuf);;
                        break;
                    }
                    case 2: { // Topic
                        char inputTopicBuf[50];
                        memset(inputTopicBuf, 0, sizeof(inputTopicBuf));
                        for (i2 = 0; i2 < length && i2 < sizeof(inputTopicBuf)-1; i2++) {
                            inputTopicBuf[i2] = (char)EEPROM.read(EP++);
                        }
                        inputTopicBuf[i2] = '\0';
                        strcpy(MQTT_Input[id], inputTopicBuf);
                        sprintf(debugBuf, "Input Topic[%d] = %s", id, inputTopicBuf);
                        logging(LOG_INFO,debugBuf);
                        break;
                    }

                    case 5: { // Name
                        char inputTopicBuf[50];
                        memset(inputTopicBuf, 0, sizeof(inputTopicBuf));
                        for (i2 = 0; i2 < length && i2 < sizeof(inputTopicBuf)-1; i2++) {
                            inputTopicBuf[i2] = (char)EEPROM.read(EP++);
                        }
                        inputTopicBuf[i2] = '\0';
                        strcpy(Input_Name[id], inputTopicBuf);
                        sprintf(debugBuf, "Input Topic[%d] = %s", id, inputTopicBuf);
                        logging(LOG_INFO,debugBuf);
                        break;
                    }

                    default:
                        EP += length;
                        sprintf(debugBuf, "Unknown input block. Skipped %d bytes.", length);
                        logging(LOG_CAUTION,debugBuf);
                        break;
                }
                break;
            case 2: { // Generic variables
                if (id >= totalVariables) { EP += length; break; } // storage index
                sprintf(debugBuf, "EEPROM READ Case 2(step 1), type: [%d]",variables[id].type);
                mqttClient.publish(msgmqtt, debugBuf);
                switch (type02) {
                    case 0: {
                        byte val = EEPROM.read(EP++);
                        variables[id].type = (VariableType)val;
                        sprintf(debugBuf, "Variable[%d] Type = %d", id, val);
                        logging(LOG_CAUTION,debugBuf);
                        break;
                    }
                    case 1: {
                        char varName[50];
                        memset(varName, 0, sizeof(varName));
                        if (length >= MAX_STRING_LENGTH) length = MAX_STRING_LENGTH - 1;
                        for (i2 = 0; i2 < length && i2 < sizeof(varName)-1; i2++) {
                            varName[i2] = (char)EEPROM.read(EP++);
                        }
                        varName[i2] = '\0';
                        variables[id].name = varName;
                        sprintf(debugBuf, "Variable[%d] Name = %s", id, varName);
                        logging(LOG_INFO,debugBuf);
                        break;
                    }
                    case 2: {
                        sprintf(debugBuf, "EEPROM READ Case 2, type: [%d]",variables[id].type);
                        logging(LOG_INFO,debugBuf);
                        switch (variables[id].type) {
                            case DYNAMIC_STRING: {
                                char strVal[50];
                                memset(strVal, 0, sizeof(strVal));
                                if (length >= MAX_STRING_LENGTH) length = MAX_STRING_LENGTH - 1;
                                for (i2 = 0; i2 < length && i2 < sizeof(strVal)-1; i2++) {
                                    strVal[i2] = (char)EEPROM.read(EP++);
                                }
                                strVal[i2] = '\0';
                                strcpy(variables[id].value.strVal, strVal);
                                sprintf(debugBuf, "Variable[%d] Value (String) = %s", id, strVal);
                                logging(LOG_INFO,debugBuf);
                                break;
                            }
                            case DYNAMIC_BYTE: {
                                byte bVal = EEPROM.read(EP++);
                                variables[id].value.byteVal = bVal;
                                sprintf(debugBuf, "Variable[%d] Value (Byte) = %d", id, bVal);
                                logging(LOG_INFO,debugBuf);
                                break;
                            }
                            case DYNAMIC_INT: {
                                const uint8_t highByte = EEPROM.read(EP++);
                                const uint8_t lowByte = EEPROM.read(EP++);
                                int intVal = (static_cast<int>(highByte) << 8) | lowByte;
                                variables[id].value.intVal = intVal;
                                sprintf(debugBuf, "Variable[%d] Value (Int) = %d", id, intVal);
                                logging(LOG_INFO,debugBuf);
                                break;
                            }
                            case DYNAMIC_IP: {
                                byte ipBytes[4];
                                for (i2 = 0; i2 < 4; i2++) {
                                    ipBytes[i2] = EEPROM.read(EP++);
                                }
                                snprintf(debugBuf, sizeof(debugBuf),
                                        "Variable[%d] Value (IP) = %d.%d.%d.%d",
                                        id,
                                        ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]);
                                logging(LOG_INFO,debugBuf);

                                // store
                                memcpy(variables[id].value.ipVal, ipBytes, 4);
                                break;
                            }
                            case DYNAMIC_PASSWORD: {
                                char passBuf[50];
                                memset(passBuf, 0, sizeof(passBuf));
                                if (length >= MAX_STRING_LENGTH) length = MAX_STRING_LENGTH - 1;
                                for (i2 = 0; i2 < length && i2 < sizeof(passBuf)-1; i2++) {
                                    passBuf[i2] = (char)EEPROM.read(EP++);
                                }
                                passBuf[i2] = '\0';
                                strcpy(variables[id].value.passVal, passBuf);
                                sprintf(debugBuf, "Variable[%d] Value (Password) updated", id);
                                logging(LOG_INFO,debugBuf);
                                break;
                            }
                            case DYNAMIC_BOOL: {
                                byte boolVal = EEPROM.read(EP++);
                                variables[id].value.byteVal = (boolVal != 0) ? 1 : 0;
                                sprintf(debugBuf, "Variable[%d] Value (Bool) = %d", id, variables[id].value.byteVal);
                                logging(LOG_INFO,debugBuf);
                                break;
                            }
                        }
                        break;
                    }
                    default:
                        EP += length;
                        sprintf(debugBuf, "Unknown generic variable block. Skipped %d bytes.", length);
                        logging(LOG_CAUTION,debugBuf);
                        break;
                }
                break;
            }
            case 3: // System (number of inputs and outputs stored in EEPROM)
            {
                // Released storage schema 1 has a four-byte header followed by
                // a two-byte little-endian value. The old comment that called
                // this a four-byte record was incorrect; tagged v1.0.0 and
                // main both write and consume all six bytes.
                if (EP + 2 > EEPROM.length()) {
                    logging(LOG_WARNING, "Truncated EEPROM system record");
                    return;
                }
                const int val = static_cast<int>(EEPROM.read(EP)) |
                                (static_cast<int>(EEPROM.read(EP + 1)) << 8);
                EP += 2;

                switch (type02) {
                    case 10: { // Number of Outputs (current)
                        OutputCounter = min(val, static_cast<int>(OutputChannels));
                        break;
                    }
                    case 11: {
                        // Max number of Outputs stored in 'id' (and optional high in 'length')
                        // If you later add a MaxOutputs variable, set it here.
                        break;
                    }
                    case 12: { // Number of Inputs (current)
                        SwitchCounter = min(val, static_cast<int>(InputChannels));
                        break;
                    }
                    case 13: {
                        // Max number of Inputs stored in 'id'
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
                EP += length;
                sprintf(debugBuf, "Unknown block type. Skipped %d bytes.", length);
                logging(LOG_CAUTION,debugBuf);
                break;
        }
    }
    // sprintf(debugBuf, "Final EP: %d", EP);
    // logging(LOG_INFO,debugBuf);
    EEPROMStorage = EP;
    logging(LOG_INFO,"End Reading data from EEPROM. . .");
}

void SendOutputValueSerial(byte id, byte value) {
    Serial.print("S01"); // Initiate
    Serial.print("S02"); // Marker
    serialwrite(0); // Type01: 0 = Output
    serialwrite(8); // type02: 8 = Value
    serialwrite(id); // id not required
    serialwrite(0); // length not required
    // Write 2-byte value
    serialwrite((byte)value);   // low byte first
    Serial.print("E02");
    Serial.print("E01");
}

void SendSystem(SystemType systemTypeVal, int value) {
    Serial.print("S01"); // Initiate
    Serial.print("S02"); // Marker
    serialwrite(3); // Type01: 3 = System
    serialwrite((byte)systemTypeVal); // type02: VARIABLE
    serialwrite(0); // id not required
    serialwrite(0); // length not required
    // Write 2-byte value
    serialwrite((byte)(value & 0xFF));   // low byte first
    serialwrite((byte)(value >> 8));     // high byte second
    Serial.print("E02");
    Serial.print("E01");
}

void SendStatus(byte type1, byte protocolId, byte statusValue) {
    byte protocolType01 = 0;
    byte protocolType02 = 0;

    switch (type1) {
        case VARIABLE_OUTPUT:
            protocolType01 = 0;
            protocolType02 = 9;
            break;

        case VARIABLE_INPUT:
            protocolType01 = 1;
            protocolType02 = 6;
            break;

        case VARIABLE_DYNAMIC:
            protocolType01 = 2;
            protocolType02 = 5;
            break;

        default:
            return;
    }

    Serial.print("S01");
    Serial.print("S02");
    serialwrite(protocolType01);
    serialwrite(protocolType02);
    serialwrite(protocolId);
    serialwrite((byte)1);
    serialwrite(statusValue);
    Serial.print("E02");
    Serial.print("E01");
}

void SendSystemInBatch(SystemType systemTypeVal, int value) {
    // Serial.print("S01"); // Initiate
    Serial.print("S02"); // Marker
    serialwrite(3); // Type01: 3 = System
    serialwrite((byte)systemTypeVal); // type02: VARIABLE
    serialwrite(0); // id not required
    serialwrite(0); // length not required
    // Write 2-byte value
    serialwrite((byte)(value & 0xFF));   // low byte first
    serialwrite((byte)(value >> 8));     // high byte second
    Serial.print("E02");
    // Serial.print("E01");
}

// ---------------- Restored from original library (needed for linking) ----------------
void serialloop() {
    // 1) Read incoming bytes from Serial
    while (Serial.available() > 0) {
        incomingByte = Serial.read();
        uint8_t byteValue = (uint8_t)incomingByte;
        if (data_index < MAX_SERIAL_BUFFER) {
            incomingdata[data_index++] = byteValue;
        } else {

            logging(LOG_CAUTION,"Warning: Serial buffer overflow");
            clearIncomingData();
        }
    }

    // Need at least 5 bytes to check for "REQ01"
    if (data_index < 5) {
        return;
    }

    // 2) Check for "REQ01" in the buffer
    for (int i = 0; i <= data_index - 5; i++) {
        if (incomingdata[i]     == 'R' &&
            incomingdata[i + 1] == 'E' &&
            incomingdata[i + 2] == 'Q' &&
            incomingdata[i + 3] == '0' &&
            incomingdata[i + 4] == '1') {
            sendProtocolAnnouncement();
            #if ELEMENTIC_WIRE_PROTOCOL_VERSION == 1
              SendAllData(false, true, false); // transitional numeric-v1 sync
            #endif
            clearIncomingData();
            return;
        }
    }

    // 3) Look for a complete "S01 ... E01" block.
    int startPos = -1;
    int endPos = -1;
    for (int i = 0; i <= data_index - 3; i++) {
        if (incomingdata[i] == 'S' && incomingdata[i + 1] == '0' && incomingdata[i + 2] == '1') {
            startPos = i;
            break;
        }
    }
    if (startPos == -1) return;
    for (int j = startPos + 3; j <= data_index - 3; j++) {
        if (incomingdata[j] == 'E' && incomingdata[j + 1] == '0' && incomingdata[j + 2] == '1') {
            endPos = j;
            break;
        }
    }
    if (endPos == -1) return;

    const size_t completeFrameLength = static_cast<size_t>(endPos + 3 - startPos);
    if (completeFrameLength >= 8U && incomingdata[startPos + 3] == '1' &&
        incomingdata[startPos + 4] == elementic::WIRE_V2) {
        #if ELEMENTIC_WIRE_PROTOCOL_VERSION == 2
          processV2Frame(incomingdata + startPos, completeFrameLength);
        #endif
        clearIncomingData();
        return;
    }

    #if ELEMENTIC_WIRE_PROTOCOL_VERSION != 1
      clearIncomingData();
      return;
    #endif

    if (!validateV1Command(incomingdata + startPos, completeFrameLength)) {
        logging(LOG_CAUTION, "Rejected malformed serial command");
        clearIncomingData();
        return;
    }

    // 4) Parse all "S02" sub-blocks between S01 and E01
    int counter = startPos + 3;
    while (true) {
        if (counter + 2 > endPos) break;
        if (incomingdata[counter] != 'S' || incomingdata[counter + 1] != '0' || incomingdata[counter + 2] != '2')
            break;
        counter += 3;
        if (counter + 4 + 1 + 3 > endPos + 1) break;
        byte Type01 = incomingdata[counter++];
        byte Type02 = incomingdata[counter++];
        byte id = incomingdata[counter++];
        byte length2 = incomingdata[counter++];

        // snprintf(msgmqtt2, sizeof(msgmqtt2), "Received updated: TYPE01=%d, TYPE02=%d, ID=%d, LENGTH=%d ", Type01, Type02, id,length2);
        // logging(LOG_INFO,msgmqtt2);

        if (counter >= endPos) break;

        // Apply data based on header values:
        byte Value;
        switch (Type01) {
            case 0: // Output
                char suffixBuf[32];

                if (OutputCounter < id){//07-11-2025: Make sure number of outputs is sufficient
                    OutputCounter=id;
                }

                // snprintf(msgmqtt2, sizeof(msgmqtt2), "Received updated for Type 2: TYPE01=%d, TYPE02=%d, ID=%d, LENGTH=%d ", Type01, Type02, id,length2);
                // logging(LOG_INFO,msgmqtt2);
                switch (Type02) {
                    case 0: { //channel/pin
                        Value = incomingdata[counter++];
                        OutputPin[id] = Value;
                        break;
                    }
                    case 1: { //Type
                        Value = incomingdata[counter++];
                        OutputType[id] = Value;
                        break;
                    }
                    case 2: { //Relay
                        Value = incomingdata[counter++];
                        OutputRelay[id] = Value;
                        setrelays();
                        break;
                    }
                    case 3: { // MinValue
                        Value = incomingdata[counter++];
                        OutputMinValue[id] = Value;
                        break;
                    }
                    case 4: { // MaxValue
                        Value = incomingdata[counter++];
                        OutputMaxValue[id] = Value;
                        break;
                    }
                    case 5: { // DefaultValue
                        Value = incomingdata[counter++];
                        OutputDefaultValue[id] = Value;
                        break;
                    }
                    case 6:{ //07-11-2025: Name
                        for(int i2=0;i2<length2;i2++)
                            {
                                Output_Name[id][i2] = (char)incomingdata[counter++];
                                Output_Name[id][i2+1] = '\0';
                            }
                            // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/RECEIVEDDATATYPENAme");
                            snprintf(msgmqtt2, sizeof(msgmqtt2), "Received updated for Type 2: TYPE01=%d, TYPE02=%d, ID=%d, LENGTH=%d, name:%s ", Type01, Type02, id,length2, Output_Name[id]);
                            logging(LOG_INFO,msgmqtt2);
                        break;
                    }
                    case 7: {//07-11-2025: Topic
                        for(int i2=0;i2<length2;i2++)
                            {
                                MQTT_Output[id][i2] = (char)incomingdata[counter++];
                                MQTT_Output[id][i2+1] = '\0';
                            }
                        snprintf(msgmqtt2, sizeof(msgmqtt2), "Received updated for Type 2: TYPE01=%d, TYPE02=%d, ID=%d, LENGTH=%d, name:%s ", Type01, Type02, id,length2, MQTT_Output[id]);
                        logging(LOG_INFO,msgmqtt2);
                        break;
                    }
                    case 8: { // Value
                        Value = incomingdata[counter++];
                        OutputValueActual[id] = Value;
                        SendOutputValueMQTT(id);
                        // strcpy_P(suffixBuf, dimvalue_status); ///////// going wrong when not dimmable!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Make cases for each output type or move to output, but requires a new memory array
                        // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTT_Output[id], suffixBuf);
                        // snprintf(msgmqtt2, 20 ,"%d",Value);
                        // mqttClient.publish(msgmqtt, msgmqtt2);

                        break;
                    }
                    default:{
                        counter++;
                        snprintf(msgmqtt2, sizeof(msgmqtt2), "Unknown received updated for Type2: TYPE01=%d, TYPE02=%d, ID=%d, LENGTH=%d, name:%s ", Type01, Type02, id,length2, MQTT_Output[id]);
                        logging(LOG_CAUTION,msgmqtt2);
                        break;
                        }
                }
                // MQTT post for Output update
                // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/OUTPUT");
                // snprintf(msgmqtt2, sizeof(msgmqtt2), "Output updated: id=%d, Type=%d, Value=%d", id, Type02, Value);
                // logging(LOG_INFO,msgmqtt2);

                break;
            case 1: // Input
                //Value = incomingdata[counter++];
                if (id>SwitchCounter){
                    SwitchCounter=id;
                }
                switch (Type02) {
                    case 0: {
                        SwitchPin[id] = incomingdata[counter++];
                        break;
                        // Extend if needed for additional input subtypes
                    }
                    case 1: // switch matrix
                    {
                        InOutMatrixCouter[id]= length2;
                        for (byte i = 0; i < length2; i++)
                        {
                            InOutMatrix[id][i] = incomingdata[counter++];
                        }
                    break;
                    }
                    case 2: //Topic
                    {
                    for(int i2=0;i2<length2;i2++)
                        {
                            MQTT_Input[id][i2] = (char)incomingdata[counter++];
                            MQTT_Input[id][i2+1] = '\0';
                        }
                    break;
                    }
                    case 3: //Type 0:on/off, 1:temperature
                    {
                        break;
                    }
                    case 4: //Value
                    {
                        uint8_t highByte = incomingdata[counter++];
                        uint8_t lowByte  = incomingdata[counter++];
                        int value = (highByte << 8) | lowByte;
                        if (value==0){
                            SwitchPressedExternal[id]=false;
                        }
                        if (value==1){
                            SwitchPressedExternal[id]=true;
                        }
                        break;
                    }
                    case 5: //Name
                    {
                        for(int i2=0;i2<length2;i2++)
                        {
                            Input_Name[id][i2] = (char)incomingdata[counter++];
                            Input_Name[id][i2+1] = '\0';
                        }
                                    // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/RECEIVEDDATATYPENAme");
                        snprintf(msgmqtt2, sizeof(msgmqtt2), "Received updated for Type 2: TYPE01=%d, TYPE02=%d, ID=%d, LENGTH=%d, name:%s ", Type01, Type02, id,length2, Input_Name[id]);
                        logging(LOG_INFO,msgmqtt2);
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }

                break;
            case 2: // Generic

                switch (Type02) {
                    case 0: break; // should not be changable externally
                    case 1: break; // should not be changable externally
                    case 2: // Generic
                    {
                        int idx = findVariableIndexByPriority(id);
                        if (idx < 0 || idx >= totalVariables) {
                            // skip payload if unknown priority
                            counter += length2;
                            break;
                        }

                        if (variables[idx].type == DYNAMIC_BYTE) {
                            Value = incomingdata[counter++];
                            variables[idx].value.byteVal = Value;
                            snprintf(msgmqtt2, sizeof(msgmqtt2),
                                    "Generic BYTE updated: id=%d, Value=%d",
                                    id, variables[idx].value.byteVal);
                            logging(LOG_INFO, msgmqtt2);
                        }

                        if (variables[idx].type == DYNAMIC_INT) {
                            byte ValueLow = incomingdata[counter++];
                            byte ValueHigh = incomingdata[counter++];
                            variables[idx].value.intVal = ((int)ValueHigh << 8) | ValueLow;
                            snprintf(msgmqtt2, sizeof(msgmqtt2),
                                    "Generic INT updated: id=%d, ValueHigh=%d, ValueLow=%d, Combined=%d",
                                    id, ValueHigh, ValueLow, variables[idx].value.intVal);
                            logging(LOG_INFO, msgmqtt2);
                        }

                        if (variables[idx].type == DYNAMIC_IP) {
                            byte IPAddress[4];
                            IPAddress[0] = incomingdata[counter++];
                            IPAddress[1] = incomingdata[counter++];
                            IPAddress[2] = incomingdata[counter++];
                            IPAddress[3] = incomingdata[counter++];

                            variables[idx].value.ipVal[0] = IPAddress[0];
                            variables[idx].value.ipVal[1] = IPAddress[1];
                            variables[idx].value.ipVal[2] = IPAddress[2];
                            variables[idx].value.ipVal[3] = IPAddress[3];

                            snprintf(msgmqtt2, sizeof(msgmqtt2),
                                    "Generic IP updated: id=%d, IP=%d.%d.%d.%d",
                                    id, IPAddress[0], IPAddress[1], IPAddress[2], IPAddress[3]);
                            logging(LOG_INFO, msgmqtt2);
                        }

                        if (variables[idx].type == DYNAMIC_STRING || variables[idx].type == DYNAMIC_PASSWORD) {
                            char ReceivedString[30];
                            memset(ReceivedString, 0, sizeof(ReceivedString));

                            byte i2;
                            for (i2 = 0; i2 < length2 && i2 < sizeof(ReceivedString) - 1; i2++) {
                                ReceivedString[i2] = (char)incomingdata[counter++];
                            }
                            ReceivedString[i2] = '\0';

                            if (variables[idx].type == DYNAMIC_STRING) {
                                strcpy(variables[idx].value.strVal, ReceivedString);
                                if (variables[idx].name == "Mac Address") {
                                    elementicApplySharedMac(false);
                                }
                            }
                            if (variables[idx].type == DYNAMIC_PASSWORD) {
                                strcpy(variables[idx].value.passVal, ReceivedString);
                            }

                            if (variables[idx].type == DYNAMIC_PASSWORD) {
                                snprintf(msgmqtt2, sizeof(msgmqtt2),
                                        "Generic Password updated: id=%d", id);
                            } else {
                                snprintf(msgmqtt2, sizeof(msgmqtt2),
                                        "Generic String updated: id=%d, Text=%s",
                                        id, ReceivedString);
                            }
                            logging(LOG_INFO, msgmqtt2);
                        }

                        if (variables[idx].type == DYNAMIC_BOOL) {
                            Value = incomingdata[counter++];
                            variables[idx].value.byteVal = (Value != 0) ? 1 : 0;
                            snprintf(msgmqtt2, sizeof(msgmqtt2),
                                    "Generic BOOL updated: id=%d, Value=%d",
                                    id, variables[idx].value.byteVal);
                            logging(LOG_INFO, msgmqtt2);
                        }
                        DynamicVariablesUpdated = true;

                        break;
                    }
                    //case 3:
                    //break; // should not be changable externally
                    //case 4:
                    //break; // should not be changable externally
                    //case 5:
                    //break; // should not be changable externally
                    default:{
                        // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/GENERIC");
                        snprintf(msgmqtt2, sizeof(msgmqtt2), "Generic: No Type");
                        logging(LOG_CAUTION,msgmqtt2);
                        break;
                        }
                    }
                break;
            case 3: // System
            {
                byte ValueLow = incomingdata[counter++];
                byte ValueHigh = incomingdata[counter++];
                int ValueInt = ((int)ValueHigh << 8) | ValueLow;

                // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/LOG");
                snprintf(msgmqtt2, sizeof(msgmqtt2), "SerialCommand TYPE02=3: %d", ValueInt);
                logging(LOG_INFO,msgmqtt2);
                if (Type02 == 6) { // Command
                    SerialCommand = ValueInt;
                    // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/LOG");
                    snprintf(msgmqtt2, sizeof(msgmqtt2), "SerialCommand: %d", ValueInt);
                    logging(LOG_INFO,msgmqtt2);;
                }

                if (Type02 == 14) { // Delete Output
                    DeleteOutput = ValueInt;
                    // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/LOG");
                    snprintf(msgmqtt2, sizeof(msgmqtt2), "DeleteOutput: %d", ValueInt);
                    logging(LOG_INFO,msgmqtt2);
                }

                if (Type02 == 15) { // Delete Input
                    DeleteInput = ValueInt;
                    // snprintf(msgmqtt, sizeof(msgmqtt), "%s%s", MQTTName, "/LOG");
                    snprintf(msgmqtt2, sizeof(msgmqtt2), "DeleteInput: %d", ValueInt);
                    logging(LOG_INFO,msgmqtt2);
                }
                break;
            }
            default:
            {
                break;
            }
        }
        // can this be removed?? ****************
        if (counter + 2 > endPos) break;
        if (incomingdata[counter] == 'E' && incomingdata[counter + 1] == '0' && incomingdata[counter + 2] == '2') {
            counter += 3;
        } else {
            break;
        }
        //****************************************
    }
    clearIncomingData();

}
