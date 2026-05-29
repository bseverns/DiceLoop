# SSD1306 OLED Notes

This display is optional. The current firmware can drive a small SSD1306 over
I2C to show tempo source, BPM, knob-derived values, and preset overlays.

## Wiring
- **VCC** → 3.3 V (do *not* feed 5 V clones unless you've confirmed the regulator)
- **GND** → Star ground
- **SCL** → Teensy 19
- **SDA** → Teensy 18

The shipped implementation uses I2C only. There are no dedicated `RES` or `DC`
signals in the current `src/ui.cpp` path.

## Firmware Hooks
- Display code lives in `src/ui.cpp`.
- Enable it with `-DDICELOOP_ENABLE_OLED=1` in `platformio.ini` or your local
  build flags.
- Defaults are `128x32` at I2C address `0x3C`. Override
  `DICELOOP_OLED_WIDTH`, `DICELOOP_OLED_HEIGHT`, or
  `DICELOOP_OLED_ADDRESS` only if your module differs.

## Mounting Tips
- The OLED flex cable is fragile. Route it away from the chaos buttons to avoid accidental punches.
- Keep at least 2 mm clearance behind the panel for airflow; these modules run warmer than they look.

## Troubleshooting
- Blank screen? First check that the build actually enabled
  `DICELOOP_ENABLE_OLED`, then verify I2C pull-ups and the device address.
- Inverted colors at boot usually mean the display was reset mid-frame. Log the firmware timing fix when you squash it.

Drop screenshots or `.png` exports in this folder. Bonus points for animated GIFs of the UI in action.
