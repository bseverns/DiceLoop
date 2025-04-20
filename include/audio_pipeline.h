#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <Audio.h>

void setupAudioPipeline();
void processAudioQueues();
float processDirt(float sample);

#endif
