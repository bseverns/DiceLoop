#include <unity.h>

#include "Arduino.h"
#include "chaos.h"

void reset_chaos_state() {
    dice_loop_stub::reset_state();
    setupChaos();
}

void test_chaos_disabled_returns_baseline() {
    reset_chaos_state();
    ChaosSnapshot snap = updateChaosModulators(0.5f, 0.5f, 128);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, snap.mixOffset);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, snap.feedbackOffset);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, snap.fuzzGain);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, snap.bloomDepthOffset);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, snap.secondaryVoicePan);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, snap.bloomLimiterGain);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, snap.secondaryFeedbackOffset);
}

void test_chaos_enabled_stays_in_expected_ranges() {
    reset_chaos_state();
    setChaosModulatorsEnabled(true);
    randomSeed(42);

    ChaosSnapshot snap = updateChaosModulators(0.5f, 0.5f, 128);

    TEST_ASSERT_TRUE(snap.mixOffset >= -0.15f && snap.mixOffset <= 0.15f);
    TEST_ASSERT_TRUE(snap.feedbackOffset >= -0.4f && snap.feedbackOffset <= 0.4f);
    TEST_ASSERT_TRUE(snap.fuzzGain >= 0.8f && snap.fuzzGain <= 1.2f);
    TEST_ASSERT_TRUE(snap.bloomDepthOffset >= -0.45f && snap.bloomDepthOffset <= 0.45f);
    TEST_ASSERT_TRUE(snap.secondaryVoicePan >= -0.95f && snap.secondaryVoicePan <= 0.95f);
    TEST_ASSERT_TRUE(snap.bloomLimiterGain >= 0.65f && snap.bloomLimiterGain <= 1.5f);
    TEST_ASSERT_TRUE(snap.secondaryFeedbackOffset >= -0.35f && snap.secondaryFeedbackOffset <= 0.35f);
}
