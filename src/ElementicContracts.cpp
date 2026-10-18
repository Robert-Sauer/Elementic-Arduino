#include "ElementicContracts.h"

#include <string.h>

namespace elementic {
namespace {

bool isTrimByte(char value) {
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

int hexNibble(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    return -1;
}

char hexUpper(uint8_t value) {
    return static_cast<char>(value < 10 ? '0' + value : 'A' + value - 10);
}

void putU16(uint8_t* out, uint16_t value) {
    out[0] = static_cast<uint8_t>(value & 0xFFU);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFFU);
}

void putU32(uint8_t* out, uint32_t value) {
    out[0] = static_cast<uint8_t>(value & 0xFFUL);
    out[1] = static_cast<uint8_t>((value >> 8) & 0xFFUL);
    out[2] = static_cast<uint8_t>((value >> 16) & 0xFFUL);
    out[3] = static_cast<uint8_t>((value >> 24) & 0xFFUL);
}

uint16_t getU16(const uint8_t* in) {
    return static_cast<uint16_t>(in[0]) |
           static_cast<uint16_t>(static_cast<uint16_t>(in[1]) << 8);
}

uint32_t getU32(const uint8_t* in) {
    return static_cast<uint32_t>(in[0]) |
           (static_cast<uint32_t>(in[1]) << 8) |
           (static_cast<uint32_t>(in[2]) << 16) |
           (static_cast<uint32_t>(in[3]) << 24);
}

bool mustEscape(uint8_t value) {
    return value == 0x53U || value == 0x45U || value == 0x7DU;
}

bool fixedLegacyWidth(uint8_t category, uint8_t field, uint8_t declared,
                      uint8_t* width) {
    if (category == 3U) {
        if (declared != 0U) return false;
        *width = 2U;
        return true;
    }
    if (declared != 0U) {
        *width = declared;
        return true;
    }
    if (category == 0U && (field <= 5U || field == 8U)) {
        *width = 1U;
        return true;
    }
    if (category == 1U && field == 0U) {
        *width = 1U;
        return true;
    }
    return false;
}

}  // namespace

MacResult elementicParseMac(const char* input, size_t capacity,
                            uint8_t out[6], char canonical[13]) {
    if (input == 0 || out == 0 || canonical == 0 || capacity == 0U) {
        return MAC_INVALID;
    }

    size_t length = 0U;
    while (length < capacity && input[length] != '\0') ++length;
    if (length == capacity) return MAC_UNTERMINATED;

    size_t begin = 0U;
    size_t end = length;
    while (begin < end && isTrimByte(input[begin])) ++begin;
    while (end > begin && isTrimByte(input[end - 1U])) --end;
    const size_t trimmedLength = end - begin;
    if (trimmedLength != 12U && trimmedLength != 17U) return MAC_INVALID;

    uint8_t parsed[6];
    if (trimmedLength == 12U) {
        for (size_t i = 0U; i < 6U; ++i) {
            const int high = hexNibble(input[begin + i * 2U]);
            const int low = hexNibble(input[begin + i * 2U + 1U]);
            if (high < 0 || low < 0) return MAC_INVALID;
            parsed[i] = static_cast<uint8_t>((high << 4) | low);
        }
    } else {
        const char separator = input[begin + 2U];
        if (separator != ':' && separator != '-') return MAC_INVALID;
        for (size_t i = 0U; i < 6U; ++i) {
            const size_t offset = begin + i * 3U;
            const int high = hexNibble(input[offset]);
            const int low = hexNibble(input[offset + 1U]);
            if (high < 0 || low < 0) return MAC_INVALID;
            if (i < 5U && input[offset + 2U] != separator) return MAC_INVALID;
            parsed[i] = static_cast<uint8_t>((high << 4) | low);
        }
    }

    bool allZero = true;
    bool allBroadcast = true;
    for (size_t i = 0U; i < 6U; ++i) {
        allZero = allZero && parsed[i] == 0U;
        allBroadcast = allBroadcast && parsed[i] == 0xFFU;
    }
    if (allZero || allBroadcast || (parsed[0] & 0x01U) != 0U) {
        return MAC_INVALID;
    }

    char formatted[13];
    for (size_t i = 0U; i < 6U; ++i) {
        formatted[i * 2U] = hexUpper(static_cast<uint8_t>(parsed[i] >> 4));
        formatted[i * 2U + 1U] = hexUpper(static_cast<uint8_t>(parsed[i] & 0x0FU));
    }
    formatted[12] = '\0';
    memcpy(out, parsed, sizeof(parsed));
    memcpy(canonical, formatted, sizeof(formatted));
    return MAC_VALID;
}

uint32_t crc32Update(uint32_t state, const uint8_t* data, size_t length) {
    if (data == 0 && length != 0U) return state;
    for (size_t i = 0U; i < length; ++i) {
        state ^= data[i];
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            const uint32_t mask = static_cast<uint32_t>(0U - (state & 1U));
            state = (state >> 1) ^ (0xEDB88320UL & mask);
        }
    }
    return state;
}

uint32_t crc32(const uint8_t* data, size_t length) {
    return crc32Update(0xFFFFFFFFUL, data, length) ^ 0xFFFFFFFFUL;
}

V2CodecResult encodeV2Frame(uint8_t kind, uint16_t sequence,
                            const uint8_t* payload, uint16_t payloadLength,
                            uint8_t* output, size_t outputCapacity,
                            size_t* outputLength) {
    if (output == 0 || outputLength == 0 ||
        (payload == 0 && payloadLength != 0U)) {
        return V2_CODEC_INVALID_ARGUMENT;
    }
    *outputLength = 0U;
    if (payloadLength > V2_MAX_PAYLOAD) return V2_CODEC_TOO_LARGE;

    uint8_t body[V2_MAX_BODY];
    body[0] = kind;
    body[1] = 0U;
    putU16(body + 2, sequence);
    putU16(body + 4, payloadLength);
    if (payloadLength != 0U) memcpy(body + 6, payload, payloadLength);

    uint32_t state = 0xFFFFFFFFUL;
    const uint8_t prefixForCrc[2] = {0x31U, WIRE_V2};
    state = crc32Update(state, prefixForCrc, sizeof(prefixForCrc));
    state = crc32Update(state, body, static_cast<size_t>(6U + payloadLength));
    putU32(body + 6U + payloadLength, state ^ 0xFFFFFFFFUL);
    const size_t bodyLength = static_cast<size_t>(10U + payloadLength);

    const uint8_t prefix[5] = {'S', '0', '1', '1', WIRE_V2};
    if (outputCapacity < sizeof(prefix) + 3U) return V2_CODEC_TOO_LARGE;
    memcpy(output, prefix, sizeof(prefix));
    size_t position = sizeof(prefix);
    for (size_t i = 0U; i < bodyLength; ++i) {
        if (mustEscape(body[i])) {
            if (position + 2U + 3U > outputCapacity) return V2_CODEC_TOO_LARGE;
            output[position++] = 0x7DU;
            output[position++] = static_cast<uint8_t>(body[i] ^ 0x20U);
        } else {
            if (position + 1U + 3U > outputCapacity) return V2_CODEC_TOO_LARGE;
            output[position++] = body[i];
        }
    }
    output[position++] = 'E';
    output[position++] = '0';
    output[position++] = '1';
    *outputLength = position;
    return V2_CODEC_OK;
}

V2CodecResult decodeV2Frame(const uint8_t* frame, size_t frameLength,
                            uint8_t* decodedBody, size_t decodedCapacity,
                            V2FrameView* view) {
    if (frame == 0 || decodedBody == 0 || view == 0) {
        return V2_CODEC_INVALID_ARGUMENT;
    }
    if (frameLength < 18U) return V2_CODEC_TRUNCATED;
    if (memcmp(frame, "S011", 4U) != 0 || frame[4] != WIRE_V2 ||
        memcmp(frame + frameLength - 3U, "E01", 3U) != 0) {
        return V2_CODEC_BAD_PREFIX;
    }

    size_t decodedLength = 0U;
    for (size_t i = 5U; i < frameLength - 3U; ++i) {
        uint8_t value = frame[i];
        if (value == 0x7DU) {
            if (++i >= frameLength - 3U) return V2_CODEC_BAD_ESCAPE;
            value = static_cast<uint8_t>(frame[i] ^ 0x20U);
            if (!mustEscape(value)) return V2_CODEC_BAD_ESCAPE;
        } else if (mustEscape(value)) {
            return V2_CODEC_BAD_ESCAPE;
        }
        if (decodedLength >= decodedCapacity || decodedLength >= V2_MAX_BODY) {
            return V2_CODEC_TOO_LARGE;
        }
        decodedBody[decodedLength++] = value;
    }
    if (decodedLength < 10U) return V2_CODEC_TRUNCATED;
    if (decodedBody[1] != 0U) return V2_CODEC_BAD_FLAGS;
    const uint16_t payloadLength = getU16(decodedBody + 4U);
    if (payloadLength > V2_MAX_PAYLOAD ||
        decodedLength != static_cast<size_t>(10U + payloadLength)) {
        return V2_CODEC_BAD_LENGTH;
    }

    uint32_t state = 0xFFFFFFFFUL;
    const uint8_t prefixForCrc[2] = {0x31U, WIRE_V2};
    state = crc32Update(state, prefixForCrc, sizeof(prefixForCrc));
    state = crc32Update(state, decodedBody, static_cast<size_t>(6U + payloadLength));
    const uint32_t expected = state ^ 0xFFFFFFFFUL;
    if (getU32(decodedBody + 6U + payloadLength) != expected) {
        return V2_CODEC_BAD_CRC;
    }

    view->kind = decodedBody[0];
    view->flags = decodedBody[1];
    view->sequence = getU16(decodedBody + 2U);
    view->payloadLength = payloadLength;
    view->payload = decodedBody + 6U;
    return V2_CODEC_OK;
}

LegacyStorageResult visitReleasedStorageV1(const uint8_t* bytes, size_t length,
                                           LegacyRecordVisitor visitor,
                                           void* context,
                                           size_t* consumed) {
    if (consumed != 0) *consumed = 0U;
    if (bytes == 0 || length < 3U || bytes[0] != 1U || bytes[1] != 1U) {
        return LEGACY_STORAGE_NOT_SCHEMA1;
    }
    size_t position = 2U;
    while (position < length && bytes[position] != 0xFFU) {
        if (position + 4U > length) return LEGACY_STORAGE_TRUNCATED;
        LegacyRecordView record;
        record.category = bytes[position++];
        record.field = bytes[position++];
        record.item = bytes[position++];
        record.length = bytes[position++];
        if (record.category > 3U) return LEGACY_STORAGE_INVALID_RECORD;
        if (record.category == 3U && (record.item != 0U || record.length != 0U)) {
            return LEGACY_STORAGE_AMBIGUOUS;
        }
        uint8_t width = 0U;
        if (!fixedLegacyWidth(record.category, record.field, record.length, &width)) {
            return LEGACY_STORAGE_AMBIGUOUS;
        }
        if (position + width > length) return LEGACY_STORAGE_TRUNCATED;
        record.value = bytes + position;
        record.valueLength = width;
        position += width;
        if (visitor != 0 && !visitor(record, context)) {
            return LEGACY_STORAGE_INVALID_RECORD;
        }
    }
    if (position >= length || bytes[position] != 0xFFU) {
        return LEGACY_STORAGE_MISSING_TERMINATOR;
    }
    if (consumed != 0) *consumed = position + 1U;
    return LEGACY_STORAGE_VALID;
}

}  // namespace elementic
