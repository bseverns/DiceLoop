# DiceLoop Hardware Lab

Welcome to the greasy-fingered half of DiceLoop. This folder is part builder's field guide, part notebook of questionable experiments. Every document here tries to explain *why* a part exists in the rig, how it couples with the firmware, and what will break in spectacular fashion if you swap it. Use it as a map, but scribble your own notes in the margins.

## How to roam this maze

- **Start with the BOM.** Grab the core inventory in [`bom/dice-loop-bom.md`](bom/dice-loop-bom.md) for the story version or the CSV if you are sending it to a fab shop.
- **Plan your wiring.** [`wiring/`](wiring) links signal names in the code (`src/main.cpp`) to physical copper.
- **Tweak the control surface.** Everything about pots, buttons, and LED feedback lives under [`control-surface/`](control-surface).
- **Pick a display.** LED bar or OLED? [`display-options/`](display-options) compares both with wiring call-outs.
- **Box it up.** [`enclosure/`](enclosure) contains panel templates, standoff spacing, and punch lists.
- **Prototype like a pro.** [`prototyping/`](prototyping) is the lab log for smoke tests and serial poking.
- **Share war stories.** [`fabrication-notes.md`](fabrication-notes.md) is where we stash EMI hacks, grounding rituals, and things we swore we'd remember next time.

## Hardware ↔ Firmware handshake

Most signals are annotated in `src/main.cpp`, but here's the cheat sheet so you don't have to grep with soldering fumes in your eyes:

| Function | Teensy Pin | Notes |
| --- | --- | --- |
| Input Pots (Gain, Loop, Feedback, Dice, Chaos) | A0, A1, A3, A4, A5 | 10k linear pots; firmware expects 0–1023 raw. |
| Chaos Buttons | D7, D8 | Active low with internal pull-ups enabled. |
| Entropy Clock (aka "dice re-roll") | D9 | Firmware pulses this to reseed random modulation. |
| LED Shift Register | D2 (SER), D3 (RCLK), D4 (SRCLK) | 74HC595 feeding the 10-seg LED bar. |
| Optional OLED (SSD1306) | SDA (18), SCL (19) | I²C, 3.3 V. |
| Audio Shield | Dedicated I²S pins | Keep the ribbon short to dodge clock jitter. |

Keep this table honest—if you reroute firmware pins, leave breadcrumbs here.

## Contributors

If you add diagrams, PDFs, or laser-cut files, drop a shout in the relevant README with revision history. Future-you will thank present-you.
