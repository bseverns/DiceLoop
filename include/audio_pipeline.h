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

inline void setFeedbackGain(unsigned int channel, float value) {
  audio_compat::setGain(feedbackMixer, channel, value);
}

void setupAudioPipeline();
void processAudioQueues();
float processDirt(float sample);

#endif

