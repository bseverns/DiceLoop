# teensyDee2 - Chaos Delay

## Overview
teensyDee2 implements a chaotic delay effect for the **Teensy 4.0** microcontroller with the
official Audio Shield. It mixes a clean signal with a noisy, density-modulated delay
line to produce unstable, glitchy echoes. Two push buttons let you reseed or reset
the random parameters while five pots control the effect in real time.

## Hardware Requirements
- Teensy 4.0
- PJRC Audio Shield
- Five potentiometers wired to:
  - A0 &ndash; delay time
  - A1 &ndash; feedback
  - A3 &ndash; noise amount
  - A4 &ndash; density
  - A5 &ndash; wet/dry mix
- Buttons on pins **8** (reseed) and **7** (reset)
- LED bar driven from pins **2**, **3**, and **4**

Exact assignments match the constants in [`src/controls.cpp`](src/controls.cpp)
and [`src/ui.cpp`](src/ui.cpp).

## Features
The firmware exposes several parameters, as defined in `controls.cpp`:
- **Delay time** &ndash; sets the delay length in milliseconds.
- **Feedback** &ndash; adjusts how much of the delayed signal is fed back.
- **Noise amount** &ndash; controls bit‑crushing/noise applied to the feedback path.
- **Density** &ndash; governs how often noisy glitches are added.
- **Mix** &ndash; blends between the clean and dirty signals.
- **Reseed / Reset buttons** &ndash; randomise or clear the chaos values.

## Usage
1. Connect audio input and output jacks to the Teensy Audio Shield.
2. Power the Teensy 4.0 and the Audio Shield.
3. Use the five pots to adjust:
   - **Delay time** for delay length
   - **Feedback** for echo intensity
   - **Noise amount** for the amount of bit-crush and fuzz
   - **Density** to set how frequently noise bursts occur
   - **Mix** to blend between clean and effected signals
4. Press **RESEED** (pin 8) to increase chaos; each press increments the level shown on the LED bar.
5. Press **RESET** (pin 7) to clear chaos and return to the cleanest state.
6. Experiment with different combinations while audio plays through the device.

## Building and Uploading
This project uses PlatformIO. Build with:
```
pio run
```
Upload the compiled firmware to the Teensy with:
```
pio run -t upload
```
Settings for the Teensy 4.0 are defined in [`platform.ini`](platform.ini).

## Source Layout
- `src/` &ndash; main firmware files (`main.cpp`, audio pipeline, controls, UI)
- `include/` &ndash; headers shared across the project
- `rough/` &ndash; early prototype sketch kept for reference (optional)

## Remaining Work
The firmware is functional but still rough around the edges:
- `setupChaos()` in `src/chaos.cpp` is only a placeholder and should add
  modulation or additional randomness.
- Parameter ranges and scaling could be tuned for musical results.
- Saving or recalling presets is not implemented.

## License / Contributing
No explicit license is provided. Use at your own risk and feel free to send
improvements via pull requests.
