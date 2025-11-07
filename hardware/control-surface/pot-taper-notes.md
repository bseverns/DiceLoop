# Pot Taper Notes

The firmware maps raw ADC values straight into the looping engine without fancy curves. That means our tactile feel lives or dies on the pot taper.

## Stock Setup
- **10 kΩ linear, 16 mm body** — keeps the ADC happy and the sweep predictable.
- Firmware expects 0–3.3 V swing. Anything higher and you'll backfeed the Teensy inputs.

## Experiments to try
- **Audio/log pots for Gain:** Might feel smoother in the first quarter turn. If you try it, record the subjective feel and whether the noise floor jumps when the wiper hits 50%.
- **Reverse-log for Feedback:** Could provide finer control near self-oscillation. Update `src/main.cpp` scaling if you adopt it.

## Mechanical notes
- D-shaft vs. smooth? Our panel files assume D-shaft knobs. If you go smooth, widen the knob holes in `enclosure/tabletop-enclosure/dice-loop-panel.dxf` accordingly.

Leave your hacks here with measurements—this doc should read like a remix log, not a spec sheet.
