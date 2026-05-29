# Chaos Button Field Notes

These two buttons are the main gesture surface for live control. The firmware
distinguishes short presses, long holds, and a dual-button chord, so hardware
feel matters.

## Electrical Basics
- Wired to Teensy D7 and D8 with internal pull-ups enabled.
- Buttons short to ground when mashed. Keep harness lengths equal so latency stays consistent.
- If you go with latching footswitches, document firmware tweaks—you'll need to debounce longer and maybe flip the active state.

## Feel & Feedback
- Add parallel 100 nF caps if your switch choice chatters. Note the part number when you do.
- LED acknowledgment? Tie one LED segment to flash on button press, but make sure the shift register timing can handle the extra writes.

## Current Gesture Map
- **Reseed short press** → raises the chaos ladder and reseeds the RNG.
- **Reset short press** → returns the ladder to defaults and reseeds the RNG.
- **Reseed hold (~600 ms)** → next stage-preset slot.
- **Reset hold (~600 ms)** → previous stage-preset slot.
- **Both buttons together** → toggles chaos modulators.
- **Keep holding both (~1.2 s total)** → toggles stutter tempo-lock mode.

If you script new gestures in firmware, log them here so the enclosure legends
and README stay aligned.

Drop audio clips or GIFs of live use—future builders should hear what the chaos does before they drill holes.
