# Chaos Button Field Notes

These two buttons are the human gateway to the random modulation engine. Treat them like performance controls, not afterthoughts.

## Electrical Basics
- Wired to Teensy D7 and D8 with internal pull-ups enabled.
- Buttons short to ground when mashed. Keep harness lengths equal so latency stays consistent.
- If you go with latching footswitches, document firmware tweaks—you'll need to debounce longer and maybe flip the active state.

## Feel & Feedback
- Add parallel 100 nF caps if your switch choice chatters. Note the part number when you do.
- LED acknowledgment? Tie one LED segment to flash on button press, but make sure the shift register timing can handle the extra writes.

## Performance Tricks
- Dual-press combos are fair game. If you script new behaviors in firmware (e.g., double-tap for freeze), log the UX here so the enclosure legends can keep up.

Drop audio clips or GIFs of live use—future builders should hear what the chaos does before they drill holes.
