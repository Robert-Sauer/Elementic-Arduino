#include <Arduino.h>
#include "ElementicEeprom.h"

#include "Elementic.h"

namespace {

static const uint16_t CONFIG_HEADER_SIZE = 32U;
static const uint16_t PROVISIONING_TOTAL_SIZE = 106U;
static const uint8_t COMMIT_MARKER = 0xA5U;

struct SlotInfo {
  bool valid;
  uint16_t offset;
  uint16_t slotSize;
  uint32_t generation;
  uint32_t payloadLength;
  uint16_t recordCount;
  uint64_t lineage;
};

uint16_t readU16(uint32_t offset) {
  return static_cast<uint16_t>(EEPROM.read(offset)) |
         static_cast<uint16_t>(static_cast<uint16_t>(EEPROM.read(offset + 1U)) << 8);
}

uint32_t readU32(uint32_t offset) {
  uint32_t value = 0;
  for (uint8_t i = 0; i < 4U; ++i) value |= static_cast<uint32_t>(EEPROM.read(offset + i)) << (8U * i);
  return value;
}

uint64_t readU64(uint32_t offset) {
  uint64_t value = 0;
  for (uint8_t i = 0; i < 8U; ++i) value |= static_cast<uint64_t>(EEPROM.read(offset + i)) << (8U * i);
  return value;
}

void writeU16(uint32_t offset, uint16_t value) {
  EEPROM.write(offset, static_cast<uint8_t>(value));
  EEPROM.write(offset + 1U, static_cast<uint8_t>(value >> 8));
}

void writeU32(uint32_t offset, uint32_t value) {
  for (uint8_t i = 0; i < 4U; ++i) EEPROM.write(offset + i, static_cast<uint8_t>(value >> (8U * i)));
}

void writeU64(uint32_t offset, uint64_t value) {
  for (uint8_t i = 0; i < 8U; ++i) EEPROM.write(offset + i, static_cast<uint8_t>(value >> (8U * i)));
}

uint16_t configurationRegionSize() {
  const int length = EEPROM.length();
  return length > static_cast<int>(PROVISIONING_TOTAL_SIZE)
             ? static_cast<uint16_t>(length - PROVISIONING_TOTAL_SIZE)
             : 0U;
}

uint16_t slotSize() {
  return static_cast<uint16_t>(configurationRegionSize() / 2U);
}

uint32_t crcEeprom(uint32_t headerOffset, uint32_t payloadOffset, uint32_t payloadLength) {
  uint32_t state = 0xFFFFFFFFUL;
  uint8_t chunk[24];
  for (uint8_t i = 0; i < 24U; ++i) chunk[i] = EEPROM.read(headerOffset + i);
  state = elementic::crc32Update(state, chunk, sizeof(chunk));
  for (uint8_t i = 0; i < 4U; ++i) chunk[i] = EEPROM.read(headerOffset + 24U + i);
  state = elementic::crc32Update(state, chunk, 4U);
  uint32_t position = 0U;
  while (position < payloadLength) {
    const uint8_t take = static_cast<uint8_t>(min(static_cast<uint32_t>(sizeof(chunk)), payloadLength - position));
    for (uint8_t i = 0; i < take; ++i) chunk[i] = EEPROM.read(payloadOffset + position + i);
    state = elementic::crc32Update(state, chunk, take);
    position += take;
  }
  return state ^ 0xFFFFFFFFUL;
}

bool readSlot(uint16_t offset, uint16_t size, SlotInfo& slot) {
  slot = SlotInfo();
  slot.offset = offset;
  slot.slotSize = size;
  if (size < CONFIG_HEADER_SIZE + 1U) return false;
  if (EEPROM.read(offset) != 'E' || EEPROM.read(offset + 1U) != 'C' ||
      EEPROM.read(offset + 2U) != 'F' || EEPROM.read(offset + 3U) != '2' ||
      readU16(offset + 4U) != 2U || readU16(offset + 6U) != CONFIG_HEADER_SIZE ||
      readU16(offset + 24U) != 1U) {
    return false;
  }
  slot.generation = readU32(offset + 8U);
  slot.payloadLength = readU32(offset + 12U);
  slot.lineage = readU64(offset + 16U);
  slot.recordCount = readU16(offset + 26U);
  if (slot.generation == 0U || slot.payloadLength > size - CONFIG_HEADER_SIZE - 1U) return false;
  const uint32_t commit = static_cast<uint32_t>(offset) + CONFIG_HEADER_SIZE + slot.payloadLength;
  if (EEPROM.read(commit) != COMMIT_MARKER) return false;
  if (readU32(offset + 28U) != crcEeprom(offset, offset + CONFIG_HEADER_SIZE, slot.payloadLength)) return false;
  slot.valid = true;
  return true;
}

bool selectSlot(SlotInfo& selected, SlotInfo* other = 0) {
  const uint16_t size = slotSize();
  SlotInfo first = {};
  SlotInfo second = {};
  readSlot(0U, size, first);
  readSlot(size, size, second);
  if (first.valid && second.valid && first.generation == second.generation) {
    selected = SlotInfo();
    return false;
  }
  if (first.valid && (!second.valid || first.generation > second.generation)) {
    selected = first;
    if (other) *other = second;
    return true;
  }
  if (second.valid) {
    selected = second;
    if (other) *other = first;
    return true;
  }
  selected = SlotInfo();
  if (other) *other = SlotInfo();
  return false;
}

struct RecordWriter {
  bool write;
  bool ok;
  uint32_t position;
  uint32_t limit;
  uint16_t records;

  RecordWriter(bool shouldWrite, uint32_t begin, uint32_t end)
      : write(shouldWrite), ok(true), position(begin), limit(end), records(0U) {}

  bool record(uint8_t category, uint8_t field, uint16_t item,
              const uint8_t* value, uint16_t length) {
    if (!ok || (value == 0 && length != 0U) || position + 6U + length > limit) {
      ok = false;
      return false;
    }
    if (write) {
      EEPROM.write(position, category);
      EEPROM.write(position + 1U, field);
      writeU16(position + 2U, item);
      writeU16(position + 4U, length);
      for (uint16_t i = 0; i < length; ++i) EEPROM.write(position + 6U + i, value[i]);
    }
    position += 6U + length;
    ++records;
    return true;
  }
};

void emitU8(RecordWriter& writer, uint8_t category, uint8_t field,
            uint16_t item, uint8_t value) {
  writer.record(category, field, item, &value, 1U);
}

void emitU16(RecordWriter& writer, uint8_t category, uint8_t field,
             uint16_t item, uint16_t value) {
  const uint8_t bytes[2] = {static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 8)};
  writer.record(category, field, item, bytes, sizeof(bytes));
}

void emitText(RecordWriter& writer, uint8_t category, uint8_t field,
              uint16_t item, const char* value, size_t capacity) {
  writer.record(category, field, item, reinterpret_cast<const uint8_t*>(value),
                static_cast<uint16_t>(strnlen(value, capacity)));
}

void emitConfiguration(RecordWriter& writer) {
  const uint8_t outputCount = min(OutputCounter, static_cast<uint8_t>(OutputChannels));
  for (uint16_t item = 1U; item <= outputCount; ++item) {
    emitU16(writer, 0U, 0U, item, OutputPin[item]);
    emitU8(writer, 0U, 1U, item, OutputType[item]);
    emitU16(writer, 0U, 2U, item, OutputRelay[item]);
    emitU8(writer, 0U, 3U, item, OutputMinValue[item]);
    emitU8(writer, 0U, 4U, item, OutputMaxValue[item]);
    emitU8(writer, 0U, 5U, item, OutputDefaultValue[item]);
    emitText(writer, 0U, 6U, item, Output_Name[item], sizeof(Output_Name[item]));
    emitText(writer, 0U, 7U, item, MQTT_Output[item], sizeof(MQTT_Output[item]));
    emitU8(writer, 0U, 8U, item, OutputValueActual[item]);
    emitU16(writer, 0U, 10U, item, OutputChannel[item]);
  }
  const uint16_t inputCount = static_cast<uint16_t>(min(SwitchCounter, static_cast<int>(InputChannels)));
  for (uint16_t item = 1U; item <= inputCount; ++item) {
    emitU16(writer, 1U, 0U, item, SwitchPin[item]);
    uint8_t links[INOUT_MATRIX_COLS * 2U];
    const uint8_t linkCount = min(InOutMatrixCouter[item], static_cast<uint8_t>(INOUT_MATRIX_COLS));
    for (uint8_t i = 0; i < linkCount; ++i) {
      links[i * 2U] = InOutMatrix[item][i];
      links[i * 2U + 1U] = 0U;
    }
    writer.record(1U, 1U, item, links, static_cast<uint16_t>(linkCount * 2U));
    emitText(writer, 1U, 2U, item, MQTT_Input[item], sizeof(MQTT_Input[item]));
    emitU8(writer, 1U, 3U, item, 1U);
    emitText(writer, 1U, 5U, item, Input_Name[item], sizeof(Input_Name[item]));
  }
  for (int priority = 1; priority < static_cast<int>(PriorityCounter); ++priority) {
    const int i = findVariableIndexByPriority(priority);
    if (i < 0 || i >= totalVariables || variables[i].name.length() == 0 ||
        variables[i].priority < 1 || variables[i].priority > 65535) continue;
    const uint16_t item = static_cast<uint16_t>(variables[i].priority);
    emitU8(writer, 2U, 0U, item, static_cast<uint8_t>(variables[i].type));
    emitText(writer, 2U, 1U, item, variables[i].name.c_str(), variables[i].name.length() + 1U);
    if (variables[i].name != "Mac Address") {
      switch (variables[i].type) {
        case DYNAMIC_STRING:
          emitText(writer, 2U, 2U, item, variables[i].value.strVal, sizeof(variables[i].value.strVal));
          break;
        case DYNAMIC_PASSWORD:
          emitText(writer, 2U, 2U, item, variables[i].value.passVal, sizeof(variables[i].value.passVal));
          break;
        case DYNAMIC_BYTE:
        case DYNAMIC_BOOL:
          emitU8(writer, 2U, 2U, item, variables[i].value.byteVal);
          break;
        case DYNAMIC_INT: {
          const int16_t value = static_cast<int16_t>(variables[i].value.intVal);
          emitU16(writer, 2U, 2U, item, static_cast<uint16_t>(value));
          break;
        }
        case DYNAMIC_IP:
          writer.record(2U, 2U, item, variables[i].value.ipVal, 4U);
          break;
      }
    }
    emitU8(writer, 2U, 5U, item, DynamicVariableStatus[i]);
  }
  const uint8_t highWater[8] = {
      static_cast<uint8_t>(outputCount + 1U), 0, 0, 0,
      static_cast<uint8_t>(inputCount + 1U), 0, 0, 0};
  writer.record(3U, 40U, 0U, highWater, sizeof(highWater));
}

bool validateRecord(uint8_t category, uint8_t field, uint16_t item,
                    uint16_t length, uint32_t valueOffset) {
  if (category == 0U) {
    if (item == 0U || item > OutputChannels) return false;
    if (field == 0U || field == 2U || field == 10U) return length == 2U;
    if (field == 1U || (field >= 3U && field <= 5U) || field == 8U) return length == 1U;
    if (field == 6U) return length < MAXNAMELENGTH;
    if (field == 7U) return length < MAXTOPICLENGTH;
    return false;
  }
  if (category == 1U) {
    if (item == 0U || item > InputChannels) return false;
    if (field == 0U) return length == 2U;
    if (field == 1U) {
      if ((length & 1U) != 0U || length > INOUT_MATRIX_COLS * 2U) return false;
      for (uint16_t i = 0; i < length; i += 2U) {
        const uint16_t link = readU16(valueOffset + i);
        if (link == 0U || link > OutputChannels) return false;
      }
      return true;
    }
    if (field == 2U) return length < MAXTOPICLENGTH;
    if (field == 3U) return length == 1U && EEPROM.read(valueOffset) == 1U;
    if (field == 5U) return length < MAXNAMELENGTH;
    return false;
  }
  if (category == 2U) {
    if (item == 0U || item > 255U) return false;
    if (field == 0U || field == 5U) return length == 1U;
    if (field == 1U) return length > 0U && length <= 63U;
    if (field == 2U) return length <= 127U;
    return false;
  }
  return category == 3U && field == 40U && item == 0U && length == 8U;
}

bool validatePayload(const SlotInfo& slot) {
  uint32_t position = static_cast<uint32_t>(slot.offset) + CONFIG_HEADER_SIZE;
  const uint32_t end = position + slot.payloadLength;
  uint16_t records = 0U;
  uint8_t previousCategory = 0U;
  uint16_t previousItem = 0U;
  uint8_t previousField = 0U;
  bool first = true;
  while (position < end) {
    if (position + 6U > end) return false;
    const uint8_t category = EEPROM.read(position);
    const uint8_t field = EEPROM.read(position + 1U);
    const uint16_t item = readU16(position + 2U);
    const uint16_t length = readU16(position + 4U);
    const uint32_t value = position + 6U;
    if (value + length > end || !validateRecord(category, field, item, length, value)) return false;
    if (!first && (category < previousCategory ||
        (category == previousCategory && item < previousItem) ||
        (category == previousCategory && item == previousItem && field <= previousField))) return false;
    first = false;
    previousCategory = category;
    previousItem = item;
    previousField = field;
    position = value + length;
    ++records;
  }
  return position == end && records == slot.recordCount;
}

void copyTextFromEeprom(char* destination, size_t capacity, uint32_t offset, uint16_t length) {
  const size_t count = min(static_cast<size_t>(length), capacity - 1U);
  for (size_t i = 0; i < count; ++i) destination[i] = static_cast<char>(EEPROM.read(offset + i));
  destination[count] = '\0';
}

bool applyPayload(const SlotInfo& slot) {
  uint32_t position = static_cast<uint32_t>(slot.offset) + CONFIG_HEADER_SIZE;
  const uint32_t end = position + slot.payloadLength;
  uint16_t maxOutput = 0U;
  uint16_t maxInput = 0U;
  while (position < end) {
    const uint8_t category = EEPROM.read(position);
    const uint8_t field = EEPROM.read(position + 1U);
    const uint16_t item = readU16(position + 2U);
    const uint16_t length = readU16(position + 4U);
    const uint32_t value = position + 6U;
    if (category == 0U) {
      maxOutput = max(maxOutput, item);
      switch (field) {
        case 0: OutputPin[item] = static_cast<uint8_t>(readU16(value)); break;
        case 1: OutputType[item] = EEPROM.read(value); break;
        case 2: OutputRelay[item] = static_cast<uint8_t>(readU16(value)); break;
        case 3: OutputMinValue[item] = EEPROM.read(value); break;
        case 4: OutputMaxValue[item] = EEPROM.read(value); break;
        case 5: OutputDefaultValue[item] = EEPROM.read(value); break;
        case 6: copyTextFromEeprom(Output_Name[item], sizeof(Output_Name[item]), value, length); break;
        case 7: copyTextFromEeprom(MQTT_Output[item], sizeof(MQTT_Output[item]), value, length); break;
        case 8: OutputValueActual[item] = EEPROM.read(value); break;
        case 10: OutputChannel[item] = static_cast<uint8_t>(readU16(value)); break;
      }
    } else if (category == 1U) {
      maxInput = max(maxInput, item);
      if (field == 0U) SwitchPin[item] = static_cast<uint8_t>(readU16(value));
      else if (field == 1U) {
        InOutMatrixCouter[item] = static_cast<uint8_t>(length / 2U);
        for (uint8_t i = 0; i < InOutMatrixCouter[item]; ++i) InOutMatrix[item][i] = static_cast<uint8_t>(readU16(value + i * 2U));
      } else if (field == 2U) copyTextFromEeprom(MQTT_Input[item], sizeof(MQTT_Input[item]), value, length);
      else if (field == 5U) copyTextFromEeprom(Input_Name[item], sizeof(Input_Name[item]), value, length);
    } else if (category == 2U) {
      const int index = findVariableIndexByPriority(item);
      if (index >= 0 && index < totalVariables) {
        if (field == 2U && variables[index].name != "Mac Address") {
          switch (variables[index].type) {
            case DYNAMIC_STRING: copyTextFromEeprom(variables[index].value.strVal, sizeof(variables[index].value.strVal), value, length); break;
            case DYNAMIC_PASSWORD: copyTextFromEeprom(variables[index].value.passVal, sizeof(variables[index].value.passVal), value, length); break;
            case DYNAMIC_BYTE:
            case DYNAMIC_BOOL: variables[index].value.byteVal = EEPROM.read(value); break;
            case DYNAMIC_INT: variables[index].value.intVal = static_cast<int16_t>(readU16(value)); break;
            case DYNAMIC_IP: for (uint8_t i = 0; i < 4U; ++i) variables[index].value.ipVal[i] = EEPROM.read(value + i); break;
          }
        } else if (field == 5U) DynamicVariableStatus[index] = EEPROM.read(value);
      }
    }
    position = value + length;
  }
  OutputCounter = static_cast<uint8_t>(maxOutput);
  SwitchCounter = static_cast<int>(maxInput);
  elementicApplySharedMac(false);
  return true;
}

bool legacyImageValid() {
  const int eepromLength = EEPROM.length();
  if (eepromLength < 3 || EEPROM.read(0) != 1U || EEPROM.read(1) != 1U) return false;
  uint32_t position = 2U;
  while (position < static_cast<uint32_t>(eepromLength) && EEPROM.read(position) != 0xFFU) {
    if (position + 4U > static_cast<uint32_t>(eepromLength)) return false;
    const uint8_t category = EEPROM.read(position++);
    const uint8_t field = EEPROM.read(position++);
    const uint8_t item = EEPROM.read(position++);
    const uint8_t declared = EEPROM.read(position++);
    uint16_t width = declared;
    if (category == 3U) {
      if (item != 0U || declared != 0U || field < 10U || field > 13U) return false;
      width = 2U;
    } else if (category == 0U) {
      if (item == 0U || item > OutputChannels || field > 8U) return false;
      if (declared == 0U && field <= 5U) width = 1U;
      else if (declared == 0U && field == 8U) width = 1U;
      else if (declared == 0U && field != 6U && field != 7U) return false;
    } else if (category == 1U) {
      if (item == 0U || item > InputChannels || !(field == 0U || field == 1U || field == 2U || field == 5U)) return false;
      if (declared == 0U && field == 0U) width = 1U;
    } else if (category == 2U) {
      if (item >= totalVariables || field > 2U) return false;
      if (declared == 0U) return false;
    } else return false;
    if (position + width > static_cast<uint32_t>(eepromLength) ||
        position + width > configurationRegionSize()) return false;
    position += width;
  }
  return position < static_cast<uint32_t>(eepromLength) && EEPROM.read(position) == 0xFFU;
}

}  // namespace

bool elementicSaveConfiguration() {
  const uint16_t size = slotSize();
  if (size < CONFIG_HEADER_SIZE + 1U) {
    elementicStorageStatus = ELEMENTIC_STORAGE_UNAVAILABLE;
    return false;
  }
  SlotInfo active = {};
  SlotInfo inactive = {};
  const bool hasActive = selectSlot(active, &inactive);
  if (hasActive && active.generation == 0xFFFFFFFFUL) {
    elementicStorageStatus = ELEMENTIC_STORAGE_UNAVAILABLE;
    return false;
  }
  const uint16_t offset = !hasActive || active.offset != 0U ? 0U : size;
  const uint32_t generation = hasActive ? active.generation + 1U : 1U;
  const uint32_t payloadBegin = static_cast<uint32_t>(offset) + CONFIG_HEADER_SIZE;
  RecordWriter measure(false, payloadBegin, static_cast<uint32_t>(offset) + size - 1U);
  emitConfiguration(measure);
  if (!measure.ok || measure.position - payloadBegin > 0xFFFFFFFFUL) {
    elementicStorageStatus = ELEMENTIC_STORAGE_UNAVAILABLE;
    return false;
  }
  const uint32_t payloadLength = measure.position - payloadBegin;
  SlotInfo previousTarget = {};
  if (readSlot(offset, size, previousTarget)) {
    EEPROM.write(static_cast<uint32_t>(offset) + CONFIG_HEADER_SIZE +
                     previousTarget.payloadLength,
                 0xFFU);
  }
  EEPROM.write(static_cast<uint32_t>(offset) + CONFIG_HEADER_SIZE + payloadLength, 0xFFU);
  EEPROM.write(offset, 'E'); EEPROM.write(offset + 1U, 'C');
  EEPROM.write(offset + 2U, 'F'); EEPROM.write(offset + 3U, '2');
  writeU16(offset + 4U, 2U);
  writeU16(offset + 6U, CONFIG_HEADER_SIZE);
  writeU32(offset + 8U, generation);
  writeU32(offset + 12U, payloadLength);
  writeU64(offset + 16U, ELEMENTIC_FIRMWARE_PROJECT_ID);
  writeU16(offset + 24U, 1U);
  writeU16(offset + 26U, measure.records);
  writeU32(offset + 28U, 0U);
  RecordWriter writer(true, payloadBegin, static_cast<uint32_t>(offset) + size - 1U);
  emitConfiguration(writer);
  if (!writer.ok || writer.records != measure.records || writer.position != measure.position) return false;
  writeU32(offset + 28U, crcEeprom(offset, payloadBegin, payloadLength));
  EEPROM.write(static_cast<uint32_t>(offset) + CONFIG_HEADER_SIZE + payloadLength, COMMIT_MARKER);
  #if defined(ESP8266) || defined(ESP32)
    if (!EEPROM.commit()) return false;
  #endif
  SlotInfo verified = {};
  if (!readSlot(offset, size, verified) || !validatePayload(verified)) {
    elementicStorageStatus = ELEMENTIC_STORAGE_CORRUPT;
    return false;
  }
  elementicApplySharedMac(true);
  EEPROMStorage = static_cast<int>(CONFIG_HEADER_SIZE + payloadLength + 1U);
  elementicStorageStatus = ELEMENTIC_STORAGE_VALID_SAVED;
  return true;
}

bool elementicLoadConfiguration() {
  SlotInfo selected = {};
  if (selectSlot(selected)) {
    if (selected.lineage != 0U && selected.lineage != ELEMENTIC_FIRMWARE_PROJECT_ID) {
      elementicStorageStatus = ELEMENTIC_STORAGE_MIGRATION_REQUIRED;
      return false;
    }
    if (!validatePayload(selected)) {
      elementicStorageStatus = ELEMENTIC_STORAGE_CORRUPT;
      return false;
    }
    applyPayload(selected);
    EEPROMStorage = static_cast<int>(CONFIG_HEADER_SIZE + selected.payloadLength + 1U);
    elementicStorageStatus = ELEMENTIC_STORAGE_VALID_SAVED;
    return true;
  }
  if (EEPROM.length() >= 2 && EEPROM.read(0) == 1U && EEPROM.read(1) == 1U) {
    if (!legacyImageValid()) {
      elementicStorageStatus = ELEMENTIC_STORAGE_MIGRATION_REQUIRED;
      return false;
    }
    ReadAllDataFromEEPROM();
    elementicApplySharedMac(false);
    elementicStorageStatus = ELEMENTIC_STORAGE_VALID_SAVED;
    return true;
  }
  if (EEPROM.length() >= 4 && EEPROM.read(0) == 'E' && EEPROM.read(1) == 'C' && EEPROM.read(2) == 'F') {
    elementicStorageStatus = ELEMENTIC_STORAGE_CORRUPT;
    return false;
  }
  elementicStorageStatus = ELEMENTIC_STORAGE_COMPILED_ONLY;
  return true;
}

bool elementicFactoryReset() {
  const uint16_t size = slotSize();
  if (size < CONFIG_HEADER_SIZE + 1U) return false;
  EEPROM.write(CONFIG_HEADER_SIZE, 0xFFU);
  EEPROM.write(static_cast<uint32_t>(size) + CONFIG_HEADER_SIZE, 0xFFU);
  EEPROM.write(0U, 0xFFU);
  EEPROM.write(size, 0xFFU);
  #if defined(ESP8266) || defined(ESP32)
    if (!EEPROM.commit()) return false;
  #endif
  elementicLoadFactoryDefaults();
  return elementicSaveConfiguration();
}

bool elementicVisitSavedConfiguration(ElementicStoredRecordVisitor visitor,
                                      void* context,
                                      uint32_t* generation,
                                      uint16_t* recordCount) {
  SlotInfo selected = {};
  if (!selectSlot(selected) || !validatePayload(selected) ||
      (selected.lineage != 0U && selected.lineage != ELEMENTIC_FIRMWARE_PROJECT_ID)) {
    return false;
  }
  if (generation) *generation = selected.generation;
  if (recordCount) *recordCount = selected.recordCount;
  if (visitor == 0) return true;

  uint32_t position = static_cast<uint32_t>(selected.offset) + CONFIG_HEADER_SIZE;
  const uint32_t end = position + selected.payloadLength;
  uint8_t value[128];
  while (position < end) {
    const uint8_t category = EEPROM.read(position);
    const uint8_t field = EEPROM.read(position + 1U);
    const uint16_t item = readU16(position + 2U);
    const uint16_t length = readU16(position + 4U);
    if (length > sizeof(value)) return false;
    for (uint16_t i = 0; i < length; ++i) value[i] = EEPROM.read(position + 6U + i);
    if (!visitor(category, field, item, value, length, context)) return false;
    position += 6U + length;
  }
  return position == end;
}
