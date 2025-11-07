# SSD1306 OLED Notes

Optional but flashy: a 128×64 SSD1306 panel that mirrors loop state in text form. Here's how to wire and drive it without summoning gremlins.

## Wiring
- **VCC** → 3.3 V (do *not* feed 5 V clones unless you've confirmed the regulator)
- **GND** → Star ground
- **SCL** → Teensy 19
- **SDA** → Teensy 18
- **RES** → Teensy 5 (configurable; update firmware constant if you move it)
- **DC** → Teensy 6

## Firmware Hooks
- Display code lives in `src/display.cpp` (future home—log file path changes here). For now, the driver stub hides inside `src/main.cpp` under `#ifdef ENABLE_OLED`.
- Update this doc if you change buffer sizes or fonts so the next hacker knows what to rebuild.

## Mounting Tips
- The OLED flex cable is fragile. Route it away from the chaos buttons to avoid accidental punches.
- Keep at least 2 mm clearance behind the panel for airflow; these modules run warmer than they look.

## Troubleshooting
- Blank screen? Check I²C pull-ups (4.7 kΩ recommended). Share photos of clean rework for future reference.
- Inverted colors at boot usually mean the display was reset mid-frame. Log the firmware timing fix when you squash it.

Drop screenshots or `.png` exports in this folder. Bonus points for animated GIFs of the UI in action.
