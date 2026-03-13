#pragma once

// Minimal Teensy Audio stubs for host-side tests.

#include <array>
#include <cstdint>
#include <cstring>
#include <deque>

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
    bool available() const { return !buffers_.empty(); }
    int16_t *readBuffer() {
        if (buffers_.empty()) {
            active_ = nullptr;
            return nullptr;
        }
        active_ = &buffers_.front();
        return active_->data();
    }
    void freeBuffer() {
        if (active_ && !buffers_.empty() && active_ == &buffers_.front()) {
            buffers_.pop_front();
        }
        active_ = nullptr;
    }

    void pushBuffer(const int16_t *data) {
        std::array<int16_t, AUDIO_BLOCK_SAMPLES> block{};
        if (data) {
            std::memcpy(block.data(), data, sizeof(block));
        }
        buffers_.push_back(block);
    }

    void clearBuffers() {
        buffers_.clear();
        active_ = nullptr;
    }

  private:
    std::deque<std::array<int16_t, AUDIO_BLOCK_SAMPLES>> buffers_;
    std::array<int16_t, AUDIO_BLOCK_SAMPLES> *active_ = nullptr;
};

class AudioPlayQueue : public AudioStream {
  public:
    int16_t *getBuffer() { return buffer_; }
    void playBuffer() {
        std::memcpy(last_played_.data(), buffer_, sizeof(buffer_));
        has_played_ = true;
    }

    const int16_t *lastPlayedBuffer() const { return last_played_.data(); }
    bool hasPlayedBuffer() const { return has_played_; }
    void clearPlayedBuffer() {
        has_played_ = false;
        std::memset(buffer_, 0, sizeof(buffer_));
        last_played_.fill(0);
    }

  private:
    int16_t buffer_[AUDIO_BLOCK_SAMPLES] = {0};
    std::array<int16_t, AUDIO_BLOCK_SAMPLES> last_played_{};
    bool has_played_ = false;
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
