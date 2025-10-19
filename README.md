# DiceLoop – Chaos Delay for the Restless

This project straps a chaotic delay line onto a **Teensy 4.0** board and dares you to
feed it audio. Clean tones go in, fractured echoes come out. Every knob twist
and button jab nudges the randomness, so you're never standing in the same river
twice. Use it to learn how real-time DSP works, or just to make your synth sound
like it fell down the stairs. Think of the repo as a lab notebook where every
subsystem is annotated, diagrammed, and cross-linked so you can both perform and
reverse-engineer the trickery.

> **Teaching vibe:** The comments and READMEs aim to be rigorous enough for a
> grad seminar yet still punk enough to keep you awake. Follow the links, read
> the notes, and then mod the hell out of it.

## Gear Checklist
- **Teensy 4.0** with the PJRC Audio Shield with input and outputs wired
- **Five 10k pots** wired like so:
  - `A0` – delay time
  - `A1` – feedback
  - `A3` – noise amount (bit‑crusher intensity)
  - `A4` – density (how often the glitches strike)
  - `A5` – wet/dry mix
- **Two momentary buttons**:
  - Pin `8` – reseed the chaos
  - Pin `7` – reset to calm
- **LED bar** driven through a shift register on pins `2`, `3`, and `4`

  ***gear diagrams soon***

Pin assignments live in `src/controls.cpp` and `src/ui.cpp` so you can swap
hardware without spelunking the whole codebase.

## Signal Flow Snapshot

```
┌────────────┐   Audio Shield I2S   ┌─────────────────────┐   ┌─────────────┐
│ Line / Mic │ ───────────────────► │  filter1 (HPF-ish)  │ ─►│ cleanQueue* │
└────────────┘                      └─────────┬───────────┘   └─────────────┘
                                              │
                                              ▼
                                    ┌────────────────────┐
                                    │ delay1 (two taps)  │
                                    └───────┬───────┬────┘
                                            │       │
                                            ▼       ▼
                                     queueL/queueR  ──► limiter1 ──► I2S out
                                            │
                                            ▼
                                      feedbackMixer ◄─────◄─ filter1
```

The `processAudioQueues()` function drains the `queue*` buffers, applies the
bit-crushing chaos via `processDirt()`, blends the dry/wet signals, and pipes
everything into `limiter1` before the final DAC hop. The ASCII diagram is rough,
but the code comments mirror it line-by-line so you can always map theory to
firmware.

## Control Map & Chaos Knob Lore

These parameters are polled each loop and shoved straight into the audio path.
The comments in `src/controls.cpp` walk through every scaling decision and why
the chosen ranges work on Teensy 4.0 (3.3 V reference, 10-bit ADC, etc.).

| Physical control | Teensy pin | Firmware symbol | Range & behaviour |
| ---------------- | ---------- | --------------- | ----------------- |
| Delay pot        | `A0`       | `delay1.delay`  | 1 ms – 300 ms of buffer on tap 0 |
| Feedback pot     | `A1`       | `feedbackAmount`| 0.00 – 1.00 linear gain into feedback mixer |
| Noise pot        | `A3`       | `noiseAmount`   | 0 – 60 → maps to 2–8 bit resolution in crusher |
| Density pot      | `A4`       | `density`       | 0 – 100% chance that a sample is crushed |
| Mix pot          | `A5`       | `mixAmount`     | 0.00 – 1.00 dry/wet crossfade |
| Reseed button    | `8`        | chaos ladder    | Adds +5 bits of nastiness per press until clamped |
| Reset button     | `7`        | chaos ladder    | Slams everything back to polite defaults |

The chaos ladder is intentionally dramatic: each reseed press ratchets both the
bit crushing depth and the glitch density, while also reseeding the RNG from an
analog pin so the statistical flavour changes in a way you can actually hear.

`src/audio_pipeline.cpp` handles the mixing and bit‑crushing while
`src/controls.cpp` maps the knobs and buttons to those globals. It's all plain
C++ and Arduino APIs—poke around and hack it.

### Teaching Notes

- **Signal levels:** The Teensy audio library pushes 16-bit signed integers in
  the range ±32767 inside `audio_block_t`. The comments in `processAudioQueues`
  spell out the conversion math so you can swap in your own fixed-point tricks.
- **Why the filter first?** `filter1` is configured as a gentle high-pass to
  keep DC out of the delay feedback path. A low, sticky offset will otherwise
  cause the delay to saturate. Change the frequency/resonance in
  `setupAudioPipeline()` and hear the difference.
- **Probability-based glitching:** `density` is interpreted as a % chance that
  we crush the current sample. This keeps the behaviour simple and makes it
  easy to swap in other distributions (Gaussian, correlated noise, etc.).
- **Feedback safety:** The limiter and explicit `constrain()` calls keep the
  feedback loop from rage quitting. Kill them if you want self-oscillation, but
  do it intentionally.

## Build & Flash
Built with [PlatformIO](https://platformio.org/). From the repo root:

```sh
pio run          # compile
pio run -t upload  # flash it to the board
```

`platform.ini` already targets the Teensy 4.0, so the above is enough to get
code onto the hardware.

> **Heads up:** The first `pio run` after cloning will pull in the entire
> [PJRC Audio library](https://www.pjrc.com/teensy/td_libs_Audio.html). Expect a
> minute or two. Future builds are quick.

## Repo Tour
- `src/` – firmware sources: `main.cpp`, `audio_pipeline.cpp`, `controls.cpp`,
  `ui.cpp`, `chaos.cpp`
- `include/` – headers shared across those files
- `rough/` – early Arduino sketch kept for historical kicks

## Study Pointers & Further Reading

- [Teensy Audio System Design Tool](https://www.pjrc.com/teensy/gui/index.html)
  – Drag blocks, wire them up, and export Arduino code. Compare the generated
  code to our hand-crafted setup in `src/audio_pipeline.cpp`.
- [PlatformIO Teensy Docs](https://docs.platformio.org/en/latest/boards/teensy/teensy40.html)
  – Explains the board configuration you inherit via `platform.ini`.
- [Bit crushing primer](https://ccrma.stanford.edu/~jos/filters/Bit_Reduction_Distortion.html)
  – Stanford CCRMA notes on what actually happens when you nuke bit depth.
- [Finite state machine for buttons](https://www.ganssle.com/debouncing.htm)
  – Want to replace the naive `delay(50)` debounce? Start here.

## Contributing / License
No explicit license. Consider it public domain punk: use, abuse, and send PRs if
you make it scream louder.
