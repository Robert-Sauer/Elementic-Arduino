#pragma once

#include <stddef.h>
#include <stdint.h>

namespace elementic {

static const uint8_t WIRE_V1 = 0x01;
static const uint8_t WIRE_V1_ASCII_ALIAS = 0x31;
static const uint8_t WIRE_V2 = 0x02;
static const size_t V2_MAX_PAYLOAD = 192;
static const size_t V2_MAX_BODY = 202;
static const size_t V2_MAX_FRAME = 412;

enum MacResult : uint8_t {
    MAC_VALID = 0,
    MAC_INVALID = 1,
    MAC_UNTERMINATED = 2
};

MacResult elementicParseMac(const char* input, size_t capacity,
                            uint8_t out[6], char canonical[13]);

uint32_t crc32(const uint8_t* data, size_t length);
uint32_t crc32Update(uint32_t state, const uint8_t* data, size_t length);

enum V2CodecResult : uint8_t {
    V2_CODEC_OK = 0,
    V2_CODEC_INVALID_ARGUMENT,
    V2_CODEC_TOO_LARGE,
    V2_CODEC_TRUNCATED,
    V2_CODEC_BAD_PREFIX,
    V2_CODEC_BAD_ESCAPE,
    V2_CODEC_BAD_FLAGS,
    V2_CODEC_BAD_LENGTH,
    V2_CODEC_BAD_CRC
};

struct V2FrameView {
    uint8_t kind;
    uint8_t flags;
    uint16_t sequence;
    const uint8_t* payload;
    uint16_t payloadLength;
};

V2CodecResult encodeV2Frame(uint8_t kind, uint16_t sequence,
                            const uint8_t* payload, uint16_t payloadLength,
                            uint8_t* output, size_t outputCapacity,
                            size_t* outputLength);

V2CodecResult decodeV2Frame(const uint8_t* frame, size_t frameLength,
                            uint8_t* decodedBody, size_t decodedCapacity,
                            V2FrameView* view);

enum LegacyStorageResult : uint8_t {
    LEGACY_STORAGE_VALID = 0,
    LEGACY_STORAGE_NOT_SCHEMA1,
    LEGACY_STORAGE_TRUNCATED,
    LEGACY_STORAGE_AMBIGUOUS,
    LEGACY_STORAGE_INVALID_RECORD,
    LEGACY_STORAGE_MISSING_TERMINATOR
};

struct LegacyRecordView {
    uint8_t category;
    uint8_t field;
    uint8_t item;
    uint8_t length;
    const uint8_t* value;
    uint8_t valueLength;
};

typedef bool (*LegacyRecordVisitor)(const LegacyRecordView& record, void* context);

LegacyStorageResult visitReleasedStorageV1(const uint8_t* bytes, size_t length,
                                           LegacyRecordVisitor visitor,
                                           void* context,
                                           size_t* consumed);

}  // namespace elementic
