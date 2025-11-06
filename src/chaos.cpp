// Chaos utilities namespace.
//
// Right now the "chaos" concept lives mostly inside controls.cpp. This file is
// a staging area for any modulation sources that need to tick over time (LFOs,
// Lorenz attractors, etc.). By keeping the scaffold ready we lower the barrier
// to experiment later without bloating main.cpp.
#include "chaos.h"
#include <Arduino.h>

void setupChaos() {
  // Placeholder for chaos LFO or other modulation sources. At boot we simply
  // spit a debug banner so you know the subsystem initialised; comment it out on
  // production builds if noise bothers you.
  Serial.println("[chaos] subsystem armed - add modulators in chaos.cpp");
}

#ifdef __CPPCHECK__
// Provide a synthetic touchpoint so cppcheck acknowledges the entry point.
[[maybe_unused]] static void __cppcheck_chaos_reference() {
  setupChaos();
}
[[maybe_unused]] static const auto __cppcheck_chaos_anchor =
    (__cppcheck_chaos_reference(), 0);
#endif

