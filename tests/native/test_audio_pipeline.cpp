#include <unity.h>

#include "Arduino.h"
#include "audio_pipeline.h"
#include "chaos.h"
#include "controls.h"

namespace {

void reset_audio_path_state();

void fill_block(int16_t *buffer, int16_t value) {
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
        buffer[i] = value;
    }
}

int render_left_wet_output_sample(int16_t dirtyValue, float bloomDepth) {
    reset_audio_path_state();

    mixAmount = 1.0f;
    bloomAmount = bloomDepth;

    int16_t clean_left[AUDIO_BLOCK_SAMPLES];
    int16_t dirty_left[AUDIO_BLOCK_SAMPLES];

    fill_block(clean_left, 0);
    fill_block(dirty_left, dirtyValue);

    cleanQueueL.pushBuffer(clean_left);
    queueL.pushBuffer(dirty_left);

    processAudioQueues();

    TEST_ASSERT_TRUE(outputQueueL.hasPlayedBuffer());
    return std::abs(static_cast<int>(outputQueueL.lastPlayedBuffer()[0]));
}

void reset_audio_path_state() {
    dice_loop_stub::reset_state();
    setupAudioPipeline();
    setupChaos();
    setChaosModulatorsEnabled(false);
    setActiveDirtStages(0);
    resetDirtStateForTest();

    mixAmount = 0.25f;
    macroMixOverride = -1.0f;
    macroWetBias = 0.0f;
    secondaryVoiceLevel = 0.0f;
    bloomAmount = 0.0f;
    bloomFeedbackBoost = 0.0f;
    feedbackAmount = 0.0f;
    noiseAmount = 0;
    density = 0;

    queueL.clearBuffers();
    queueR.clearBuffers();
    cleanQueueL.clearBuffers();
    cleanQueueR.clearBuffers();
    outputQueueL.clearPlayedBuffer();
    outputQueueR.clearPlayedBuffer();
}

}  // namespace

void test_process_audio_queues_blends_clean_and_dirty_blocks() {
    reset_audio_path_state();

    int16_t clean_left[AUDIO_BLOCK_SAMPLES];
    int16_t dirty_left[AUDIO_BLOCK_SAMPLES];
    int16_t clean_right[AUDIO_BLOCK_SAMPLES];
    int16_t dirty_right[AUDIO_BLOCK_SAMPLES];

    fill_block(clean_left, 10000);
    fill_block(dirty_left, 20000);
    fill_block(clean_right, -12000);
    fill_block(dirty_right, 4000);

    cleanQueueL.pushBuffer(clean_left);
    queueL.pushBuffer(dirty_left);
    cleanQueueR.pushBuffer(clean_right);
    queueR.pushBuffer(dirty_right);

    processAudioQueues();

    TEST_ASSERT_TRUE(outputQueueL.hasPlayedBuffer());
    TEST_ASSERT_TRUE(outputQueueR.hasPlayedBuffer());
    TEST_ASSERT_INT16_WITHIN(1, 12500, outputQueueL.lastPlayedBuffer()[0]);
    TEST_ASSERT_INT16_WITHIN(1, -8000, outputQueueR.lastPlayedBuffer()[0]);
    TEST_ASSERT_FALSE(queueL.available());
    TEST_ASSERT_FALSE(cleanQueueL.available());
    TEST_ASSERT_FALSE(queueR.available());
    TEST_ASSERT_FALSE(cleanQueueR.available());
}

void test_process_audio_queues_applies_ghost_crossfeed() {
    reset_audio_path_state();

    mixAmount = 1.0f;
    secondaryVoiceLevel = 1.0f;

    int16_t clean_left[AUDIO_BLOCK_SAMPLES];
    int16_t dirty_left[AUDIO_BLOCK_SAMPLES];
    int16_t clean_right[AUDIO_BLOCK_SAMPLES];
    int16_t dirty_right[AUDIO_BLOCK_SAMPLES];

    fill_block(clean_left, 0);
    fill_block(dirty_left, 20000);
    fill_block(clean_right, 0);
    fill_block(dirty_right, -10000);

    cleanQueueL.pushBuffer(clean_left);
    queueL.pushBuffer(dirty_left);
    cleanQueueR.pushBuffer(clean_right);
    queueR.pushBuffer(dirty_right);

    processAudioQueues();

    TEST_ASSERT_TRUE(outputQueueL.hasPlayedBuffer());
    TEST_ASSERT_TRUE(outputQueueR.hasPlayedBuffer());
    TEST_ASSERT_INT16_WITHIN(1, 5000, outputQueueL.lastPlayedBuffer()[0]);
    TEST_ASSERT_INT16_WITHIN(1, 5000, outputQueueR.lastPlayedBuffer()[0]);
}

void test_process_audio_queues_bloom_compresses_dynamic_range() {
    const int quietLinear = render_left_wet_output_sample(6000, 0.0f);
    const int loudLinear = render_left_wet_output_sample(24000, 0.0f);
    const int quietBloom = render_left_wet_output_sample(6000, 0.2f);
    const int loudBloom = render_left_wet_output_sample(24000, 0.2f);

    TEST_ASSERT_TRUE(loudLinear > quietLinear * 3);
    TEST_ASSERT_TRUE(loudBloom < quietBloom * 3);
    TEST_ASSERT_TRUE(loudBloom <= 32767);
}

void test_process_audio_queues_macro_dry_override_bypasses_wet_path() {
    reset_audio_path_state();

    mixAmount = 1.0f;
    macroMixOverride = 0.0f;
    secondaryVoiceLevel = 1.0f;
    bloomAmount = 0.8f;

    int16_t clean_left[AUDIO_BLOCK_SAMPLES];
    int16_t dirty_left[AUDIO_BLOCK_SAMPLES];
    int16_t clean_right[AUDIO_BLOCK_SAMPLES];
    int16_t dirty_right[AUDIO_BLOCK_SAMPLES];

    fill_block(clean_left, 7000);
    fill_block(dirty_left, 28000);
    fill_block(clean_right, -9000);
    fill_block(dirty_right, 18000);

    cleanQueueL.pushBuffer(clean_left);
    queueL.pushBuffer(dirty_left);
    cleanQueueR.pushBuffer(clean_right);
    queueR.pushBuffer(dirty_right);

    processAudioQueues();

    TEST_ASSERT_TRUE(outputQueueL.hasPlayedBuffer());
    TEST_ASSERT_TRUE(outputQueueR.hasPlayedBuffer());
    TEST_ASSERT_INT16_WITHIN(1, 7000, outputQueueL.lastPlayedBuffer()[0]);
    TEST_ASSERT_INT16_WITHIN(1, -9000, outputQueueR.lastPlayedBuffer()[0]);
}
