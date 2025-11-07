// Audio processing pipeline for the chaos delay.
//
// The Teensy Audio Library builds a directed graph of `AudioStream` objects.
// The graph below mirrors the signal flow ASCII art in README.md. Each global
// object represents a node; the `AudioConnection` instances form the edges.
//
// ┌───────────────┐    ┌──────────────┐    ┌─────────────┐
// │ AudioInputI2S │───►│ filter1 (SVF)│───►│ feedbackMixer│──┐
// └───────────────┘    └──────┬───────┘    └────┬────────┘  │
//                              │                 │           │
//                              ▼                 │           │
//                     cleanQueueL/R (tap)        │           │
//                              │                 │           │
//                              ▼                 │           │
//                       delay1 (stereo taps)     │◄──────────┘
//                              │
//                              ▼
//                        queueL/queueR
//                              │
//                              ▼
//                   outputQueueL/R → i2sOut
//
// The helper functions in this file set up the graph and then manually mix the
// queue buffers so we can sprinkle in probabilistic bit crushing.
#include "audio_pipeline.h"
#include "Arduino.h"
#include "chaos.h"
#include "controls.h"
#include <cmath>

// === Audio Objects ===
AudioInputI2S          i2sIn;
AudioEffectDelay       delay1;
AudioFilterStateVariable filter1;
AudioRecordQueue       queueL, queueR;
AudioRecordQueue       cleanQueueL, cleanQueueR;
AudioPlayQueue         outputQueueL, outputQueueR;
AudioOutputI2S         i2sOut;
AudioMixer4           feedbackMixer;

// `feedbackAmount` mirrors the front-panel feedback pot and is shared with the
// controls module. It is applied as gain on `feedbackMixer` input 1.
float feedbackAmount = 0.0f;

// === Audio Connections ===
AudioConnection patchCord1(i2sIn, 0, filter1, 0);
AudioConnection patchCord2(filter1, 0, feedbackMixer, 0);
AudioConnection patchCord3(delay1, 0, feedbackMixer, 1);
AudioConnection patchCord4(feedbackMixer, 0, delay1, 0);
AudioConnection patchCord5(filter1, 0, cleanQueueL, 0);
AudioConnection patchCord6(filter1, 0, cleanQueueR, 0);
AudioConnection patchCord7(delay1, 0, queueL, 0);
AudioConnection patchCord8(delay1, 1, queueR, 0);
AudioConnection patchCord9(outputQueueL, 0, i2sOut, 0);
AudioConnection patchCord10(outputQueueR, 0, i2sOut, 1);

// Ratio of dirty (post-delay) to clean (pre-delay) signal. 0 = dry, 1 = fully
// crushed chaos.
float mixAmount = 0.5f;

namespace {
// State for the dynamic "dirt engine" cluster. All knobs steer these modulators
// so the grit feels alive instead of binary on/off switches.
float tremPhase = 0.0f;
float foldMemory = 0.0f;
float heldSample = 0.0f;
int holdCountdown = 0;

// Block-scope modulation shared across the current audio buffers. `processDirt`
// reads these so the optional chaos modulators can warp parameters without
// adding more arguments or globals elsewhere.
float blockMixAmount = 0.5f;
float blockFuzzScale = 1.0f;
float currentDensityNorm = 0.0f;
float currentNoiseNorm = 0.0f;

constexpr float twoPi = 2.0f * PI;

float applyBitCrush(float sample, float noiseNorm) {
  // Map 0..1 → 8..2 bits of resolution. Rounding keeps transitions smooth when
  // the knob jitters or the chaos ladder nudges `noiseAmount`.
  int crushBits = static_cast<int>(roundf(2.0f + (1.0f - noiseNorm) * 6.0f));
  crushBits = constrain(crushBits, 2, 8);
  int steps = 1 << crushBits;
  int crushed = static_cast<int>(sample * steps);
  return static_cast<float>(crushed) / steps;
}

float applyWaveFold(float sample, float noiseNorm) {
  // A sine fold keeps things musical while still adding chaos. `foldMemory`
  // smears the movement so tiny control changes translate into chewy motion.
  float drive = 1.0f + noiseNorm * 5.0f;
  float folded = sinf(sample * drive * PI);
  foldMemory = 0.92f * foldMemory + 0.08f * folded;
  return constrain(folded + 0.5f * foldMemory, -1.0f, 1.0f);
}

float applyStutter(float sample, float densityNorm) {
  // Sample-and-hold creates rhythmic chokes. Higher density → faster retrigs;
  // lower density → longer freezes that feel more like tape stoppages.
  int window = 3 + static_cast<int>((1.0f - densityNorm) * 160.0f);
  if (--holdCountdown <= 0) {
    holdCountdown = window;
    heldSample = sample;
  }
  float freezeBlend = densityNorm * densityNorm; // gentle curve, 0..1.
  return heldSample * freezeBlend + sample * (1.0f - freezeBlend);
}

float nextTremoloGain(float densityNorm) {
  // Chaotic tremolo that gets quicker as density rises. Depth stays shallow so
  // it feels like motion rather than muting.
  float rate = 0.35f + densityNorm * 7.5f; // Hz
  tremPhase += twoPi * (rate / AUDIO_SAMPLE_RATE_EXACT);
  if (tremPhase > twoPi) {
    tremPhase -= twoPi;
  }
  return 0.75f + 0.25f * sinf(tremPhase);
}
} // namespace

float processDirt(float sample) {
  // Apply glitch only on a percentage of samples defined by `density`. The rest
  // sail through untouched so the delay never loses its sense of pulse.
  if (random(100) >= density) {
    return sample;
  }

  float densityNorm = currentDensityNorm;
  float noiseNorm = currentNoiseNorm;

  float crushed = applyBitCrush(sample, noiseNorm);
  float folded = applyWaveFold(sample, noiseNorm);
  float stuttered = applyStutter(crushed, densityNorm);

  // Density leans into the rhythmic stutter, noise controls harmonic brutality.
  float toneBlend = (1.0f - noiseNorm) * crushed + noiseNorm * folded;
  float rhythmBlend = (1.0f - densityNorm) * toneBlend + densityNorm * stuttered;

  // Sprinkle controlled fuzz so even static notes evolve. Noise knob sets how
  // hairy the fuzz gets, density dictates how often new grains are minted.
  float fuzz = static_cast<float>(random(-32768, 32767)) / 32767.0f;
  float fuzzAmount = (0.08f + 0.42f * noiseNorm) * (0.4f + 0.6f * densityNorm);
  fuzzAmount *= blockFuzzScale;
  float result = rhythmBlend + fuzz * fuzzAmount;

  result *= nextTremoloGain(densityNorm);
  return constrain(result, -1.0f, 1.0f);
}

void processAudioQueues() {
  bool leftReady = queueL.available() && cleanQueueL.available();
  bool rightReady = queueR.available() && cleanQueueR.available();
  if (!leftReady && !rightReady) {
    return;
  }

  currentDensityNorm = constrain(density / 100.0f, 0.0f, 1.0f);
  currentNoiseNorm = constrain(noiseAmount / 60.0f, 0.0f, 1.0f);

  ChaosSnapshot chaosSnapshot =
      updateChaosModulators(currentDensityNorm, currentNoiseNorm,
                            AUDIO_BLOCK_SAMPLES);
  blockMixAmount = constrain(mixAmount + chaosSnapshot.mixOffset, 0.0f, 1.0f);
  blockFuzzScale = chaosSnapshot.fuzzGain;

  if (chaosModulatorsEnabled()) {
    float modFeedback =
        constrain(feedbackAmount + chaosSnapshot.feedbackOffset, 0.0f, 0.99f);
    setFeedbackGain(1, modFeedback);
  } else {
    setFeedbackGain(1, feedbackAmount);
  }

  if (leftReady) {
    // Mix left channel from clean and dirty delay buffers. Audio blocks are
    // 128-sample chunks. Teensy represents them as int16_t where ±32767 equals
    // ±1.0f. We convert to floats for clarity then convert back.
    audio_block_t *dirty = queueL.readBuffer();
    audio_block_t *clean = cleanQueueL.readBuffer();
    audio_block_t *outBlock = outputQueueL.getBuffer();
    if (!outBlock) {
      queueL.freeBuffer();
      cleanQueueL.freeBuffer();
    } else {
      for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float c = static_cast<float>(clean->data[i]) / 32768.0f;
        float d = static_cast<float>(dirty->data[i]) / 32768.0f;
        // Apply dirt and blend with clean signal.
        d = processDirt(d);
        float mixed = (1.0f - blockMixAmount) * c + blockMixAmount * d;
        mixed = constrain(mixed, -1.0f, 1.0f);
        outBlock->data[i] = static_cast<int16_t>(mixed * 32767.0f);
      }
      outputQueueL.playBuffer(outBlock);
      queueL.freeBuffer();
      cleanQueueL.freeBuffer();
    }
  }

  if (rightReady) {
    // Repeat the same dance for the right channel.
    audio_block_t *dirty = queueR.readBuffer();
    audio_block_t *clean = cleanQueueR.readBuffer();
    audio_block_t *outBlock = outputQueueR.getBuffer();
    if (!outBlock) {
      queueR.freeBuffer();
      cleanQueueR.freeBuffer();
    } else {
      for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float c = static_cast<float>(clean->data[i]) / 32768.0f;
        float d = static_cast<float>(dirty->data[i]) / 32768.0f;
        // Apply dirt and blend with clean signal.
        d = processDirt(d);
        float mixed = (1.0f - blockMixAmount) * c + blockMixAmount * d;
        mixed = constrain(mixed, -1.0f, 1.0f);
        outBlock->data[i] = static_cast<int16_t>(mixed * 32767.0f);
      }
      outputQueueR.playBuffer(outBlock);
      queueR.freeBuffer();
      cleanQueueR.freeBuffer();
    }
  }
}

void setupAudioPipeline() {
  // Reserve audio memory buffers. Each block equals 128 samples, so 60 blocks
  // gives the delay ample breathing room without starving the mixer.
  AudioMemory(60);

  // Configure delay and filter defaults. `delay1.delay(0, x)` sets tap 0 (left)
  // to x milliseconds. The right channel uses the library default and is
  // modulated via the same API if desired.
  delay1.delay(0, 200);
  filter1.frequency(500);
  filter1.resonance(0.7);

  setFeedbackGain(0, 1.0f);
  setFeedbackGain(1, feedbackAmount);

  queueL.begin();
  queueR.begin();
  cleanQueueL.begin();
  cleanQueueR.begin();
}

#ifdef __CPPCHECK__
// When cppcheck analyses this translation unit in isolation it misses the calls
// from main.cpp. We provide an explicit reference hook so the analyzer can see
// the control flow without relying on comments or suppressions.
[[maybe_unused]] static void __cppcheck_audio_pipeline_reference() {
  setupAudioPipeline();
  processAudioQueues();
  (void)processDirt(0.0f);
}
[[maybe_unused]] static const auto __cppcheck_audio_pipeline_anchor =
    (__cppcheck_audio_pipeline_reference(), 0);
#endif

