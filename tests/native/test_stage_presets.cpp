#include <unity.h>

#include "stage_presets.h"
#include "audio_pipeline.h"
#include "Arduino.h"
#include <string>

void reset_presets_state() {
    dice_loop_stub::reset_state();
    resetStagePresetsForTest();
    setupStagePresets();
}

void test_stage_preset_defaults_and_store() {
    reset_presets_state();
    TEST_ASSERT_EQUAL_UINT8(4, stagePresetSlotCount());
    TEST_ASSERT_FALSE(stagePresetMuteSupported());

    const char *expected_ids[] = {
        "full_send",
        "crush_hiccups",
        "sine_smear",
        "fuzz_bloom",
    };
    for (uint8_t slot = 0; slot < stagePresetSlotCount(); ++slot) {
        DirtStackInfo info{};
        TEST_ASSERT_EQUAL_STRING(expected_ids[slot], factoryStagePresetStackId(slot));
        TEST_ASSERT_TRUE(curatedDirtStackById(expected_ids[slot], &info));
        TEST_ASSERT_EQUAL_UINT8(info.mask, stagePresetMask(slot));
    }

    DirtStackInfo catalog_only{};
    TEST_ASSERT_TRUE(curatedDirtStackById("stutter_gate", &catalog_only));
    for (uint8_t slot = 0; slot < stagePresetSlotCount(); ++slot) {
        TEST_ASSERT_NOT_EQUAL_UINT8(catalog_only.mask, stagePresetMask(slot));
    }

    uint8_t start_index = currentStagePresetIndex();
    selectNextStagePreset();
    TEST_ASSERT_NOT_EQUAL(start_index, currentStagePresetIndex());

    uint8_t before = stagePresetMask(0);
    TEST_ASSERT_FALSE(storeStagePreset(0, 0, false, false));
    TEST_ASSERT_EQUAL_UINT8(before, stagePresetMask(0));
}

void test_stage_preset_serial_save_named_stages() {
    reset_presets_state();
    dice_loop_stub::clear_serial_output();
    dice_loop_stub::push_serial_input("preset save 2 bit_crush fuzz\n");

    pollStagePresetSerial();

    uint8_t expected =
        dirtStageBit(DirtStage::BitCrush) | dirtStageBit(DirtStage::Fuzz);
    TEST_ASSERT_EQUAL_UINT8(2, currentStagePresetIndex());
    TEST_ASSERT_EQUAL_UINT8(expected, stagePresetMask(2));
    TEST_ASSERT_TRUE((getActiveDirtStages() & expected) == expected);
    TEST_ASSERT_NOT_EQUAL(std::string::npos,
                          dice_loop_stub::serial_output_text().find("[presets] slot 2 saved"));
}

void test_stage_preset_serial_rejects_bad_mask() {
    reset_presets_state();
    uint8_t before = stagePresetMask(1);
    dice_loop_stub::clear_serial_output();
    dice_loop_stub::push_serial_input("preset mask 1 nope\n");

    pollStagePresetSerial();

    TEST_ASSERT_EQUAL_UINT8(before, stagePresetMask(1));
    TEST_ASSERT_NOT_EQUAL(std::string::npos,
                          dice_loop_stub::serial_output_text().find("[presets] bad mask value"));
}

void test_stage_preset_serial_lists_slots() {
    reset_presets_state();
    dice_loop_stub::clear_serial_output();
    dice_loop_stub::push_serial_input("preset list\n");

    pollStagePresetSerial();

    const std::string &output = dice_loop_stub::serial_output_text();
    TEST_ASSERT_NOT_EQUAL(std::string::npos, output.find("[presets] slots"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, output.find("0* factory:full_send"));
}

void test_stage_preset_serial_rejects_muted_mask_in_production() {
    reset_presets_state();
    uint8_t before = stagePresetMask(1);
    dice_loop_stub::clear_serial_output();
    dice_loop_stub::push_serial_input("preset mask 1 0x0\n");

    pollStagePresetSerial();

    TEST_ASSERT_EQUAL_UINT8(before, stagePresetMask(1));
    TEST_ASSERT_NOT_EQUAL(std::string::npos,
                          dice_loop_stub::serial_output_text().find("[presets] bad mask value"));
}

void test_stage_preset_serial_lists_curated_catalog() {
    reset_presets_state();
    dice_loop_stub::clear_serial_output();
    dice_loop_stub::push_serial_input("preset stack list\n");

    pollStagePresetSerial();

    const std::string &output = dice_loop_stub::serial_output_text();
    TEST_ASSERT_NOT_EQUAL(std::string::npos, output.find("[presets] curated stacks"));
    TEST_ASSERT_NOT_EQUAL(std::string::npos, output.find("stutter_gate"));
}
