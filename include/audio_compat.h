#ifndef AUDIO_COMPAT_H
#define AUDIO_COMPAT_H

// Compatibility helpers to smooth over API differences across Teensy Audio
// library revisions. The project leans on `AudioMixer4::gain()` and friends, but
// newer releases renamed or refactored a handful of methods. These templates
// provide a tiny adapter layer so the firmware keeps building regardless of
// which version PlatformIO pulls in.

namespace audio_compat {

// `AudioMixer4` (and its modern cousins) expose either `.gain()` or
// `.setGain()`. We SFINAE our way into the right call so the rest of the code
// can stick with a single helper.
template <typename Mixer>
auto setGain(Mixer &mixer, unsigned int channel, float gain)
    -> decltype(mixer.gain(channel, gain), void()) {
  mixer.gain(channel, gain);
}

template <typename Mixer>
auto setGain(Mixer &mixer, unsigned int channel, float gain)
    -> decltype(mixer.setGain(channel, gain), void()) {
  mixer.setGain(channel, gain);
}

}  // namespace audio_compat

#endif  // AUDIO_COMPAT_H
