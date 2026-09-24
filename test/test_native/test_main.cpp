#include <unity.h>

#include "ElementicContracts.h"

#include <string.h>

using namespace elementic;

namespace {

size_t fromHex(const char* hex, uint8_t* output, size_t capacity) {
    const size_t chars = strlen(hex);
    TEST_ASSERT_EQUAL_UINT(0U, chars % 2U);
    TEST_ASSERT_TRUE(chars / 2U <= capacity);
    for (size_t i = 0; i < chars / 2U; ++i) {
        char pair[3] = {hex[i * 2U], hex[i * 2U + 1U], '\0'};
        output[i] = static_cast<uint8_t>(strtoul(pair, 0, 16));
    }
    return chars / 2U;
}

void test_mac_contract() {
    uint8_t output[6] = {9, 9, 9, 9, 9, 9};
    char canonical[13] = "unchanged";
    TEST_ASSERT_EQUAL(MAC_VALID,
                      elementicParseMac(" 02-aa-bb-cc-dd-ee\r\n", 22,
                                        output, canonical));
    const uint8_t expected[6] = {0x02, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, output, 6);
    TEST_ASSERT_EQUAL_STRING("02AABBCCDDEE", canonical);

    uint8_t unchanged[6];
    memcpy(unchanged, output, sizeof(output));
    char unchangedText[13];
    memcpy(unchangedText, canonical, sizeof(canonical));
    TEST_ASSERT_EQUAL(MAC_INVALID,
                      elementicParseMac("01AABBCCDDEE", 13, output, canonical));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(unchanged, output, 6);
    TEST_ASSERT_EQUAL_STRING(unchangedText, canonical);
}

void test_mac_unterminated() {
    const char notTerminated[12] = {'0','2','A','A','B','B','C','C','D','D','E','E'};
    uint8_t output[6] = {0};
    char canonical[13] = {0};
    TEST_ASSERT_EQUAL(MAC_UNTERMINATED,
                      elementicParseMac(notTerminated, sizeof(notTerminated),
                                        output, canonical));
}

void test_crc_fixture() {
    const uint8_t input[] = {'1','2','3','4','5','6','7','8','9'};
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926UL, crc32(input, sizeof(input)));
}

void test_v2_project_fixture() {
    uint8_t expected[V2_MAX_FRAME];
    const size_t expectedLength = fromHex(
        "5330313102020001000e00032300000800ac13014fb2674ad87d5db6e7dc453031",
        expected, sizeof(expected));
    uint8_t payload[14];
    fromHex("032300000800ac13014fb2674ad8", payload, sizeof(payload));
    uint8_t actual[V2_MAX_FRAME];
    size_t actualLength = 0;
    TEST_ASSERT_EQUAL(V2_CODEC_OK,
                      encodeV2Frame(0x02, 1, payload, sizeof(payload), actual,
                                    sizeof(actual), &actualLength));
    TEST_ASSERT_EQUAL_UINT(expectedLength, actualLength);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, expectedLength);

    uint8_t body[V2_MAX_BODY];
    V2FrameView view = {};
    TEST_ASSERT_EQUAL(V2_CODEC_OK,
                      decodeV2Frame(actual, actualLength, body, sizeof(body), &view));
    TEST_ASSERT_EQUAL_UINT8(0x02, view.kind);
    TEST_ASSERT_EQUAL_UINT16(1, view.sequence);
    TEST_ASSERT_EQUAL_UINT16(sizeof(payload), view.payloadLength);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, view.payload, sizeof(payload));
}

void test_v2_escape_fixture() {
    uint8_t expected[V2_MAX_FRAME];
    const size_t expectedLength = fromHex(
        "5330313102050001001300020200010d007d7330317d6530317d7330327d6530327d5d1f7c1385453031",
        expected, sizeof(expected));
    uint8_t payload[19];
    fromHex("020200010d005330314530315330324530327d", payload, sizeof(payload));
    uint8_t actual[V2_MAX_FRAME];
    size_t actualLength = 0;
    TEST_ASSERT_EQUAL(V2_CODEC_OK,
                      encodeV2Frame(0x05, 1, payload, sizeof(payload), actual,
                                    sizeof(actual), &actualLength));
    TEST_ASSERT_EQUAL_UINT(expectedLength, actualLength);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, expectedLength);
}

struct LegacyCapture {
    unsigned count;
    uint16_t systemValue;
    uint8_t pin;
};

bool captureLegacy(const LegacyRecordView& record, void* context) {
    LegacyCapture* capture = static_cast<LegacyCapture*>(context);
    ++capture->count;
    if (record.category == 3U) {
        capture->systemValue = static_cast<uint16_t>(record.value[0]) |
                               static_cast<uint16_t>(record.value[1] << 8);
    } else if (record.category == 0U && record.field == 0U) {
        capture->pin = record.value[0];
    }
    return true;
}

void test_released_storage_layout() {
    uint8_t bytes[32];
    const size_t length = fromHex("0101030a000001000000010007ffffff",
                                  bytes, sizeof(bytes));
    LegacyCapture capture = {};
    size_t consumed = 0;
    TEST_ASSERT_EQUAL(LEGACY_STORAGE_VALID,
                      visitReleasedStorageV1(bytes, length, captureLegacy,
                                             &capture, &consumed));
    TEST_ASSERT_EQUAL_UINT(2, capture.count);
    TEST_ASSERT_EQUAL_UINT16(1, capture.systemValue);
    TEST_ASSERT_EQUAL_UINT8(7, capture.pin);
    TEST_ASSERT_EQUAL_UINT(14, consumed);
}

void test_unreleased_four_byte_storage_rejected() {
    uint8_t bytes[16];
    const size_t length = fromHex("0101030d1300ffffff", bytes, sizeof(bytes));
    TEST_ASSERT_EQUAL(LEGACY_STORAGE_AMBIGUOUS,
                      visitReleasedStorageV1(bytes, length, 0, 0, 0));
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_mac_contract);
    RUN_TEST(test_mac_unterminated);
    RUN_TEST(test_crc_fixture);
    RUN_TEST(test_v2_project_fixture);
    RUN_TEST(test_v2_escape_fixture);
    RUN_TEST(test_released_storage_layout);
    RUN_TEST(test_unreleased_four_byte_storage_rejected);
    return UNITY_END();
}
