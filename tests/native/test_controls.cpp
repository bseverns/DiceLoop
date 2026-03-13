#include <unity.h>

#include "Arduino.h"
#include "chaos.h"
#include "controls.h"
#include "stage_presets.h"
#include "audio_pipeline.h"

namespace {

constexpr unsigned long debounce_settle_ms = 60UL;

void prime_control_surface() {
    dice_loop_stub::set_analog_pin(A0, 0);
    dice_loop_stub::set_analog_pin(A1, 0);
    dice_loop_stub::set_analog_pin(A3, 0);
    dice_loop_stub::set_analog_pin(A4, 0);
    dice_loop_stub::set_analog_pin(A5, 0);
    dice_loop_stub::set_digital_pin(7, HIGH);
    dice_loop_stub::set_digital_pin(8, HIGH);
}

void settle_button(uint8_t pin, int value) {
    dice_loop_stub::set_digital_pin(pin, value);
    updateControl();
    dice_loop_stub::advance_millis(debounce_settle_ms);
    updateControl();
}

void reset_controls_state() {
    dice_loop_stub::reset_state();
    prime_control_surface();
    setupControls();
    setupChaos();
    setupStagePresets();
    loadStagePreset(0, false);
    setChaosModulatorsEnabled(false);
    setStutterTimingMode(StutterTimingMode::Probability);
}

}  // namespace

void test_reseed_short_press_increments_chaos_level() {
    reset_controls_state();
    TEST_ASSERT_EQUAL_INT(0, currentChaosLevel());

    settle_button(8, LOW);
    settle_button(8, HIGH);

    TEST_ASSERT_EQUAL_INT(1, currentChaosLevel());
}

void test_reset_short_press_clears_chaos_level() {
    reset_controls_state();

    settle_button(8, LOW);
    settle_button(8, HIGH);
    TEST_ASSERT_EQUAL_INT(1, currentChaosLevel());

    settle_button(7, LOW);
    settle_button(7, HIGH);

    TEST_ASSERT_EQUAL_INT(0, currentChaosLevel());
}

void test_reseed_hold_cycles_stage_preset() {
    reset_controls_state();
    TEST_ASSERT_TRUE(loadStagePreset(0, false));
    TEST_ASSERT_EQUAL_UINT8(0, currentStagePresetIndex());

    settle_button(8, LOW);
    dice_loop_stub::advance_millis(600UL);
    updateControl();

    TEST_ASSERT_EQUAL_UINT8(1, currentStagePresetIndex());

    settle_button(8, HIGH);
}

void test_dual_button_chord_toggles_modulators_and_tempo_lock() {
    reset_controls_state();
    TEST_ASSERT_FALSE(chaosModulatorsEnabled());
    TEST_ASSERT_EQUAL(StutterTimingMode::Probability, stutterTimingMode());

    dice_loop_stub::set_digital_pin(7, LOW);
    dice_loop_stub::set_digital_pin(8, LOW);
    updateControl();
    dice_loop_stub::advance_millis(debounce_settle_ms);
    updateControl();

    TEST_ASSERT_TRUE(chaosModulatorsEnabled());
    TEST_ASSERT_EQUAL(StutterTimingMode::Probability, stutterTimingMode());

    dice_loop_stub::advance_millis(1200UL);
    updateControl();

    TEST_ASSERT_EQUAL(StutterTimingMode::TempoLocked, stutterTimingMode());

    dice_loop_stub::set_digital_pin(7, HIGH);
    dice_loop_stub::set_digital_pin(8, HIGH);
    updateControl();
    dice_loop_stub::advance_millis(debounce_settle_ms);
    updateControl();

    TEST_ASSERT_EQUAL_INT(0, currentChaosLevel());
}
