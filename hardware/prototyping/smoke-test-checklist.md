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
   - [ ] Open serial monitor at 115200 baud. Confirm you see `DiceLoop boot` banner.
4. **Control sweep**
   - [ ] Twist each pot. Serial log should show values between 0–1023.
   - [ ] Press chaos buttons. Expect `CHAOS_TRIGGER` prints.
5. **Audio path**
   - [ ] Patch in a synth drone. Confirm loop playback through headphones.
   - [ ] Sweep feedback to edge-of-oscillation. Listen for crackles (document if present).
6. **Displays**
   - [ ] LED bar climbs with input level.
   - [ ] OLED (if installed) shows status page without flicker.

Log failures and fixes right below this checklist with timestamp + initials. If a step changes, edit both the firmware comments and this doc.
