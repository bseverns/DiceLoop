// Audio processing pipeline public API.
//
// Exposes the globally-instantiated Audio objects as well as helper functions
// for initialisation and queue processing. This mirrors the structure laid out
// in src/audio_pipeline.cpp and lets other modules (controls, main) poke values
// directly without additional plumbing.
#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <Audio.h>

extern AudioInputI2S i2sIn;                // Audio shield line in
extern AudioEffectDelay delay1;            // Dual-tap delay line
extern AudioFilterStateVariable filter1;   // High-pass conditioning filter
extern AudioPlayQueue queueL, queueR;      // Post-delay dirty taps
extern AudioPlayQueue cleanQueueL, cleanQueueR; // Pre-delay clean taps
extern AudioEffectEnvelope limiter1;       // Dynamics safety net
extern AudioOutputI2S i2sOut;              // Audio shield line out
extern AudioMixer4 feedbackMixer;          // Combines dry signal + feedback loop

extern float mixAmount;                    // Dry/wet crossfade coefficient
extern float feedbackAmount;               // Feedback loop gain coefficient

void setupAudioPipeline();
void processAudioQueues();
float processDirt(float sample);

#endif

