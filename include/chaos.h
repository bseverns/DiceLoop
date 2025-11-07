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
};

void setupChaos(); // call once from setup() to initialise chaos utilities

bool chaosModulatorsEnabled();
bool setChaosModulatorsEnabled(bool enabled);
bool toggleChaosModulators();

ChaosSnapshot updateChaosModulators(float densityNorm, float noiseNorm,
                                    int samplesPerBlock);

ChaosSnapshot latestChaosSnapshot(); // last modulation block fed to the audio pipeline

#endif

