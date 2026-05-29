# Pot Taper Notes

The front panel has five pots, but they do not all behave the same way in
firmware. Delay is a macro control with several regions, while the others map
more directly into gain or probability-style parameters. Pot choice therefore
affects feel as much as raw range.

## Stock Setup
- **10 kΩ linear, 16 mm body** — keeps the ADC happy and the sweep predictable.
- Firmware expects 0–3.3 V swing. Anything higher and you'll backfeed the Teensy inputs.

## Experiments to try
- **Audio/log taper for Delay:** Probably not worth it. The delay control already
  uses a custom macro curve in `src/controls.cpp`, so a non-linear taper may
  make the first scene boundaries harder to hit.
- **Reverse-log for Feedback:** Could provide finer control near
  self-oscillation. Update the scaling in `src/controls.cpp` if you adopt it.

## Mechanical notes
- D-shaft vs. smooth? Our panel files assume D-shaft knobs. If you go smooth, widen the knob holes in `enclosure/tabletop-enclosure/dice-loop-panel.dxf` accordingly.

Leave your hacks here with measurements—this doc should read like a remix log, not a spec sheet.
