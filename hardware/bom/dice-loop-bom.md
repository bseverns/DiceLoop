# DiceLoop Story BOM

This is the conversational bill of materials—why each part exists, what corners you can cut, and which ones will bite you.

| Qty | Part | Suggested Source | Why it matters | Notes |
| --- | ---- | ---------------- | -------------- | ----- |
| 1 | Teensy 4.0 | PJRC | Brains + DSP grunt. | Needs the audio shield header pins installed. |
| 1 | PJRC Audio Shield (Rev D) | PJRC | I²S codec and SD slot for sample juggling. | Solder the headphone amp jumpers if you want stereo outs. |
| 5 | 10 kΩ Linear Potentiometer (16 mm) | Tayda / Bourns | Controls gain, loop length, feedback, dice modulation, chaos depth. | Log pots will make the firmware scaling weird; stick to linear unless you patch the curves. |
| 2 | Momentary Pushbutton (tactile or stomp) | C&K / Tayda | Chaos triggers. | If you use footswitches, debounce caps might be required—document in control-surface notes. |
| 1 | 74HC595 Shift Register | TI / Nexperia | Drives the LED bar with three pins. | Socket it if you plan to mod the LED array. |
| 1 | 10-segment LED Bar Graph (2x5) | Kingbright | Loop level display. | Firmware currently lights 8 segments via one 74HC595; segments 9-10 need an added driver path. Diffuse with translucent tape for even glow. |
| 1 | SSD1306 128×64 OLED (Optional) | Adafruit / BuyDisplay | Fancy status page for loop nerds. | Needs 3.3 V logic; don't run off 5 V clones. |
| 2 | 3.5 mm Audio Jack (or 1/4" if going pedalboard) | Neutrik | In/out to the outside world. | If you float the sleeve, note it in fabrication notes. |
| 1 | DC Barrel Jack (2.1 mm center positive) | Switchcraft | Power in. | 9 V center-positive is standard, but we regulate down to 5 V/3.3 V on board. |
| misc | Resistors, headers, ribbon cable | Local | Pull-ups, LED current limiting, wiring harnesses. | Keep resistor values with the schematic you actually use. |

## Optional candy
- **Expression pedal jack:** Normalled to the Chaos pot. Document calibration if you add it.
- **MIDI I/O:** The firmware is ready for serial MIDI taps—log any opto-isolator choices here.

## Procurement checklist
- [ ] Order the CSV version (`panel-bom.csv`) if you need automated assembly.
- [ ] Double-check pot shaft style matches your knobs.
- [ ] Grab heat-shrink and ferrules. Future-you hates bare bus wire.
