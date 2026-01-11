#pragma once

// Minimal Teensy Audio stubs for host-side tests.

#include <cstdint>
#include <cstring>

#ifndef AUDIO_BLOCK_SAMPLES
#define AUDIO_BLOCK_SAMPLES 128
#endif

#ifndef AUDIO_SAMPLE_RATE_EXACT
#define AUDIO_SAMPLE_RATE_EXACT 44100.0f
#endif

struct audio_block_t {
    int16_t data[AUDIO_BLOCK_SAMPLES];
};

class AudioStream {
  public:
    AudioStream() = default;
    explicit AudioStream(unsigned char, audio_block_t **) {}
    virtual ~AudioStream() = default;
    virtual void update() {}

    static audio_block_t *allocate() { return new audio_block_t(); }
    static void release(audio_block_t *block) { delete block; }

  protected:
    audio_block_t *receiveReadOnly(unsigned int) { return nullptr; }
    void transmit(audio_block_t *, unsigned int = 0) {}
};

class AudioInputI2S : public AudioStream {};
class AudioOutputI2S : public AudioStream {};

class AudioEffectDelay : public AudioStream {
  public:
    void delay(unsigned int, float) {}
};

class AudioFilterStateVariable : public AudioStream {
  public:
    void frequency(float) {}
    void resonance(float) {}
};

class AudioRecordQueue : public AudioStream {
  public:
    void begin() {}
    bool available() const { return false; }
    int16_t *readBuffer() { return nullptr; }
    void freeBuffer() {}
};

class AudioPlayQueue : public AudioStream {
  public:
    int16_t *getBuffer() { return buffer_; }
    void playBuffer() {}

  private:
    int16_t buffer_[AUDIO_BLOCK_SAMPLES] = {0};
};

class AudioMixer4 : public AudioStream {
  public:
    AudioMixer4() = default;
    void gain(unsigned int, float) {}
    void setGain(unsigned int, float) {}
};

class AudioConnection {
  public:
    AudioConnection(AudioStream &, unsigned char, AudioStream &, unsigned char) {}
};

class AudioControlSGTL5000 {
  public:
    void enable() {}
    void inputSelect(int) {}
    void lineInLevel(int) {}
    void volume(float) {}
};

inline void AudioMemory(int) {}

#ifndef AUDIO_INPUT_LINEIN
#define AUDIO_INPUT_LINEIN 0
#endif
