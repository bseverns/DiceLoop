# teensyDee2 – Chaos Delay for the Restless

This project straps a chaotic delay line onto a **Teensy 4.0** and dares you to
feed it audio. Clean tones go in, fractured echoes come out. Every knob twist
and button jab nudges the randomness, so you're never standing in the same river
twice. Use it to learn how real-time DSP works, or just to make your synth sound
like it fell down the stairs.

## Gear Checklist
- **Teensy 4.0** with the PJRC Audio Shield
- **Five pots** wired like so:
  - `A0` – delay time
  - `A1` – feedback
  - `A3` – noise amount (bit‑crusher intensity)
  - `A4` – density (how often the glitches strike)
  - `A5` – wet/dry mix
- **Two buttons**:
  - Pin `8` – reseed the chaos
  - Pin `7` – reset to calm
- **LED bar** driven through a shift register on pins `2`, `3`, and `4`

Pin assignments live in `src/controls.cpp` and `src/ui.cpp` so you can swap
hardware without spelunking the whole codebase.

## Sonic Mayhem Controls
These parameters are polled each loop and shoved straight into the audio path:
- **Delay time** – milliseconds of echo stored in `AudioEffectDelay`
- **Feedback** – gain fed from the delay output back to its input
- **Noise amount** – number of bits lopped off when crunching the feedback
- **Density** – percentage chance a sample gets mangled
- **Mix** – blend between the untouched and the trashed signals
- **Reseed** – bump up the chaos level and reseed the RNG
- **Reset** – clear the chaos and start clean

`src/audio_pipeline.cpp` handles the mixing and bit‑crushing while
`src/controls.cpp` maps the knobs and buttons to those globals. It's all plain
C++ and Arduino APIs—poke around and hack it.

## Build & Flash
Built with [PlatformIO](https://platformio.org/). From the repo root:

```sh
pio run          # compile
pio run -t upload  # flash it to the board
```

`platform.ini` already targets the Teensy 4.0, so the above is enough to get
code onto the hardware.

## Repo Tour
- `src/` – firmware sources: `main.cpp`, `audio_pipeline.cpp`, `controls.cpp`,
  `ui.cpp`, `chaos.cpp`
- `include/` – headers shared across those files
- `rough/` – early Arduino sketch kept for historical kicks

## Contributing / License
No explicit license. Consider it public domain punk: use, abuse, and send PRs if
you make it scream louder.
