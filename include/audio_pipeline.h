// Definitions for the audio processing pipeline. Provides setup
// and buffer mixing utilities used by the main firmware.
#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <Audio.h>

extern AudioEffectDelay delay1;

void setupAudioPipeline();
void processAudioQueues();
float processDirt(float sample);

#endif
