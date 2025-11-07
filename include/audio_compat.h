#ifndef AUDIO_COMPAT_H
#define AUDIO_COMPAT_H

// Compatibility helpers to smooth over API differences across Teensy Audio
// library revisions. The project leans on `AudioMixer4::gain()` and friends, but
// newer releases renamed or refactored a handful of methods. These templates
// provide a tiny adapter layer so the firmware keeps building regardless of
// which version PlatformIO pulls in.

#include <algorithm>
#include <cstring>
#include <type_traits>

namespace audio_compat {

// `AudioMixer4` (and its modern cousins) expose either `.gain()` or
// `.setGain()`. Newer Teensy Audio builds added an optional smoothing argument
// to `.gain()`, so we probe for all three shapes and fall back to a static
// assert if none exist. The overload dance keeps compilation-time errors crisp
// instead of devolving into linker mysteries.
namespace detail {

// Some Teensy Audio releases accidentally drop `AudioMixer4`'s default
// constructor, which breaks the classic `AudioMixer4 mixer;` pattern plastered
// across their own examples. Rather than panic, we ship a tiny four-channel
// mixer that mirrors the public API we actually use (gain-setting plus basic
// mixing). When the upstream class is constructible we defer to it; otherwise we
// swap in this shim.
class Mixer4Fallback : public AudioStream {
 public:
  Mixer4Fallback() : AudioStream(kChannelCount, inputQueueArray_) {
    for (unsigned int i = 0; i < kChannelCount; ++i) {
      inputQueueArray_[i] = nullptr;
      gains_[i] = 1.0f;
    }
  }

  void gain(unsigned int channel, float value) {
    if (channel >= kChannelCount) {
      return;
    }
    gains_[channel] = std::clamp(value, 0.0f, 1.0f);
  }

  void setGain(unsigned int channel, float value) { gain(channel, value); }

  void update() override {
    audio_block_t *blocks[kChannelCount];
    bool hasInput = false;
    for (unsigned int i = 0; i < kChannelCount; ++i) {
      blocks[i] = receiveReadOnly(i);
      hasInput |= (blocks[i] != nullptr);
    }
    if (!hasInput) {
      return;
    }

    audio_block_t *out = allocate();
    if (!out) {
      for (audio_block_t *block : blocks) {
        if (block) {
          release(block);
        }
      }
      return;
    }

    std::memset(out->data, 0, sizeof(out->data));

    for (unsigned int i = 0; i < kChannelCount; ++i) {
      audio_block_t *block = blocks[i];
      if (!block) {
        continue;
      }
      float gain = gains_[i];
      if (gain == 0.0f) {
        release(block);
        continue;
      }
      for (int j = 0; j < AUDIO_BLOCK_SAMPLES; ++j) {
        float mixed = static_cast<float>(out->data[j]) +
                      static_cast<float>(block->data[j]) * gain;
        mixed = std::clamp(mixed, -32768.0f, 32767.0f);
        out->data[j] = static_cast<int16_t>(mixed);
      }
      release(block);
    }

    transmit(out);
    release(out);
  }

 private:
  static constexpr unsigned int kChannelCount = 4;
  audio_block_t *inputQueueArray_[kChannelCount];
  float gains_[kChannelCount];
};

template <typename Mixer, bool = std::is_default_constructible<Mixer>::value>
struct Mixer4Selector {
  using type = Mixer;
};

template <typename Mixer>
struct Mixer4Selector<Mixer, false> {
  using type = Mixer4Fallback;
};

using Mixer4Type = typename Mixer4Selector<AudioMixer4>::type;

template <typename Mixer>
auto dispatchGain(Mixer &mixer, unsigned int channel, float gain, int)
    -> decltype(mixer.gain(channel, gain, 0), void()) {
  mixer.gain(channel, gain, 0);
}

template <typename Mixer>
auto dispatchGain(Mixer &mixer, unsigned int channel, float gain, long)
    -> decltype(mixer.gain(channel, gain), void()) {
  mixer.gain(channel, gain);
}

template <typename Mixer>
auto dispatchGain(Mixer &mixer, unsigned int channel, float gain, long long)
    -> decltype(mixer.setGain(channel, gain), void()) {
  mixer.setGain(channel, gain);
}

template <typename Mixer>
void dispatchGain(Mixer &, unsigned int, float, ...) {
  static_assert(sizeof(Mixer) == 0,
                "audio_compat::setGain: mixer missing gain setter");
}

}  // namespace detail

using Mixer4 = detail::Mixer4Type;

template <typename Mixer>
void setGain(Mixer &mixer, unsigned int channel, float gain) {
  detail::dispatchGain(mixer, channel, gain, 0);
}

}  // namespace audio_compat

#endif  // AUDIO_COMPAT_H
