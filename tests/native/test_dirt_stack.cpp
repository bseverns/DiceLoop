#include <unity.h>

#include "audio_pipeline.h"

void test_dirt_stage_registry_and_masks() {
    setActiveDirtStages(0xFF);
    TEST_ASSERT_EQUAL_UINT32(4u, static_cast<unsigned int>(dirtStageCount()));

    DirtStage stage{};
    TEST_ASSERT_TRUE(dirtStageById("fuzz", &stage));
    TEST_ASSERT_EQUAL(DirtStage::Fuzz, stage);

    uint8_t original = getActiveDirtStages();
    enableDirtStage(DirtStage::Fuzz, false);
    TEST_ASSERT_EQUAL_UINT8(original & ~dirtStageBit(DirtStage::Fuzz),
                            getActiveDirtStages());
    enableDirtStageById("fuzz", true);
    TEST_ASSERT_TRUE((getActiveDirtStages() & dirtStageBit(DirtStage::Fuzz)) != 0);

    DirtStackInfo stack{};
    TEST_ASSERT_TRUE(curatedDirtStackById("full_send", &stack));
    TEST_ASSERT_TRUE(stack.mask != 0);
}
