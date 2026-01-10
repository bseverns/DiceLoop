#include <unity.h>

#include "stage_presets.h"
#include "audio_pipeline.h"
#include "Arduino.h"

void reset_presets_state() {
    dice_loop_stub::reset_state();
    setupStagePresets();
}

void test_stage_preset_defaults_and_store() {
    reset_presets_state();
    TEST_ASSERT_EQUAL_UINT8(4, stagePresetSlotCount());

    DirtStackInfo info{};
    TEST_ASSERT_TRUE(curatedDirtStackInfo(0, &info));
    TEST_ASSERT_EQUAL_UINT8(info.mask, stagePresetMask(0));

    uint8_t start_index = currentStagePresetIndex();
    selectNextStagePreset();
    TEST_ASSERT_NOT_EQUAL(start_index, currentStagePresetIndex());

    storeStagePreset(0, 0, false, false);
    TEST_ASSERT_NOT_EQUAL_UINT8(0, stagePresetMask(0));
}
