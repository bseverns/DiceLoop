// Chaos subsystem public API placeholder.
//
// Keeping this header separate lets future modulators stay decoupled from the
// audio/control headers until they actually need to exchange data.
#ifndef CHAOS_H
#define CHAOS_H

struct ChaosSnapshot {
  float mixOffset;      // additive offset applied to the wet/dry crossfade
  float feedbackOffset; // additive offset applied to the feedback mixer gain
  float fuzzGain;       // multiplier applied to the fuzz amount inside processDirt
  float bloomDepthOffset;   // additive offset applied to bloomAmount (0..1 range)
  float secondaryVoicePan;  // -1..1 pan applied to the ghost voice crossfeed
  float bloomLimiterGain;   // multiplier applied to bloom limiter intensity
  float secondaryFeedbackOffset; // additive offset applied to ghost feedback level
};

void setupChaos(); // call once from setup() to initialise chaos utilities

bool chaosModulatorsEnabled();
bool setChaosModulatorsEnabled(bool enabled);
bool toggleChaosModulators();

ChaosSnapshot updateChaosModulators(float densityNorm, float noiseNorm,
                                    int samplesPerBlock);

ChaosSnapshot latestChaosSnapshot(); // last modulation block fed to the audio pipeline

#endif

