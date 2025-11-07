// Chaos utilities namespace.
//
// The "chaos" concept started life as a couple of globals in controls.cpp. This
// translation unit now houses optional modulation sources that can bend the
// audio engine without forcing every build to pay for the overhead. Think of it
// as a sketchbook for Lorenz attractors, LFO swarms, or whatever other math you
// want to throw at the feedback loop.
#include "chaos.h"
#include <Arduino.h>
#include <AudioStream.h>
#include <cmath>

namespace {
bool modulatorsEnabled = false;
float mixLfoPhase = 0.0f;
float fuzzLfoPhase = 0.0f;
float logisticState = 0.37f; // keep it away from 0/1 so the map keeps moving
ChaosSnapshot lastSnapshot{0.0f, 0.0f, 1.0f};

constexpr float twoPi = 2.0f * PI;

float wrapPhase(float phase) {
  while (phase > twoPi) {
    phase -= twoPi;
  }
  while (phase < 0.0f) {
    phase += twoPi;
  }
  return phase;
}
} // namespace

void setupChaos() {
  // Placeholder banner so you know the subsystem initialised. Leave the mod
  // engines disabled by default; you opt-in via the front-panel button chord or
  // by calling setChaosModulatorsEnabled(true) from your own code.
  modulatorsEnabled = false;
  Serial.println("[chaos] subsystem armed – modulators idle");
}

bool chaosModulatorsEnabled() { return modulatorsEnabled; }

bool setChaosModulatorsEnabled(bool enabled) {
  modulatorsEnabled = enabled;
  return modulatorsEnabled;
}

bool toggleChaosModulators() {
  modulatorsEnabled = !modulatorsEnabled;
  return modulatorsEnabled;
}

ChaosSnapshot updateChaosModulators(float densityNorm, float noiseNorm,
                                    int samplesPerBlock) {
  // Default offsets keep the engine honest even when modulators are bypassed;
  // mix/feedback stay at whatever the performer dialled in while fuzz gain sits
  // at unity. The early return caches that baseline so the UI can render it too.
  ChaosSnapshot snapshot{0.0f, 0.0f, 1.0f};
  if (!modulatorsEnabled || samplesPerBlock <= 0) {
    lastSnapshot = snapshot;
    return snapshot;
  }

  // Convert block size into seconds. The Teensy audio stack runs at
  // AUDIO_SAMPLE_RATE_EXACT, so each block equals `samplesPerBlock / SR`.
  const float blockDuration =
      static_cast<float>(samplesPerBlock) / AUDIO_SAMPLE_RATE_EXACT;

  // Mix LFO: density pot steers the speed so busy glitch textures sway faster.
  const float mixRateHz = 0.08f + densityNorm * 1.8f;
  mixLfoPhase = wrapPhase(mixLfoPhase + twoPi * mixRateHz * blockDuration);
  snapshot.mixOffset = 0.15f * sinf(mixLfoPhase);

  // Feedback chaos: logistic map hugs 0..1, then we recentre and scale. Noise
  // pot pushes the map towards the edge of stability while density weights the
  // output. A sprinkle of random jitter keeps it from repeating cycles.
  const float logisticMu = constrain(3.4f + noiseNorm * 0.45f + densityNorm * 0.15f,
                                     3.2f, 3.95f);
  logisticState = logisticMu * logisticState * (1.0f - logisticState);
  if (logisticState <= 0.0001f || logisticState >= 0.9999f) {
    logisticState = 0.37f; // yank it back into the interesting zone
  }
  const float centred = logisticState - 0.5f;
  const float jitter = (static_cast<float>(random(-32768, 32767)) / 32767.0f) *
                       0.05f * densityNorm;
  snapshot.feedbackOffset =
      constrain((centred * (0.45f + densityNorm * 0.35f)) + jitter, -0.4f, 0.4f);

  // Fuzz engine wobble: tied mostly to the noise pot so harmonic grit pulses in
  // sympathy with how gnarly the player dials things in.
  const float fuzzRateHz = 0.25f + noiseNorm * 6.0f;
  fuzzLfoPhase = wrapPhase(fuzzLfoPhase + twoPi * fuzzRateHz * blockDuration);
  snapshot.fuzzGain = 1.0f + 0.2f * sinf(fuzzLfoPhase);

  lastSnapshot = snapshot;
  return snapshot;
}

ChaosSnapshot latestChaosSnapshot() { return lastSnapshot; }

#ifdef __CPPCHECK__
// Provide a synthetic touchpoint so cppcheck acknowledges the entry point.
[[maybe_unused]] static void __cppcheck_chaos_reference() {
  setupChaos();
  setChaosModulatorsEnabled(true);
  (void)updateChaosModulators(0.5f, 0.5f, AUDIO_BLOCK_SAMPLES);
  toggleChaosModulators();
}
[[maybe_unused]] static const auto __cppcheck_chaos_anchor =
    (__cppcheck_chaos_reference(), 0);
#endif

