#include <unity.h>

#include "Arduino.h"
#include "tempo_sync.h"

namespace {
int tempo_listener_calls = 0;
float last_period_ms = 0.0f;
TempoSource last_source = TempoSource::Internal;

void tempo_listener(float period_ms, TempoSource source) {
    tempo_listener_calls++;
    last_period_ms = period_ms;
    last_source = source;
}
}  // namespace

void reset_tempo_state() {
    dice_loop_stub::reset_state();
    setupTempoSync();
    tempo_listener_calls = 0;
    last_period_ms = 0.0f;
    last_source = TempoSource::Internal;
    registerTempoListener(tempo_listener);
}

void test_internal_tempo_updates_period() {
    reset_tempo_state();
    applyPotTempoBase(500.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 500.0f, tempoSyncCurrentPeriodMs());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 120.0f, tempoSyncCurrentBpm());
    TEST_ASSERT_EQUAL(TempoSource::Internal, tempoSyncCurrentSource());
    TEST_ASSERT_EQUAL_INT(1, tempo_listener_calls);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 500.0f, last_period_ms);
    TEST_ASSERT_EQUAL(TempoSource::Internal, last_source);
}

void test_tap_tempo_latches_external_clock() {
    reset_tempo_state();
    dice_loop_stub::set_digital_pin(6, HIGH);
    dice_loop_stub::set_millis(1000);
    updateTempoSync();

    dice_loop_stub::set_digital_pin(6, LOW);
    updateTempoSync();

    dice_loop_stub::set_digital_pin(6, HIGH);
    updateTempoSync();

    dice_loop_stub::advance_millis(500);
    dice_loop_stub::set_digital_pin(6, LOW);
    updateTempoSync();

    TEST_ASSERT_TRUE(tempoSyncHasExternalClock());
    TEST_ASSERT_EQUAL(TempoSource::Tap, tempoSyncCurrentSource());
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 500.0f, tempoSyncCurrentPeriodMs());

    dice_loop_stub::advance_millis(125);
    float progress = tempoSyncPulseProgress();
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.25f, progress);
}
