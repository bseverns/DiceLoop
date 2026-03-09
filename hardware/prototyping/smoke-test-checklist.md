# Smoke Test Checklist

Run this every time you assemble a new DiceLoop rig. It mirrors the firmware boot logs and catches the obvious gremlins before a gig.

1. **Visual inspection**
   - [ ] Headers soldered? No bridges?
   - [ ] Ribbon cables keyed correctly?
2. **Power sanity**
   - [ ] Plug in via USB. Verify 5 V and 3.3 V rails with a multimeter.
   - [ ] No smoke, no overheating regulators.
3. **Firmware handshake**
   - [ ] Upload latest build (`platformio run -t upload`).
   - [ ] Open serial monitor at 9600 baud.
   - [ ] Confirm startup includes `[chaos] subsystem armed – modulators idle` and a `[presets] loaded slot ...` line.
4. **Control sweep**
   - [ ] Twist each pot; audio response should track the knob moves.
   - [ ] Tap reseed/reset buttons; chaos intensity and LED bar should move between 0–8.
   - [ ] Hold reseed or reset for ~0.6 s; expect a `[presets] loaded slot ...` print as the dirt-stack preset changes.
5. **Audio path**
   - [ ] Patch in a synth drone. Confirm loop playback through headphones.
   - [ ] Sweep feedback to edge-of-oscillation. Listen for crackles (document if present).
6. **Displays**
   - [ ] LED bar climbs with input level.
   - [ ] OLED (if installed) shows status page without flicker.

Log failures and fixes right below this checklist with timestamp + initials. If a step changes, edit both the firmware comments and this doc.
