# DiceLoop Native Test Bench

This directory hosts the laptop-native test bench. It mirrors the Teensy
firmware logic while swapping in lightweight Arduino/Audio stubs so you can
exercise the DSP and control logic without hardware.

## Run the tests

From the repo root:

```sh
pio test -e native
```

## What's covered

- Tempo sync tap logic, USB MIDI clock latching, and BPM tracking.
- Chaos modulator bounds (to keep offsets sane).
- Dirt stack registry, explicit factory preset seeds, and production-time mute
  mask rejection.
- Front-panel control gestures: short presses, preset holds, and the
  modulator/tempo-lock chord.
- Audio block behavior: clean/dirty dry-wet mixing, ghost crossfeed, and bloom
  compression behavior in `processAudioQueues()`, plus macro-forced dry bypass.
- Preset serial command parsing: slot listing, curated catalog listing,
  named-stage save, and invalid or muted-mask rejection.
- Offline sample renders: writes WAVs to `docs/sample/outputs/` using
  `docs/sample/sample.wav` as input and checks peak/RMS/zero-crossing
  fingerprints for regression coverage.

The stubs live in `tests/native/stubs/` and are intentionally tiny—add the next
Arduino/Audio helper there when you need it for a new test or demo.
