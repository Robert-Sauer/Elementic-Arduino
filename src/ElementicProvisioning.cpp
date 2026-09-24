#include <Arduino.h>
#include "ElementicEeprom.h"

#include "Elementic.h"

namespace {

static const uint16_t PROVISIONING_SLOT_SIZE = 53U;
static const uint16_t PROVISIONING_TOTAL_SIZE = PROVISIONING_SLOT_SIZE * 2U;
static const uint8_t COMMIT_MARKER = 0xA5U;

struct ProvisioningRecord {
  bool valid;
  uint32_t generation;
  uint8_t mac[6];
  char unitUlid[27];
  uint16_t offset;
};

uint8_t effectiveMac[6] = {0};
char effectiveCanonical[13] = {0};
bool effectiveMacValid = false;
ProvisioningRecord persisted = {};

uint16_t provisioningStart() {
  const int length = EEPROM.length();
  return length >= static_cast<int>(PROVISIONING_TOTAL_SIZE)
             ? static_cast<uint16_t>(length - PROVISIONING_TOTAL_SIZE)
             : 0U;
}

uint16_t readU16(uint16_t offset) {
  return static_cast<uint16_t>(EEPROM.read(offset)) |
         static_cast<uint16_t>(static_cast<uint16_t>(EEPROM.read(offset + 1U)) << 8);
}

uint32_t readU32(uint16_t offset) {
  uint32_t value = 0;
  for (uint8_t i = 0; i < 4U; ++i) {
    value |= static_cast<uint32_t>(EEPROM.read(offset + i)) << (8U * i);
  }
  return value;
}

void writeU16(uint16_t offset, uint16_t value) {
  EEPROM.write(offset, static_cast<uint8_t>(value & 0xFFU));
  EEPROM.write(offset + 1U, static_cast<uint8_t>(value >> 8));
}

void writeU32(uint16_t offset, uint32_t value) {
  for (uint8_t i = 0; i < 4U; ++i) {
    EEPROM.write(offset + i, static_cast<uint8_t>(value >> (8U * i)));
  }
}

bool validUnitUlid(const char value[27]) {
  if (value == 0 || value[26] != '\0' || value[0] < '0' || value[0] > '7') return false;
  for (uint8_t i = 0; i < 26U; ++i) {
    const char c = value[i];
    const bool digit = c >= '0' && c <= '9';
    const bool letter = c >= 'A' && c <= 'Z' && c != 'I' && c != 'L' && c != 'O' && c != 'U';
    if (!digit && !letter) return false;
  }
  return true;
}

bool validateMacBytes(const uint8_t mac[6], char canonical[13]) {
  if (mac == 0) return false;
  char text[13];
  snprintf(text, sizeof(text), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  uint8_t parsed[6];
  return elementic::elementicParseMac(text, sizeof(text), parsed, canonical) ==
         elementic::MAC_VALID;
}

bool readRecord(uint16_t offset, ProvisioningRecord& record) {
  record.valid = false;
  if (EEPROM.length() < static_cast<size_t>(offset + PROVISIONING_SLOT_SIZE)) return false;
  if (EEPROM.read(offset + 52U) != COMMIT_MARKER) return false;
  const char magic[4] = {
      static_cast<char>(EEPROM.read(offset)),
      static_cast<char>(EEPROM.read(offset + 1U)),
      static_cast<char>(EEPROM.read(offset + 2U)),
      static_cast<char>(EEPROM.read(offset + 3U))};
  if (memcmp(magic, "EUP1", 4U) != 0 || readU16(offset + 4U) != 1U ||
      readU16(offset + 6U) != 16U || readU16(offset + 12U) != 32U ||
      readU16(offset + 14U) != 0U) {
    return false;
  }
  uint8_t crcBytes[48];
  for (uint8_t i = 0; i < sizeof(crcBytes); ++i) crcBytes[i] = EEPROM.read(offset + i);
  if (elementic::crc32(crcBytes, sizeof(crcBytes)) != readU32(offset + 48U)) return false;
  for (uint8_t i = 0; i < 6U; ++i) record.mac[i] = EEPROM.read(offset + 16U + i);
  for (uint8_t i = 0; i < 26U; ++i) record.unitUlid[i] = static_cast<char>(EEPROM.read(offset + 22U + i));
  record.unitUlid[26] = '\0';
  char canonical[13];
  if (!validateMacBytes(record.mac, canonical) || !validUnitUlid(record.unitUlid)) return false;
  record.generation = readU32(offset + 8U);
  if (record.generation == 0U) return false;
  record.offset = offset;
  record.valid = true;
  return true;
}

void loadPersisted() {
  persisted = ProvisioningRecord();
  if (EEPROM.length() < static_cast<int>(PROVISIONING_TOTAL_SIZE)) return;
  ProvisioningRecord first = {};
  ProvisioningRecord second = {};
  const uint16_t start = provisioningStart();
  readRecord(start, first);
  readRecord(static_cast<uint16_t>(start + PROVISIONING_SLOT_SIZE), second);
  if (first.valid && second.valid && first.generation == second.generation &&
      (memcmp(first.mac, second.mac, 6U) != 0 || strcmp(first.unitUlid, second.unitUlid) != 0)) {
    return;
  }
  if (first.valid && (!second.valid || first.generation >= second.generation)) persisted = first;
  else if (second.valid) persisted = second;
}

bool commitRecord(const uint8_t mac[6], const char unitUlid[27]) {
  if (EEPROM.length() < static_cast<int>(PROVISIONING_TOTAL_SIZE) ||
      !validUnitUlid(unitUlid)) return false;
  char canonical[13];
  if (!validateMacBytes(mac, canonical)) return false;
  loadPersisted();
  if (persisted.valid && persisted.generation == 0xFFFFFFFFUL) return false;

  const uint16_t start = provisioningStart();
  const uint16_t offset = !persisted.valid || persisted.offset != start
                              ? start
                              : static_cast<uint16_t>(start + PROVISIONING_SLOT_SIZE);
  const uint32_t generation = persisted.valid ? persisted.generation + 1U : 1U;
  EEPROM.write(offset + 52U, 0xFFU);
  EEPROM.write(offset, 'E'); EEPROM.write(offset + 1U, 'U');
  EEPROM.write(offset + 2U, 'P'); EEPROM.write(offset + 3U, '1');
  writeU16(offset + 4U, 1U);
  writeU16(offset + 6U, 16U);
  writeU32(offset + 8U, generation);
  writeU16(offset + 12U, 32U);
  writeU16(offset + 14U, 0U);
  for (uint8_t i = 0; i < 6U; ++i) EEPROM.write(offset + 16U + i, mac[i]);
  for (uint8_t i = 0; i < 26U; ++i) EEPROM.write(offset + 22U + i, unitUlid[i]);
  uint8_t crcBytes[48];
  for (uint8_t i = 0; i < sizeof(crcBytes); ++i) crcBytes[i] = EEPROM.read(offset + i);
  writeU32(offset + 48U, elementic::crc32(crcBytes, sizeof(crcBytes)));
  EEPROM.write(offset + 52U, COMMIT_MARKER);
  #if defined(ESP8266) || defined(ESP32)
    if (!EEPROM.commit()) return false;
  #endif
  ProvisioningRecord verified = {};
  if (!readRecord(offset, verified) || verified.generation != generation) return false;
  persisted = verified;
  return true;
}

bool chooseFallback(uint8_t mac[6], char canonical[13]) {
  if (effectiveMacValid) {
    memcpy(mac, effectiveMac, 6U);
    memcpy(canonical, effectiveCanonical, 13U);
    return true;
  }
  loadPersisted();
  if (persisted.valid && validateMacBytes(persisted.mac, canonical)) {
    memcpy(mac, persisted.mac, 6U);
    return true;
  }
  if (validateMacBytes(factorymac, canonical)) {
    memcpy(mac, factorymac, 6U);
    return true;
  }
  return false;
}

}  // namespace

ElementicMacStatus elementicMacStatus = ELEMENTIC_MAC_PROVISIONING_REQUIRED;
ElementicStorageStatus elementicStorageStatus = ELEMENTIC_STORAGE_COMPILED_ONLY;

bool elementicGetEffectiveMac(uint8_t out[6]) {
  if (out == 0 || !effectiveMacValid) return false;
  memcpy(out, effectiveMac, 6U);
  return true;
}

bool elementicApplySharedMac(bool persistValue) {
  const int index = findVariableIndexByName("Mac Address");
  if (index < 0 || variables[index].type != DYNAMIC_STRING) {
    elementicMacStatus = ELEMENTIC_MAC_PROVISIONING_REQUIRED;
    effectiveMacValid = false;
    return false;
  }

  uint8_t parsed[6];
  char canonical[13];
  const elementic::MacResult result = elementic::elementicParseMac(
      variables[index].value.strVal, sizeof(variables[index].value.strVal),
      parsed, canonical);
  if (result != elementic::MAC_VALID) {
    if (!chooseFallback(parsed, canonical)) {
      variables[index].value.strVal[0] = '\0';
      effectiveMacValid = false;
      elementicMacStatus = ELEMENTIC_MAC_PROVISIONING_REQUIRED;
      return false;
    }
    elementicMacStatus = ELEMENTIC_MAC_CORRECTED;
  } else {
    elementicMacStatus = ELEMENTIC_MAC_VALID;
  }

  memcpy(effectiveMac, parsed, sizeof(effectiveMac));
  memcpy(effectiveCanonical, canonical, sizeof(effectiveCanonical));
  effectiveMacValid = true;
  SetStringValue("Mac Address", canonical);

  if (persistValue) {
    loadPersisted();
    if (!persisted.valid || !commitRecord(parsed, persisted.unitUlid)) return false;
  }
  return true;
}

bool elementicProvisionMac(const uint8_t mac[6], const char unitUlid[27]) {
  if (!commitRecord(mac, unitUlid)) return false;
  memcpy(effectiveMac, mac, sizeof(effectiveMac));
  snprintf(effectiveCanonical, sizeof(effectiveCanonical), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  effectiveMacValid = true;
  elementicMacStatus = ELEMENTIC_MAC_VALID;
  SetStringValue("Mac Address", effectiveCanonical);
  return true;
}

void elementicEnterMacReprovisioning() {
  elementicMacStatus = ELEMENTIC_MAC_PROVISIONING_REQUIRED;
  WIFIConnected = false;
  EthernetConnected = false;
  networkConnected = false;
}
