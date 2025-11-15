// Audio processing pipeline for the chaos delay.
//
// The Teensy Audio Library builds a directed graph of `AudioStream` objects.
// The graph below mirrors the signal flow ASCII art in README.md. Each global
// object represents a node; the `AudioConnection` instances form the edges.
//
// ┌────────────────┐   Audio Shield I²S   ┌───────────────────────┐
// │ AudioInputI2S  │ ───────────────────► │ filter1 (gentle HPF)  │
// └────────────────┘                      └──────────┬────────────┘
//                                                   │
//                                                   │   clean tap for manual mix
//                                                   ▼
//                                      ┌─────────────────────────────┐
//                                      │ cleanQueueL / cleanQueueR   │
//                                      └────────────┬────────────────┘
//                                                   │
//                                                   ▼
//                                       ┌────────────────────────┐
//                                       │ feedbackMixer (4x1)   │◄────────────┐
//                                       └───────────┬──────────┘             │
//                                                   │                        │
//                                                   ▼                        │
//                                        ┌──────────────────────┐            │
//                                        │ delay1 (stereo taps) │────────────┘
//                                        └────────────┬─────────┘
//                                                     │
//                                                     │  post-delay capture for chaos
//                                                     ▼
//                               ┌────────────────────────────────┐
//                               │ queueL / queueR  (dirty tap)   │
//                               └────────────┬───────────────────┘
//                                            │
//                            chaos modulators│ feed offsets to ↓
//                  reseed/reset ladder & pots │
//                                            ▼
//                  ┌────────────────────────────────────────────────────┐
//                  │ processAudioQueues()                               │
//                  │   ├─ blend clean tap + dirty tap                   │
//                  │   ├─ Chaos Engine: processDirt()                   │
//                  │   │     • bit crush core                           │
//                  │   │     • wavefold smear                           │
//                  │   │     • stutter / hold shards                    │
//                  │   └─ trem/fuzz polish + mix routing                │
//                  └──────────┬─────────────────────────────────────────┘
//                              │
//                              ▼
//                    ┌────────────────────────────┐
//                    │ outputQueueL / outputQueueR│
//                    └────────────┬────────────────┘
//                                  │
//                                  ▼
//                               i2sOut
//
// The helper functions in this file set up the graph and then manually mix the
// queue buffers so we can sprinkle in probabilistic bit crushing.
#include "audio_pipeline.h"
#include "Arduino.h"
#include "chaos.h"
#include "controls.h"
#include <cctype>
#include <cstddef>
#include <cmath>
#include <cstring>

// === Audio Objects ===
AudioInputI2S          i2sIn;
AudioEffectDelay       delay1;
AudioFilterStateVariable filter1;
AudioRecordQueue       queueL, queueR;
AudioRecordQueue       cleanQueueL, cleanQueueR;
AudioPlayQueue         outputQueueL, outputQueueR;
AudioOutputI2S         i2sOut;
audio_compat::Mixer4 feedbackMixer;
AudioControlSGTL5000   audioShield;

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
float macroMixOverride = -1.0f;
float macroWetBias = 0.0f;
float secondaryVoiceLevel = 0.0f;
float bloomAmount = 0.0f;
float bloomFeedbackBoost = 0.0f;

namespace {
StutterTimingMode currentStutterMode = StutterTimingMode::Probability;
float stutterBasePeriodSeconds = 0.5f; // default ≈120 BPM quarter note
int tempoWindowSamples = AUDIO_BLOCK_SAMPLES;
bool tempoWindowDirty = true;

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
float bloomEnvelopeL = 0.0f;
float bloomEnvelopeR = 0.0f;

struct DirtStageContext {
  float input;
  float densityNorm;
  float noiseNorm;
  float fuzzScale;
  float crushed = 0.0f;
  bool crushedActive = false;
  float folded = 0.0f;
  bool foldedActive = false;
  float stuttered = 0.0f;
  bool stutteredActive = false;
  float fuzzContribution = 0.0f;
  bool fuzzActive = false;
};

float applyBitCrush(float sample, float noiseNorm);
float applyWaveFold(float sample, float noiseNorm);
float applyStutter(float sample, float densityNorm);

void runBitCrush(DirtStageContext &ctx) {
  ctx.crushed = applyBitCrush(ctx.input, ctx.noiseNorm);
  ctx.crushedActive = true;
}

void runWaveFold(DirtStageContext &ctx) {
  ctx.folded = applyWaveFold(ctx.input, ctx.noiseNorm);
  ctx.foldedActive = true;
}

void runStutter(DirtStageContext &ctx) {
  float source = ctx.crushedActive ? ctx.crushed : ctx.input;
  ctx.stuttered = applyStutter(source, ctx.densityNorm);
  ctx.stutteredActive = true;
}

void runFuzz(DirtStageContext &ctx) {
  float fuzz = static_cast<float>(random(-32768, 32767)) / 32767.0f;
  float fuzzAmount = (0.08f + 0.42f * ctx.noiseNorm) *
                     (0.4f + 0.6f * ctx.densityNorm);
  fuzzAmount *= ctx.fuzzScale;
  ctx.fuzzContribution = fuzz * fuzzAmount;
  ctx.fuzzActive = true;
}

struct RegisteredDirtStage {
  DirtStage stage;
  const char *id;
  void (*apply)(DirtStageContext &);
};

const RegisteredDirtStage dirtStageRegistry[] = {
    {DirtStage::BitCrush, "bit_crush", runBitCrush},
    {DirtStage::WaveFold, "wave_fold", runWaveFold},
    {DirtStage::Stutter, "stutter", runStutter},
    {DirtStage::Fuzz, "fuzz", runFuzz},
};

static_assert(static_cast<size_t>(DirtStage::Count) ==
                  (sizeof(dirtStageRegistry) / sizeof(dirtStageRegistry[0])),
              "Dirt stage registry mismatch");

constexpr uint8_t kAllDirtStagesMask =
    (1u << static_cast<uint8_t>(DirtStage::Count)) - 1u;

uint8_t activeDirtStageMask = kAllDirtStagesMask;

bool stageIdEquals(const char *lhs, const char *rhs) {
  if (lhs == nullptr || rhs == nullptr) {
    return false;
  }
  while (*lhs && *rhs) {
    if (tolower(static_cast<unsigned char>(*lhs)) !=
        tolower(static_cast<unsigned char>(*rhs))) {
      return false;
    }
    ++lhs;
    ++rhs;
  }
  return *lhs == '\0' && *rhs == '\0';
}

constexpr float twoPi = 2.0f * PI;
constexpr float tempoSubdivisionsBeats[] = {
    4.0f,   // whole note
    3.0f,   // dotted half
    2.0f,   // half
    1.5f,   // dotted quarter
    1.0f,   // quarter
    0.75f,  // dotted eighth
    0.5f,   // eighth
    0.375f, // dotted sixteenth
    0.25f,  // sixteenth
    0.1875f,// dotted thirty-second
    0.125f  // thirty-second
};

int lastTempoSubdivisionIndex = -1;

void refreshTempoLockedWindow(float densityNorm) {
  if (currentStutterMode != StutterTimingMode::TempoLocked) {
    return;
  }

  const int subdivisionCount = sizeof(tempoSubdivisionsBeats) / sizeof(float);
  densityNorm = constrain(densityNorm, 0.0f, 1.0f);
  int subdivisionIndex = static_cast<int>(floorf(densityNorm * subdivisionCount));
  if (subdivisionIndex >= subdivisionCount) {
    subdivisionIndex = subdivisionCount - 1;
  }
  if (subdivisionIndex < 0) {
    subdivisionIndex = 0;
  }

  if (!tempoWindowDirty && subdivisionIndex == lastTempoSubdivisionIndex) {
    return;
  }

  float basePeriodSeconds = stutterBasePeriodSeconds;
  if (basePeriodSeconds <= 0.0f) {
    basePeriodSeconds = static_cast<float>(AUDIO_BLOCK_SAMPLES) /
                        AUDIO_SAMPLE_RATE_EXACT;
  }

  float windowSeconds = tempoSubdivisionsBeats[subdivisionIndex] * basePeriodSeconds;
  const float blockDuration = static_cast<float>(AUDIO_BLOCK_SAMPLES) /
                              AUDIO_SAMPLE_RATE_EXACT;
  int blocks = static_cast<int>(roundf(windowSeconds / blockDuration));
  if (blocks < 1) {
    blocks = 1;
  }
  int newTempoWindowSamples = blocks * AUDIO_BLOCK_SAMPLES;
  if (newTempoWindowSamples != tempoWindowSamples) {
    tempoWindowSamples = newTempoWindowSamples;
    holdCountdown = 0;
  }

  lastTempoSubdivisionIndex = subdivisionIndex;
  tempoWindowDirty = false;
}

} // namespace

void setStutterTimingMode(StutterTimingMode mode) {
  if (mode == currentStutterMode) {
    return;
  }
  currentStutterMode = mode;
  holdCountdown = 0;
  tempoWindowDirty = true;
  lastTempoSubdivisionIndex = -1;
}

StutterTimingMode stutterTimingMode() { return currentStutterMode; }

void setStutterBasePeriodMs(float milliseconds) {
  if (milliseconds <= 0.0f) {
    return; // keep last tempo so bypass regions don't nuke the groove
  }
  float newBase = milliseconds / 1000.0f;
  if (fabsf(newBase - stutterBasePeriodSeconds) < 0.0001f) {
    return;
  }
  stutterBasePeriodSeconds = newBase;
  tempoWindowDirty = true;
  lastTempoSubdivisionIndex = -1;
}

namespace {

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
  if (currentStutterMode == StutterTimingMode::TempoLocked) {
    int window = tempoWindowSamples;
    if (window < 1) {
      window = 1;
    }
    if (--holdCountdown <= 0) {
      holdCountdown = window;
      heldSample = sample;
    }
  } else {
    int window = 3 + static_cast<int>((1.0f - densityNorm) * 160.0f);
    if (--holdCountdown <= 0) {
      holdCountdown = window;
      heldSample = sample;
    }
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

float applyBloomLimiter(float sample, float amount, float &envelope) {
  if (amount <= 0.0f) {
    return sample;
  }

  float absSample = fabsf(sample);
  float attack = 0.45f + amount * 0.35f;
  float release = 0.92f - amount * 0.25f;
  if (absSample > envelope) {
    envelope = attack * absSample + (1.0f - attack) * envelope;
  } else {
    envelope = release * envelope + (1.0f - release) * absSample;
  }

  float swell = 1.0f + amount * (1.0f - envelope);
  float drive = 1.0f + amount * 8.0f;
  float saturated = tanhf(sample * drive);
  return constrain(saturated * swell, -1.0f, 1.0f);
}
} // namespace

size_t dirtStageCount() { return static_cast<size_t>(DirtStage::Count); }

const char *dirtStageId(DirtStage stage) {
  size_t index = static_cast<size_t>(stage);
  if (index >= dirtStageCount()) {
    return "";
  }
  return dirtStageRegistry[index].id;
}

void setActiveDirtStages(uint8_t stageMask) {
  activeDirtStageMask = stageMask & kAllDirtStagesMask;
}

uint8_t getActiveDirtStages() { return activeDirtStageMask; }

bool enableDirtStage(DirtStage stage, bool enabled) {
  uint8_t index = static_cast<uint8_t>(stage);
  if (index >= static_cast<uint8_t>(DirtStage::Count)) {
    return false;
  }
  uint8_t bit = dirtStageBit(stage);
  if (enabled) {
    activeDirtStageMask |= bit;
  } else {
    activeDirtStageMask &= static_cast<uint8_t>(~bit);
  }
  return true;
}

bool enableDirtStageById(const char *id, bool enabled) {
  if (id == nullptr) {
    return false;
  }
  for (const auto &entry : dirtStageRegistry) {
    if (stageIdEquals(entry.id, id)) {
      return enableDirtStage(entry.stage, enabled);
    }
  }
  return false;
}

float processDirt(float sample) {
  if (activeDirtStageMask == 0) {
    return sample;
  }

  // Apply glitch only on a percentage of samples defined by `density`. The rest
  // sail through untouched so the delay never loses its sense of pulse.
  long glitchRoll = random(100L);
  long densityThreshold = static_cast<long>(density);
  if (glitchRoll >= densityThreshold) {
    return sample;
  }

  float densityNorm = currentDensityNorm;
  float noiseNorm = currentNoiseNorm;

  DirtStageContext ctx{sample, densityNorm, noiseNorm, blockFuzzScale};
  for (const auto &entry : dirtStageRegistry) {
    if ((activeDirtStageMask & dirtStageBit(entry.stage)) != 0) {
      entry.apply(ctx);
    }
  }

  float toneBlend = sample;
  if (ctx.crushedActive && ctx.foldedActive) {
    toneBlend = (1.0f - noiseNorm) * ctx.crushed + noiseNorm * ctx.folded;
  } else if (ctx.crushedActive) {
    toneBlend = ctx.crushed;
  } else if (ctx.foldedActive) {
    toneBlend = ctx.folded;
  }

  float rhythmBlend = toneBlend;
  if (ctx.stutteredActive) {
    rhythmBlend = (1.0f - densityNorm) * toneBlend + densityNorm * ctx.stuttered;
  }

  float result = rhythmBlend;
  if (ctx.fuzzActive) {
    result += ctx.fuzzContribution;
  }

  result *= nextTremoloGain(densityNorm);
  return constrain(result, -1.0f, 1.0f);
}

void processAudioQueues() {
  // Each queue pair carries an interleaved block from the delay line and the
  // pre-delay tap. If one side is empty we skip the entire pass; trying to mix
  // mismatched buffers would smear channels and explode the noise floor.
  bool leftReady = queueL.available() && cleanQueueL.available();
  bool rightReady = queueR.available() && cleanQueueR.available();
  if (!leftReady && !rightReady) {
    return;
  }

  // Normalise control values into 0..1 so downstream helpers can lean on the
  // same maths regardless of pot scale or ladder step. The clamps keep us
  // honest when pots jitter or the button ladder nudges past the endpoints.
  currentDensityNorm = constrain(density / 100.0f, 0.0f, 1.0f);
  currentNoiseNorm = constrain(noiseAmount / 60.0f, 0.0f, 1.0f);

  if (currentStutterMode == StutterTimingMode::TempoLocked) {
    refreshTempoLockedWindow(currentDensityNorm);
  }

  // Sample chaos modulators once per audio block. Think of this as grabbing a
  // snapshot from a modular synth: the values stay frozen for the entire block
  // so we can apply them consistently while iterating sample-by-sample below.
  ChaosSnapshot chaosSnapshot =
      updateChaosModulators(currentDensityNorm, currentNoiseNorm,
                            AUDIO_BLOCK_SAMPLES);
  float baseMix = mixAmount;
  if (macroMixOverride >= 0.0f) {
    baseMix = macroMixOverride;
  }
  baseMix = constrain(baseMix + macroWetBias, 0.0f, 1.0f);
  blockMixAmount = constrain(baseMix + chaosSnapshot.mixOffset, 0.0f, 1.0f);
  blockFuzzScale = chaosSnapshot.fuzzGain;
  const float blockBloomAmount =
      constrain(bloomAmount + chaosSnapshot.bloomDepthOffset, 0.0f, 1.0f);
  const float blockPanOffset =
      constrain(chaosSnapshot.secondaryVoicePan, -1.0f, 1.0f);

  float baseFeedback = constrain(feedbackAmount + bloomFeedbackBoost, 0.0f, 0.99f);
  if (chaosModulatorsEnabled()) {
    // When chaos is active we temporarily bend the feedback gain. The limiter
    // below caps it at <1 to avoid runaway oscillation if the logistic map goes
    // particularly feral.
    float modFeedback =
        constrain(baseFeedback + chaosSnapshot.feedbackOffset, 0.0f, 0.99f);
    setFeedbackGain(1, modFeedback);
  } else {
    // Otherwise we honour the front-panel knob verbatim plus the macro shove.
    setFeedbackGain(1, baseFeedback);
  }

  if (leftReady && rightReady) {
    // Both taps are active: treat them as a stereo micro-mixer so we can cross
    // pollinate the ghost voice and run the bloom limiter across both tails.
    int16_t *dirtyL = queueL.readBuffer();
    int16_t *cleanL = cleanQueueL.readBuffer();
    int16_t *dirtyR = queueR.readBuffer();
    int16_t *cleanR = cleanQueueR.readBuffer();
    int16_t *outL = outputQueueL.getBuffer();
    int16_t *outR = outputQueueR.getBuffer();
    if (!outL || !outR) {
      queueL.freeBuffer();
      cleanQueueL.freeBuffer();
      queueR.freeBuffer();
      cleanQueueR.freeBuffer();
      return;
    }

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      float cL = static_cast<float>(cleanL[i]) / 32768.0f;
      float cR = static_cast<float>(cleanR[i]) / 32768.0f;
      float dL = static_cast<float>(dirtyL[i]) / 32768.0f;
      float dR = static_cast<float>(dirtyR[i]) / 32768.0f;

      dL = processDirt(dL);
      dR = processDirt(dR);

      if (secondaryVoiceLevel > 0.0f) {
        float cross = 0.5f * secondaryVoiceLevel;
        float crossFromR = constrain(cross * (1.0f - blockPanOffset), 0.0f, 1.0f);
        float crossFromL = constrain(cross * (1.0f + blockPanOffset), 0.0f, 1.0f);
        float ghostL = dL * (1.0f - crossFromR) + dR * crossFromR;
        float ghostR = dR * (1.0f - crossFromL) + dL * crossFromL;
        dL = ghostL;
        dR = ghostR;
      }

      float mixedL = (1.0f - blockMixAmount) * cL + blockMixAmount * dL;
      float mixedR = (1.0f - blockMixAmount) * cR + blockMixAmount * dR;

      if (blockBloomAmount > 0.0f) {
        mixedL = applyBloomLimiter(mixedL, blockBloomAmount, bloomEnvelopeL);
        mixedR = applyBloomLimiter(mixedR, blockBloomAmount, bloomEnvelopeR);
      }

      mixedL = constrain(mixedL, -1.0f, 1.0f);
      mixedR = constrain(mixedR, -1.0f, 1.0f);
      outL[i] = static_cast<int16_t>(mixedL * 32767.0f);
      outR[i] = static_cast<int16_t>(mixedR * 32767.0f);
    }

    outputQueueL.playBuffer();
    outputQueueR.playBuffer();
    queueL.freeBuffer();
    cleanQueueL.freeBuffer();
    queueR.freeBuffer();
    cleanQueueR.freeBuffer();
    return;
  }

  if (leftReady) {
    int16_t *dirty = queueL.readBuffer();
    int16_t *clean = cleanQueueL.readBuffer();
    int16_t *outBlock = outputQueueL.getBuffer();
    if (!outBlock) {
      queueL.freeBuffer();
      cleanQueueL.freeBuffer();
    } else {
      for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float c = static_cast<float>(clean[i]) / 32768.0f;
        float d = static_cast<float>(dirty[i]) / 32768.0f;
        d = processDirt(d);
        float mixed = (1.0f - blockMixAmount) * c + blockMixAmount * d;
        if (blockBloomAmount > 0.0f) {
          mixed = applyBloomLimiter(mixed, blockBloomAmount, bloomEnvelopeL);
        }
        mixed = constrain(mixed, -1.0f, 1.0f);
        outBlock[i] = static_cast<int16_t>(mixed * 32767.0f);
      }
      outputQueueL.playBuffer();
      queueL.freeBuffer();
      cleanQueueL.freeBuffer();
    }
  }

  if (rightReady) {
    int16_t *dirty = queueR.readBuffer();
    int16_t *clean = cleanQueueR.readBuffer();
    int16_t *outBlock = outputQueueR.getBuffer();
    if (!outBlock) {
      queueR.freeBuffer();
      cleanQueueR.freeBuffer();
    } else {
      for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float c = static_cast<float>(clean[i]) / 32768.0f;
        float d = static_cast<float>(dirty[i]) / 32768.0f;
        d = processDirt(d);
        float mixed = (1.0f - blockMixAmount) * c + blockMixAmount * d;
        if (blockBloomAmount > 0.0f) {
          mixed = applyBloomLimiter(mixed, blockBloomAmount, bloomEnvelopeR);
        }
        mixed = constrain(mixed, -1.0f, 1.0f);
        outBlock[i] = static_cast<int16_t>(mixed * 32767.0f);
      }
      outputQueueR.playBuffer();
      queueR.freeBuffer();
      cleanQueueR.freeBuffer();
    }
  }
}

void setupAudioPipeline() {
  // Reserve audio memory buffers. Each block equals 128 samples. Running the
  // macro delay wide open needs ~1180 ms worth of history plus headroom for the
  // state variable filter and record/play queues, so we overprovision.
  AudioMemory(512);

  // Wake the SGTL5000 codec on the Teensy Audio Shield. This is the actual ADC
  // front-end we use for line-level signals. The Teensy 4.0's on-chip ADC stays
  // parked—its driver in PJRC's library still targets the older Kinetis chips,
  // so we lean on the shield's rock-solid converters instead.
  audioShield.enable();
  audioShield.inputSelect(AUDIO_INPUT_LINEIN);
  audioShield.lineInLevel(10);  // Unity-ish gain, tweak to taste.
  audioShield.volume(0.7f);     // Leave headroom for hot synths/pedals.

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

