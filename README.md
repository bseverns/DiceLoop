# teensyDee2 - Chaos Delay

## Overview
teensyDee2 implements a chaotic delay effect for the **Teensy 4.0** microcontroller with the
official Audio Shield. It mixes a clean signal with a noisy, density‑modulated delay
line to produce unstable, glitchy echoes. The amount of bit‑crushing and how often
glitches occur are shaped by the *noise amount* and *density* controls. Two push buttons let you reseed or reset
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
- **Noise amount** &ndash; sets the intensity of the bit‑crush and noise added
  to the delay line.
- **Density** &ndash; controls the probability that a sample will be glitched,
  effectively adjusting how frequently dirt is injected.
- **Mix** &ndash; blends between the clean and dirty signals.
- **Reseed / Reset buttons** &ndash; randomise or clear the chaos values.

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

## License / Contributing
No explicit license is provided. Use at your own risk and feel free to send
improvements via pull requests.
