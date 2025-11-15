// Audio processing pipeline public API.
//
// Exposes the globally-instantiated Audio objects as well as helper functions
// for initialisation and queue processing. This mirrors the structure laid out
// in src/audio_pipeline.cpp and lets other modules (controls, main) poke values
// directly without additional plumbing.
#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#if __has_include(<Arduino.h>)
#include <Arduino.h>
#else
#include <stdint.h>
using boolean = bool;
#endif

#ifndef F_CPU_ACTUAL
#ifdef F_CPU
#define F_CPU_ACTUAL F_CPU
#endif
#endif

#include <Audio.h>
#include <cstddef>

#include "audio_compat.h"

extern AudioInputI2S i2sIn;                // Audio shield line in
extern AudioEffectDelay delay1;            // Dual-tap delay line
extern AudioFilterStateVariable filter1;   // High-pass conditioning filter
extern AudioRecordQueue queueL, queueR;    // Post-delay dirty taps
extern AudioRecordQueue cleanQueueL, cleanQueueR; // Pre-delay clean taps
extern AudioOutputI2S i2sOut;              // Audio shield line out
extern audio_compat::Mixer4 feedbackMixer; // Combines dry signal + feedback loop
extern AudioControlSGTL5000 audioShield;   // Codec controller (ADC/DAC front-end)

extern float mixAmount;                    // Dry/wet crossfade coefficient
extern float feedbackAmount;               // Feedback loop gain coefficient
extern float macroMixOverride;             // Optional dry/wet override from delay macro
extern float macroWetBias;                 // Extra wetness shove for macro stages
extern float secondaryVoiceLevel;          // Crossfeed blend for the ghost voice
extern float bloomAmount;                  // Limiter/feedback bloom depth (0..1)
extern float bloomFeedbackBoost;           // Additional feedback injected by bloom

enum class StutterTimingMode {
  Probability,  // density acts as per-sample probability (legacy behaviour)
  TempoLocked   // stutter window snaps to musical subdivisions of the tempo
};

void setStutterTimingMode(StutterTimingMode mode);
StutterTimingMode stutterTimingMode();
void setStutterBasePeriodMs(float milliseconds);

inline void setFeedbackGain(unsigned int channel, float value) {
  audio_compat::setGain(feedbackMixer, channel, value);
}

enum class DirtStage : uint8_t {
  BitCrush = 0,
  WaveFold,
  Stutter,
  Fuzz,
  Count
};

constexpr uint8_t dirtStageBit(DirtStage stage) {
  return static_cast<uint8_t>(1u << static_cast<uint8_t>(stage));
}

struct DirtStackInfo {
  const char *id;
  const char *label;
  uint8_t mask;
};

size_t dirtStageCount();
const char *dirtStageId(DirtStage stage);
void setActiveDirtStages(uint8_t stageMask);
uint8_t getActiveDirtStages();
bool enableDirtStage(DirtStage stage, bool enabled);
bool enableDirtStageById(const char *id, bool enabled);
bool dirtStageById(const char *id, DirtStage *stage);

size_t curatedDirtStackCount();
bool curatedDirtStackInfo(size_t index, DirtStackInfo *info);
bool curatedDirtStackById(const char *id, DirtStackInfo *info);

void setupAudioPipeline();
void processAudioQueues();
float processDirt(float sample);

#endif

