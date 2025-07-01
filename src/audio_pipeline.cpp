// Audio processing pipeline. Handles delay line, filtering and mixing
// between the clean and "dirty" signals. Key functions:
//   - setupAudioPipeline() : initialises the audio objects
//   - processAudioQueues() : mixes clean and dirty buffers
//   - processDirt()        : applies bit crushing
#include "audio_pipeline.h"
#include "Arduino.h"
#include "controls.h"

// === Audio Objects ===
AudioInputI2S          i2sIn;
AudioEffectDelay       delay1;
AudioFilterStateVariable filter1;
AudioPlayQueue         queueL, queueR;
AudioPlayQueue         cleanQueueL, cleanQueueR;
AudioEffectEnvelope    limiter1;
AudioOutputI2S         i2sOut;
AudioMixer4           feedbackMixer;

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
AudioConnection patchCord9(limiter1, 0, i2sOut, 0);
AudioConnection patchCord10(limiter1, 1, i2sOut, 1);

float processDirt(float sample) {
  // Bit-crush the incoming sample to introduce dirt/noise
  const int crushBits = 4;
  int crushed = int(sample * (1 << crushBits));
  float crushedSample = float(crushed) / (1 << crushBits);
  if (crushedSample > 0.8) crushedSample = 0.8;
  if (crushedSample < -0.8) crushedSample = -0.8;
  return crushedSample;
}

void processAudioQueues() {
  if (queueL.available() && cleanQueueL.available()) {
    // Mix left channel from clean and dirty delay buffers
    audio_block_t *dirty = queueL.readBuffer();
    audio_block_t *clean = cleanQueueL.readBuffer();
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      float c = (float)clean->data[i] / 32768.0f;
      float d = (float)dirty->data[i] / 32768.0f;
      // Apply dirt and blend with clean signal
      d = processDirt(d);
      float mixed = (1.0f - mixAmount) * c + mixAmount * d;
      mixed = constrain(mixed, -1.0f, 1.0f);
      clean->data[i] = (int16_t)(mixed * 32767.0f);
    }
    limiter1.input(clean, 0);
    queueL.freeBuffer();
  }

  if (queueR.available() && cleanQueueR.available()) {
    // Mix right channel from clean and dirty delay buffers
    audio_block_t *dirty = queueR.readBuffer();
    audio_block_t *clean = cleanQueueR.readBuffer();
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      float c = (float)clean->data[i] / 32768.0f;
      float d = (float)dirty->data[i] / 32768.0f;
      // Apply dirt and blend with clean signal
      d = processDirt(d);
      float mixed = (1.0f - mixAmount) * c + mixAmount * d;
      mixed = constrain(mixed, -1.0f, 1.0f);
      clean->data[i] = (int16_t)(mixed * 32767.0f);
    }
    limiter1.input(clean, 1);
    queueR.freeBuffer();
  }
}

void setupAudioPipeline() {
  // Reserve audio memory buffers
  AudioMemory(60);

  // Configure delay and filter defaults
  delay1.delay(0, 200);
  filter1.frequency(500);
  filter1.resonance(0.7);

  feedbackMixer.gain(0, 1.0f);
  feedbackMixer.gain(1, feedbackAmount);

  // Basic limiter settings to keep level in check
  limiter1.attack(5);
  limiter1.release(100);
  limiter1.hold(50);
}
