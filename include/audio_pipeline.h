#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <Audio.h>

extern AudioEffectDelay delay1;

void setupAudioPipeline();
void processAudioQueues();
float processDirt(float sample);

#endif
