#ifndef STAGE_PRESETS_H
#define STAGE_PRESETS_H

#include <stddef.h>
#include <stdint.h>

// Dirt stage preset manager.
//
// Slots live in EEPROM (when available) so a long-press on the front-panel
// buttons can morph between curated gremlin stacks without re-flashing the
// Teensy. Serial commands mirror the behaviour for folks who want to script
// preset changes from a laptop or DAW.

void setupStagePresets();

// Cycle through the stored presets. Returns true when a new preset is applied.
bool selectNextStagePreset();
bool selectPreviousStagePreset();

// Explicitly load or update a preset slot. `apply` controls whether the mask is
// pushed to the audio engine immediately.
bool loadStagePreset(uint8_t slot, bool announce = true);
bool storeStagePreset(uint8_t slot, uint8_t mask, bool announce = true,
                      bool apply = false);

// Query helpers so UI or debug routines can render the current state.
uint8_t stagePresetSlotCount();
uint8_t currentStagePresetIndex();
uint8_t stagePresetMask(uint8_t slot);
const char *factoryStagePresetStackId(uint8_t slot);
bool stagePresetMuteSupported();
void resetStagePresetsForTest();

// Pump this each loop to read serial commands ("preset list", "preset load",
// "preset save", etc.). The parser is forgiving and ignores unknown commands so
// you can spam the monitor while playing.
void pollStagePresetSerial();

#endif  // STAGE_PRESETS_H
