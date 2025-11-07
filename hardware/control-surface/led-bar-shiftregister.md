# LED Bar + Shift Register Playbook

We use a 74HC595 to slam 10 LEDs with just three pins. Here's how to keep the blink poetry tight.

## Wiring Snapshot
- **SER (Pin 14)** → Teensy D2
- **RCLK/Latch (Pin 12)** → Teensy D3
- **SRCLK/Clock (Pin 11)** → Teensy D4
- **OE (Pin 13)** → GND (always on; add a header if you want brightness control later)
- **MR (Pin 10)** → 3.3 V (reset disabled)
- **Outputs QA–QH** → LED bar segments 1–8; chain a second 595 for segments 9–10 or wire directly.

Current limiting: 330 Ω resistors per segment keep the bar happy at 3.3 V. If you drive from 5 V, recalc using your LED's forward voltage.

## Firmware Timing Notes
- Refresh happens in `renderMeters()` (check `src/main.cpp`). If you see ghosting, lengthen the latch pulse in code and log the change here.
- To add PWM dimming, hijack OE with a spare PWM pin and document the duty cycle experiments.

## Mods worth logging
- **Bi-color bar graph?** Note which outputs drive which color and any multiplexing tricks.
- **Alternate driver (TLC5916)?** Capture register settings so we can port the firmware without guesswork.

Snap photos of your wiring harness and throw them into `wiring/` so others can clone your layout.
