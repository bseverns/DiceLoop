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

- Tempo sync tap logic and external clock latching.
- Chaos modulator bounds (to keep offsets sane).
- Dirt stack registry + preset defaults.
- Offline sample renders: writes WAVs to `docs/sample/outputs/` using
  `docs/sample/sample.wav` as input.

The stubs live in `tests/native/stubs/` and are intentionally tiny—add the next
Arduino/Audio helper there when you need it for a new test or demo.
