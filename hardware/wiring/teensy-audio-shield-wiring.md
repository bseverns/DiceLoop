# Teensy ↔ Audio Shield Wiring Notes

We piggyback the PJRC audio shield on the Teensy 4.0 using the standard stacking headers. Still, there are a few quirks worth tattooing on your bench:

## Header Stack Checklist
- **Pins 0–13 + 14/A0–A9:** Must be soldered straight to keep the shield aligned. Use the spacer jig or a second breadboard to keep the headers perpendicular.
- **VIN/VUSB Cut?** If you power the rig from the barrel jack, cut the VUSB trace on the Teensy to keep USB power from back-feeding.
- **AGND tie-down:** Run a short jumper from AGND on the shield to the main ground star to keep the codec quiet.

## Extra Jumpers
| From | To | Purpose |
| --- | --- | --- |
| Audio Shield `MCLK` | Teensy pin 23 | Required for the codec clock; double-check continuity before power-up. |
| Audio Shield `SCK` | Teensy pin 13 | SPI for SD card; route away from analog traces. |
| Audio Shield `SDA/SCL` | Teensy 18/19 | Shared with the optional OLED—confirm pull-ups. |

## Testing Ritual
1. Power the rig with USB only and run the firmware smoke test in [`prototyping/smoke-test-checklist.md`](../prototyping/smoke-test-checklist.md).
2. Plug headphones into the shield and listen for the boot chirp. No chirp? Check `MCLK` continuity first.
3. Move each pot; you should hear noise floor shifts. If the codec locks up, check for solder bridges on the I²S pins.

_Add oscilloscope screenshots or SD card quirks here so we can keep the shield reliable._
