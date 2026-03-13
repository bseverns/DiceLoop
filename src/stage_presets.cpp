#include "stage_presets.h"

#include "audio_pipeline.h"

#if __has_include(<Arduino.h>)
#include <Arduino.h>
#else
#include <cstdio>
#define Serial DummySerial::instance()
#ifndef DEC
#define DEC 10
#endif
#ifndef HEX
#define HEX 16
#endif
namespace {
class DummySerial {
 public:
  static DummySerial &instance() {
    static DummySerial serial;
    return serial;
  }
  void println(const char *msg) { fprintf(stderr, "%s\n", msg); }
  void print(const char *msg) { fprintf(stderr, "%s", msg); }
  void print(unsigned long value, int base = DEC) {
    if (base == HEX) {
      fprintf(stderr, "%lx", value);
    } else {
      fprintf(stderr, "%lu", value);
    }
  }
  void print(int value) { fprintf(stderr, "%d", value); }
  int available() const { return 0; }
  int read() { return -1; }
};
}  // namespace
#endif

#if __has_include(<EEPROM.h>)
#include <EEPROM.h>
#define DICELOOP_HAVE_EEPROM 1
#else
#define DICELOOP_HAVE_EEPROM 0
#endif

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
constexpr uint8_t kPresetSlotCount = 4;
constexpr uint8_t kPresetVersion = 1;
constexpr uint32_t kPresetMagic = 0x44505253;  // "DPRS" – Dirt PReSet
#ifndef DICELOOP_ALLOW_MUTED_STAGE_MASK
#define DICELOOP_ALLOW_MUTED_STAGE_MASK 0
#endif

constexpr int kMagicAddress = 0;
constexpr int kVersionAddress = kMagicAddress + sizeof(uint32_t);
constexpr int kSlotCountAddress = kVersionAddress + sizeof(uint8_t);
constexpr int kActiveSlotAddress = kSlotCountAddress + sizeof(uint8_t);
constexpr int kReservedAddress = kActiveSlotAddress + sizeof(uint8_t);
constexpr int kMaskAddress = kReservedAddress + sizeof(uint8_t);

constexpr const char *kFactoryPresetStackIds[kPresetSlotCount] = {
    "full_send",
    "crush_hiccups",
    "sine_smear",
    "fuzz_bloom",
};

uint8_t presetMasks[kPresetSlotCount];
uint8_t activePreset = 0;
bool stagePresetInitialised = false;

char descriptionBuffer[96];

uint8_t allowedStageMask() {
  uint8_t mask = 0;
  for (size_t i = 0; i < dirtStageCount(); ++i) {
    mask |= dirtStageBit(static_cast<DirtStage>(i));
  }
  return mask;
}

bool equalsIgnoreCase(const char *a, const char *b) {
  if (!a || !b) {
    return false;
  }
  while (*a && *b) {
    if (tolower(static_cast<unsigned char>(*a)) !=
        tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

const char *maskToDescription(uint8_t mask) {
  descriptionBuffer[0] = '\0';
  size_t len = 0;
  for (size_t i = 0; i < dirtStageCount(); ++i) {
    uint8_t bit = dirtStageBit(static_cast<DirtStage>(i));
    if ((mask & bit) == 0) {
      continue;
    }
    const char *id = dirtStageId(static_cast<DirtStage>(i));
    if (!id || id[0] == '\0') {
      continue;
    }
    if (len > 0 && len < sizeof(descriptionBuffer) - 1) {
      descriptionBuffer[len++] = '+';
      descriptionBuffer[len] = '\0';
    }
    size_t remaining = (len < sizeof(descriptionBuffer))
                           ? sizeof(descriptionBuffer) - len
                           : 0;
    if (remaining > 1) {
      strncat(descriptionBuffer + len, id, remaining - 1);
      len = strlen(descriptionBuffer);
    }
  }
  if (len == 0) {
    strncpy(descriptionBuffer, "(mute)", sizeof(descriptionBuffer) - 1);
    descriptionBuffer[sizeof(descriptionBuffer) - 1] = '\0';
  }
  return descriptionBuffer;
}

uint8_t sanitiseMask(uint8_t mask) {
  return static_cast<uint8_t>(mask & allowedStageMask());
}

bool maskIsAllowed(uint8_t mask) {
  uint8_t cleanMask = sanitiseMask(mask);
  if (cleanMask == 0) {
    return DICELOOP_ALLOW_MUTED_STAGE_MASK;
  }
  return true;
}

uint8_t fallbackMaskForSlot(uint8_t slot) {
  if (slot < kPresetSlotCount) {
    DirtStackInfo info;
    const char *stackId = kFactoryPresetStackIds[slot];
    if (curatedDirtStackById(stackId, &info)) {
      uint8_t mask = sanitiseMask(info.mask);
      if (mask != 0) {
        return mask;
      }
    }
  }
  return allowedStageMask();
}

uint8_t normaliseStoredMask(uint8_t slot, uint8_t mask) {
  uint8_t cleanMask = sanitiseMask(mask);
  if (maskIsAllowed(cleanMask)) {
    return cleanMask;
  }
  return fallbackMaskForSlot(slot);
}

void listCuratedStacks() {
  Serial.println("[presets] curated stacks");
  size_t count = curatedDirtStackCount();
  for (size_t i = 0; i < count; ++i) {
    DirtStackInfo info;
    if (!curatedDirtStackInfo(i, &info)) {
      continue;
    }
    Serial.print("  ");
    Serial.print(info.id);
    Serial.print("  mask 0x");
    Serial.print(info.mask, HEX);
    Serial.print("  ");
    if (info.label && info.label[0] != '\0') {
      Serial.print(info.label);
      Serial.print(" → ");
    }
    Serial.println(maskToDescription(sanitiseMask(info.mask)));
  }
}

bool applyCuratedStack(const DirtStackInfo &info, bool announce) {
  uint8_t mask = sanitiseMask(info.mask);
  for (size_t i = 0; i < kPresetSlotCount; ++i) {
    if (presetMasks[i] == mask) {
      return loadStagePreset(static_cast<uint8_t>(i), announce);
    }
  }
  setActiveDirtStages(mask);
  if (announce) {
    Serial.print("[presets] stack ");
    Serial.print(info.id);
    Serial.print(" → ");
    Serial.println(maskToDescription(mask));
    if (info.label && info.label[0] != '\0') {
      Serial.print("           ");
      Serial.println(info.label);
    }
  }
  return true;
}

void copyDefaults() {
  for (size_t i = 0; i < kPresetSlotCount; ++i) {
    DirtStackInfo info;
    uint8_t mask = fallbackMaskForSlot(static_cast<uint8_t>(i));
    const char *stackId = kFactoryPresetStackIds[i];
    if (curatedDirtStackById(stackId, &info)) {
      mask = sanitiseMask(info.mask);
    }
    presetMasks[i] = mask;
  }
  activePreset = 0;
}

#if DICELOOP_HAVE_EEPROM
void persistMetadata() {
  EEPROM.put(kMagicAddress, kPresetMagic);
  EEPROM.put(kVersionAddress, kPresetVersion);
  EEPROM.put(kSlotCountAddress, kPresetSlotCount);
  EEPROM.put(kActiveSlotAddress, activePreset);
}

void persistMasks() {
  for (size_t i = 0; i < kPresetSlotCount; ++i) {
    EEPROM.put(kMaskAddress + static_cast<int>(i), presetMasks[i]);
  }
}

void persistAll() {
  persistMetadata();
  persistMasks();
}

bool loadFromEEPROM() {
  uint32_t magic = 0;
  EEPROM.get(kMagicAddress, magic);
  uint8_t version = 0;
  EEPROM.get(kVersionAddress, version);
  uint8_t storedSlots = 0;
  EEPROM.get(kSlotCountAddress, storedSlots);
  uint8_t storedActive = 0;
  EEPROM.get(kActiveSlotAddress, storedActive);

  if (magic != kPresetMagic || version != kPresetVersion ||
      storedSlots != kPresetSlotCount) {
    copyDefaults();
    persistAll();
    return false;
  }

  for (size_t i = 0; i < kPresetSlotCount; ++i) {
    uint8_t value = 0;
    EEPROM.get(kMaskAddress + static_cast<int>(i), value);
    presetMasks[i] = normaliseStoredMask(static_cast<uint8_t>(i), value);
  }
  if (storedActive >= kPresetSlotCount) {
    storedActive = 0;
  }
  activePreset = storedActive;
  return true;
}
#endif  // DICELOOP_HAVE_EEPROM

void applyPreset(uint8_t slot, bool announce) {
  if (slot >= kPresetSlotCount) {
    return;
  }
  uint8_t mask = presetMasks[slot];
  setActiveDirtStages(mask);
  activePreset = slot;
#if DICELOOP_HAVE_EEPROM
  EEPROM.put(kActiveSlotAddress, activePreset);
#endif
  if (announce) {
    Serial.print("[presets] loaded slot ");
    Serial.print(slot);
    Serial.print(" → ");
    Serial.println(maskToDescription(mask));
  }
}

bool parseStageListToMask(char **tokens, size_t count, uint8_t &mask) {
  uint8_t result = 0;
  for (size_t i = 0; i < count; ++i) {
    DirtStage stage;
    if (!dirtStageById(tokens[i], &stage)) {
      Serial.print("[presets] unknown stage id: ");
      Serial.println(tokens[i]);
      return false;
    }
    result |= dirtStageBit(stage);
  }
  mask = sanitiseMask(result);
  if (!maskIsAllowed(mask)) {
    Serial.println("[presets] mute masks disabled in production builds");
    return false;
  }
  return true;
}

bool parseMaskToken(const char *token, uint8_t &mask) {
  if (!token) {
    return false;
  }
  char *end = nullptr;
  unsigned long value = strtoul(token, &end, 0);
  if (end == token) {
    return false;
  }
  mask = sanitiseMask(static_cast<uint8_t>(value));
  if (!maskIsAllowed(mask)) {
    return false;
  }
  return true;
}

void listPresets() {
  Serial.println("[presets] slots");
  for (size_t i = 0; i < kPresetSlotCount; ++i) {
    Serial.print("  ");
    Serial.print(i);
    Serial.print(i == activePreset ? "* " : "  ");
    Serial.print("factory:");
    Serial.print(kFactoryPresetStackIds[i]);
    Serial.print("  ");
    Serial.print("mask 0x");
    Serial.print(presetMasks[i], HEX);
    Serial.print("  stages: ");
    Serial.println(maskToDescription(presetMasks[i]));
  }
}

void showHelp() {
  Serial.println("[presets] commands:");
  Serial.println("  preset list");
  Serial.println("  preset load <slot>");
  Serial.println("  preset save <slot> <stage ids...>");
  Serial.println("  preset mask <slot> <hex mask>");
  Serial.println("  preset stacks");
  Serial.println("  preset stack list");
  Serial.println("  preset stack load <id>");
  Serial.println("  preset stack save <slot> <id>");
}

void handlePresetCommand(char **tokens, size_t count) {
  if (count == 0) {
    return;
  }
  if (equalsIgnoreCase(tokens[0], "list")) {
    listPresets();
    return;
  }
  if (equalsIgnoreCase(tokens[0], "stacks")) {
    listCuratedStacks();
    return;
  }
  if (equalsIgnoreCase(tokens[0], "stack")) {
    if (count < 2) {
      Serial.println("[presets] usage: preset stack <list|load|save> ...");
      return;
    }
    if (equalsIgnoreCase(tokens[1], "list")) {
      listCuratedStacks();
      return;
    }
    if (equalsIgnoreCase(tokens[1], "load")) {
      if (count < 3) {
        Serial.println("[presets] usage: preset stack load <id>");
        return;
      }
      DirtStackInfo info;
      if (!curatedDirtStackById(tokens[2], &info)) {
        Serial.print("[presets] unknown stack id: ");
        Serial.println(tokens[2]);
        return;
      }
      applyCuratedStack(info, true);
      return;
    }
    if (equalsIgnoreCase(tokens[1], "save")) {
      if (count < 4) {
        Serial.println("[presets] usage: preset stack save <slot> <id>");
        return;
      }
      int slot = atoi(tokens[2]);
      if (slot < 0 || slot >= static_cast<int>(kPresetSlotCount)) {
        Serial.println("[presets] invalid slot");
        return;
      }
      DirtStackInfo info;
      if (!curatedDirtStackById(tokens[3], &info)) {
        Serial.print("[presets] unknown stack id: ");
        Serial.println(tokens[3]);
        return;
      }
      storeStagePreset(static_cast<uint8_t>(slot), sanitiseMask(info.mask), true,
                       true);
      return;
    }
    Serial.println("[presets] unknown stack action – try 'preset stack list'");
    return;
  }
  if (equalsIgnoreCase(tokens[0], "load")) {
    if (count < 2) {
      Serial.println("[presets] usage: preset load <slot>");
      return;
    }
    int slot = atoi(tokens[1]);
    if (!loadStagePreset(slot)) {
      Serial.println("[presets] invalid slot");
    }
    return;
  }
  if (equalsIgnoreCase(tokens[0], "save")) {
    if (count < 3) {
      Serial.println("[presets] usage: preset save <slot> <stage ids...>");
      return;
    }
    int slot = atoi(tokens[1]);
    if (slot < 0 || slot >= static_cast<int>(kPresetSlotCount)) {
      Serial.println("[presets] invalid slot");
      return;
    }
    uint8_t mask = 0;
    if (!parseStageListToMask(&tokens[2], count - 2, mask)) {
      return;
    }
    storeStagePreset(static_cast<uint8_t>(slot), mask, true, true);
    return;
  }
  if (equalsIgnoreCase(tokens[0], "mask")) {
    if (count < 3) {
      Serial.println("[presets] usage: preset mask <slot> <value>");
      return;
    }
    int slot = atoi(tokens[1]);
    if (slot < 0 || slot >= static_cast<int>(kPresetSlotCount)) {
      Serial.println("[presets] invalid slot");
      return;
    }
    uint8_t mask = 0;
    if (!parseMaskToken(tokens[2], mask)) {
      Serial.println("[presets] bad mask value");
      return;
    }
    storeStagePreset(static_cast<uint8_t>(slot), mask, true, true);
    return;
  }
  if (equalsIgnoreCase(tokens[0], "help")) {
    showHelp();
    return;
  }
  Serial.println("[presets] unknown command – try 'preset help'");
}

void dispatchCommand(char *line) {
  char *tokens[12];
  size_t count = 0;
  char *cursor = strtok(line, " \t");
  while (cursor && count < (sizeof(tokens) / sizeof(tokens[0]))) {
    tokens[count++] = cursor;
    cursor = strtok(nullptr, " \t");
  }
  if (count == 0) {
    return;
  }
  if (equalsIgnoreCase(tokens[0], "preset")) {
    handlePresetCommand(&tokens[1], count - 1);
  } else if (equalsIgnoreCase(tokens[0], "help")) {
    showHelp();
  }
}

constexpr size_t kSerialBufferSize = 96;
char serialBuffer[kSerialBufferSize];
size_t serialIndex = 0;

}  // namespace

void setupStagePresets() {
  if (stagePresetInitialised) {
    return;
  }
  stagePresetInitialised = true;
#if DICELOOP_HAVE_EEPROM
  bool restored = loadFromEEPROM();
  if (!restored) {
    Serial.println("[presets] EEPROM empty – writing defaults");
  }
#else
  copyDefaults();
  Serial.println("[presets] EEPROM missing – using RAM presets only");
#endif
  applyPreset(activePreset, true);
}

bool selectNextStagePreset() {
  uint8_t next = static_cast<uint8_t>((activePreset + 1) % kPresetSlotCount);
  if (next == activePreset) {
    return false;
  }
  applyPreset(next, true);
  return true;
}

bool selectPreviousStagePreset() {
  uint8_t prev = static_cast<uint8_t>((activePreset + kPresetSlotCount - 1) %
                                      kPresetSlotCount);
  if (prev == activePreset) {
    return false;
  }
  applyPreset(prev, true);
  return true;
}

bool loadStagePreset(uint8_t slot, bool announce) {
  if (slot >= kPresetSlotCount) {
    return false;
  }
  if (slot == activePreset && !announce) {
    return true;
  }
  applyPreset(slot, announce);
  return true;
}

bool storeStagePreset(uint8_t slot, uint8_t mask, bool announce, bool apply) {
  if (slot >= kPresetSlotCount) {
    return false;
  }
  uint8_t cleanMask = sanitiseMask(mask);
  if (!maskIsAllowed(cleanMask)) {
    if (announce) {
      Serial.println("[presets] mute masks disabled in production builds");
    }
    return false;
  }
  presetMasks[slot] = cleanMask;
#if DICELOOP_HAVE_EEPROM
  persistMasks();
#endif
  if (announce) {
    Serial.print("[presets] slot ");
    Serial.print(slot);
    Serial.print(" saved → ");
    Serial.println(maskToDescription(cleanMask));
  }
  if (apply) {
    loadStagePreset(slot, announce);
  }
  return true;
}

uint8_t stagePresetSlotCount() { return kPresetSlotCount; }

uint8_t currentStagePresetIndex() { return activePreset; }

uint8_t stagePresetMask(uint8_t slot) {
  if (slot >= kPresetSlotCount) {
    return 0;
  }
  return presetMasks[slot];
}

const char *factoryStagePresetStackId(uint8_t slot) {
  if (slot >= kPresetSlotCount) {
    return nullptr;
  }
  return kFactoryPresetStackIds[slot];
}

bool stagePresetMuteSupported() {
  return DICELOOP_ALLOW_MUTED_STAGE_MASK;
}

void resetStagePresetsForTest() {
  copyDefaults();
  activePreset = 0;
  stagePresetInitialised = false;
  serialIndex = 0;
  serialBuffer[0] = '\0';
  setActiveDirtStages(presetMasks[activePreset]);
}

void pollStagePresetSerial() {
#if __has_include(<Arduino.h>)
  while (Serial.available() > 0) {
    int raw = Serial.read();
    if (raw < 0) {
      break;
    }
    char c = static_cast<char>(raw);
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      serialBuffer[serialIndex] = '\0';
      dispatchCommand(serialBuffer);
      serialIndex = 0;
      continue;
    }
    if (serialIndex < kSerialBufferSize - 1) {
      serialBuffer[serialIndex++] = c;
    }
  }
#else
  (void)serialBuffer;
  (void)serialIndex;
#endif
}
