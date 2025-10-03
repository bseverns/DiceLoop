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
//                         limiter1 → i2sOut
//
// The helper functions in this file set up the graph and then manually mix the
// queue buffers so we can sprinkle in probabilistic bit crushing.
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
AudioConnection patchCord9(limiter1, 0, i2sOut, 0);
AudioConnection patchCord10(limiter1, 1, i2sOut, 1);

// Ratio of dirty (post-delay) to clean (pre-delay) signal. 0 = dry, 1 = fully
// crushed chaos.
float mixAmount = 0.5f;

float processDirt(float sample) {
  // Apply glitch only on a percentage of samples defined by `density`.
  // `density` comes from the controls module and represents 0–100%.
  if (random(100) >= density) {
    return sample;
  }

  // Map noiseAmount (0-60) to a reduction in bit depth. Higher values mean
  // fewer bits and therefore harsher crushing. The formula intentionally keeps
  // the lowest resolution at 2 bits so the waveform retains *some* structure.
  int crushBits = 8 - noiseAmount / 10; // roughly 8..2 bits of resolution
  if (crushBits < 2) crushBits = 2;
  int steps = 1 << crushBits;

  int crushed = int(sample * steps);
  float crushedSample = float(crushed) / steps;

  // Inject random noise scaled by noiseAmount to add fuzziness. Random returns
  // a 15-bit integer; we normalise to ±1.0 and scale by a 0–0.6 factor.
  float noise = ((float)random(-32768, 32767) / 32767.0f) * (noiseAmount / 100.0f);
  float result = crushedSample + noise;

  result = constrain(result, -1.0f, 1.0f);
  return result;
}

void processAudioQueues() {
  if (queueL.available() && cleanQueueL.available()) {
    // Mix left channel from clean and dirty delay buffers. Audio blocks are
    // 128-sample chunks. Teensy represents them as int16_t where ±32767 equals
    // ±1.0f. We convert to floats for clarity then convert back.
    audio_block_t *dirty = queueL.readBuffer();
    audio_block_t *clean = cleanQueueL.readBuffer();
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      float c = (float)clean->data[i] / 32768.0f;
      float d = (float)dirty->data[i] / 32768.0f;
      // Apply dirt and blend with clean signal.
      d = processDirt(d);
      float mixed = (1.0f - mixAmount) * c + mixAmount * d;
      mixed = constrain(mixed, -1.0f, 1.0f);
      clean->data[i] = (int16_t)(mixed * 32767.0f);
    }
    limiter1.input(clean, 0);
    queueL.freeBuffer();
  }

  if (queueR.available() && cleanQueueR.available()) {
    // Repeat the same dance for the right channel.
    audio_block_t *dirty = queueR.readBuffer();
    audio_block_t *clean = cleanQueueR.readBuffer();
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      float c = (float)clean->data[i] / 32768.0f;
      float d = (float)dirty->data[i] / 32768.0f;
      // Apply dirt and blend with clean signal.
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
  // Reserve audio memory buffers. Each block equals 128 samples, so 60 blocks
  // gives the delay ample breathing room without starving the mixer.
  AudioMemory(60);

  // Configure delay and filter defaults. `delay1.delay(0, x)` sets tap 0 (left)
  // to x milliseconds. The right channel uses the library default and is
  // modulated via the same API if desired.
  delay1.delay(0, 200);
  filter1.frequency(500);
  filter1.resonance(0.7);

  feedbackMixer.gain(0, 1.0f);
  feedbackMixer.gain(1, feedbackAmount);

  // Basic limiter settings to keep level in check. Adjust attack/release to
  // taste; the defaults keep transients snappy while catching runaway feedback.
  limiter1.attack(5);
  limiter1.release(100);
  limiter1.hold(50);
}

