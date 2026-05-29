# LED Bar + Shift Register Playbook

The current firmware drives 8 LEDs from one 74HC595 with three pins. If you use
a 10-segment bar, treat segments 9-10 as unused unless you also extend the
firmware.

## Wiring Snapshot
- **SER (Pin 14)** → Teensy D2
- **RCLK/Latch (Pin 12)** → Teensy D3
- **SRCLK/Clock (Pin 11)** → Teensy D4
- **OE (Pin 13)** → GND (always on; add a header if you want brightness control later)
- **MR (Pin 10)** → 3.3 V (reset disabled)
- **Outputs QA–QH** → LED bar segments 1–8.

Current limiting: 330 Ω resistors per segment keep the bar happy at 3.3 V. If you drive from 5 V, recalc using your LED's forward voltage.

## Firmware Timing Notes
- Refresh rides the `renderStatusUI()` → `updateLEDBar()` path in `src/ui.cpp`.
  `renderStatusUI()` decides whether the bar is showing chaos level or a preset
  overlay before `updateLEDBar()` clocks the shift register.
- The actual latch-low / `shiftOut` / latch-high sequence lives in
  `shiftLedPattern()` and `updateLEDBar()`:
  ```cpp
  digitalWrite(pin_config::ledShiftLatch, LOW);
  shiftOut(pin_config::ledShiftData, pin_config::ledShiftClock, MSBFIRST, pattern);
  digitalWrite(pin_config::ledShiftLatch, HIGH);
  ```
  Flip `MSBFIRST` or reverse the segment wiring if your bar graph is upside
  down, then log the change here.
- Want PWM dimming? Patch OE to a spare PWM pin, extend `updateLEDBar()` with a duty-cycle helper, and document the sweet spot so we can keep the noir vibe without blinding anyone.

## Mods worth logging
- **Bi-color bar graph?** Note which outputs drive which color and any multiplexing tricks.
- **Alternate driver (TLC5916)?** Capture register settings so we can port the firmware without guesswork.

Snap photos of your wiring harness and throw them into `wiring/` so others can clone your layout.
