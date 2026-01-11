#include <unity.h>

void setUp() {}
void tearDown() {}

// Forward declarations for the test cases in sibling translation units.
void test_internal_tempo_updates_period();
void test_tap_tempo_latches_external_clock();
void test_chaos_disabled_returns_baseline();
void test_chaos_enabled_stays_in_expected_ranges();
void test_stage_preset_defaults_and_store();
void test_dirt_stage_registry_and_masks();
void test_render_samples();

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_internal_tempo_updates_period);
    RUN_TEST(test_tap_tempo_latches_external_clock);
    RUN_TEST(test_chaos_disabled_returns_baseline);
    RUN_TEST(test_chaos_enabled_stays_in_expected_ranges);
    RUN_TEST(test_stage_preset_defaults_and_store);
    RUN_TEST(test_dirt_stage_registry_and_masks);
    RUN_TEST(test_render_samples);
    return UNITY_END();
}
