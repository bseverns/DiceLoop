# DiceLoop Hardware Lab

Welcome to the build-facing half of DiceLoop. This folder is a field guide for
the physical rig: what each part does, which firmware surface it connects to,
and which details are optional versus part of the default build. Use it as a
map, then add your own notes as the hardware evolves.

## How to roam this maze

- **Start with the BOM.** Grab the core inventory in [`bom/dice-loop-bom.md`](bom/dice-loop-bom.md) for the story version or the CSV if you are sending it to a fab shop.
- **Plan your wiring.** [`wiring/`](wiring) links signal names in the code (`include/pin_config.h`) to physical copper.
- **Tweak the control surface.** Everything about pots, buttons, and LED feedback lives under [`control-surface/`](control-surface).
- **Pick a display.** LED bar or OLED? [`display-options/`](display-options) compares both with wiring call-outs.
- **Box it up.** [`enclosure/`](enclosure) contains panel templates, standoff spacing, and punch lists.
- **Prototype like a pro.** [`prototyping/`](prototyping) is the lab log for smoke tests and serial poking.
- **Share war stories.** [`fabrication-notes.md`](fabrication-notes.md) is where we stash EMI hacks, grounding rituals, and things we swore we'd remember next time.

## Hardware ↔ Firmware handshake

Most signal assignments are centralized in `include/pin_config.h`. This table is
the fast hardware-to-firmware translation layer:

| Function | Teensy Pin | Notes |
| --- | --- | --- |
| Input Pots (Delay, Feedback, Noise, Density, Mix) | A0, A1, A3, A4, A5 | 10k linear pots; firmware expects 0–1023 raw. |
| Chaos Buttons | D7, D8 | Active low with internal pull-ups enabled. |
| Tap Footswitch | D6 | Optional normally-open tap switch to ground for external tempo. |
| Entropy Seed Source | D9 | Configured as high-frequency PWM, then sampled with `analogRead()` for reseed entropy. |
| LED Shift Register | D2 (SER), D3 (RCLK), D4 (SRCLK) | One 74HC595 drives the default 8-LED bar. |
| Optional OLED (SSD1306) | SDA (18), SCL (19) | I²C only in the current firmware; no dedicated `RES`/`DC` pins are used. |
| Audio Shield | Dedicated I²S pins | Current firmware uses the left line-in channel as a mono source, then fans it into stereo-ish delay output. |

Keep this table honest. If you reroute pins or promote an optional hardware hook
into a supported feature, update this file and `include/pin_config.h` together.

## Contributors

If you add diagrams, PDFs, or laser-cut files, drop a shout in the relevant README with revision history. Future-you will thank present-you.
