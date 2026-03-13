#include <unity.h>

void setUp() {}
void tearDown() {}

// Forward declarations for the test cases in sibling translation units.
void test_internal_tempo_updates_period();
void test_tap_tempo_latches_external_clock();
void test_usb_midi_clock_latches_external_clock();
void test_reseed_short_press_increments_chaos_level();
void test_reset_short_press_clears_chaos_level();
void test_reseed_hold_cycles_stage_preset();
void test_dual_button_chord_toggles_modulators_and_tempo_lock();
void test_process_audio_queues_blends_clean_and_dirty_blocks();
void test_process_audio_queues_applies_ghost_crossfeed();
void test_process_audio_queues_bloom_compresses_dynamic_range();
void test_process_audio_queues_macro_dry_override_bypasses_wet_path();
void test_chaos_disabled_returns_baseline();
void test_chaos_enabled_stays_in_expected_ranges();
void test_stage_preset_defaults_and_store();
void test_stage_preset_serial_save_named_stages();
void test_stage_preset_serial_rejects_bad_mask();
void test_stage_preset_serial_lists_slots();
void test_stage_preset_serial_rejects_muted_mask_in_production();
void test_stage_preset_serial_lists_curated_catalog();
void test_dirt_stage_registry_and_masks();
void test_render_samples();

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_internal_tempo_updates_period);
    RUN_TEST(test_tap_tempo_latches_external_clock);
    RUN_TEST(test_usb_midi_clock_latches_external_clock);
    RUN_TEST(test_reseed_short_press_increments_chaos_level);
    RUN_TEST(test_reset_short_press_clears_chaos_level);
    RUN_TEST(test_reseed_hold_cycles_stage_preset);
    RUN_TEST(test_dual_button_chord_toggles_modulators_and_tempo_lock);
    RUN_TEST(test_process_audio_queues_blends_clean_and_dirty_blocks);
    RUN_TEST(test_process_audio_queues_applies_ghost_crossfeed);
    RUN_TEST(test_process_audio_queues_bloom_compresses_dynamic_range);
    RUN_TEST(test_process_audio_queues_macro_dry_override_bypasses_wet_path);
    RUN_TEST(test_chaos_disabled_returns_baseline);
    RUN_TEST(test_chaos_enabled_stays_in_expected_ranges);
    RUN_TEST(test_stage_preset_defaults_and_store);
    RUN_TEST(test_stage_preset_serial_save_named_stages);
    RUN_TEST(test_stage_preset_serial_rejects_bad_mask);
    RUN_TEST(test_stage_preset_serial_lists_slots);
    RUN_TEST(test_stage_preset_serial_rejects_muted_mask_in_production);
    RUN_TEST(test_stage_preset_serial_lists_curated_catalog);
    RUN_TEST(test_dirt_stage_registry_and_masks);
    RUN_TEST(test_render_samples);
    return UNITY_END();
}
