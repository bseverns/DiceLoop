# DiceLoop Bring-Up And Smoke Test Checklist

Run this on every new build, repair, or wiring change before the rig leaves the
bench. The goal is not just "does it make sound?" but "is it safe, repeatable,
and ready for a rehearsal or gig?"

## Bench kit

- Firmware host with PlatformIO and a known-good USB cable
- Headphones or powered monitors
- Known-good mono source for line input
- 1/4" patch cables
- Multimeter
- Optional tap footswitch wired normally-open to ground
- Optional USB MIDI clock source
- Optional SSD1306 OLED, if that variant is installed

## Test artifact setup

- Firmware build: latest `main` or the candidate branch under review
- Input source: steady synth drone, looper phrase, or 440 Hz / 1 kHz tone
- Monitor level: conservative to start; bloom and feedback checks can jump fast
- Serial monitor: 9600 baud

Record `PASS`, `FAIL`, or `N/A` next to each step. If anything fails, stop,
write down the symptom, and note the fix before continuing.

## 1. Visual and power inspection

- [ ] Headers, socket strips, and panel wiring look fully soldered with no
      obvious bridges or cold joints.
- [ ] Ribbon cables and JST-style connectors are keyed correctly and fully
      seated.
- [ ] Pot grounds, button grounds, and shield grounds all continuity-check back
      to the same ground plane.
- [ ] USB power-up shows no smoke, no overheating regulators, and no hot audio
      shield.
- [ ] 5 V rail and 3.3 V rail measure within expected tolerance on the bench.

Pass condition: no visible assembly faults and rails are stable before audio is
patched.

## 2. Firmware handshake

- [ ] Upload the current firmware with `platformio run -t upload`.
- [ ] Open the serial monitor at 9600 baud.
- [ ] Confirm boot output includes:
      - `[chaos] subsystem armed – modulators idle`
      - `[presets] loaded slot ...`
- [ ] If EEPROM is blank or a fresh board was flashed, confirm the one-time
      default write message is sensible and the rig still lands on slot 0.

Pass condition: firmware boots cleanly and exposes the expected preset/chaos
state over serial.

## 3. Control surface sweep

- [ ] Turn each pot slowly through full travel and confirm the audible behavior
      tracks the control without dead zones or sudden dropouts.
- [ ] Short-tap the reseed and reset buttons; confirm chaos level moves through
      the ladder and the LED bar responds.
- [ ] Hold reseed for about 0.6 s; confirm the preset advances and serial logs a
      `[presets] loaded slot ...` message.
- [ ] Hold reset for about 0.6 s; confirm the preset moves backward and the
      overlay / LED bar updates accordingly.
- [ ] Hold both front-panel buttons long enough to toggle the chaos modulators;
      confirm the serial log reflects the mode change.
- [ ] Keep holding the dual-button chord long enough to toggle tempo-lock mode;
      confirm the serial log reflects the stutter timing change.

Pass condition: every physical control has a stable, repeatable effect and no
gesture triggers the wrong mode.

## 4. Audio path and macro behavior

- [ ] Feed the mono line input with a steady source and confirm signal reaches
      both outputs.
- [ ] Verify the box behaves as intended: mono-in, stereo-ish out, with the
      second voice and crossfeed widening the wet path instead of acting like a
      true stereo input processor.
- [ ] Sweep the delay macro through its four zones and confirm:
      - near-minimum: dry/bypass behavior
      - lower-mid: slapback range
      - upper-mid: ghost voice / crossfeed spread
      - upper range: bloom and wet push
- [ ] Sweep feedback up toward edge-of-oscillation and listen for crackles,
      zippering, or runaway behavior that does not recover when backed down.

Pass condition: the dry path is clean, the wet path follows the macro map, and
feedback stays controllable.

## 5. External tempo checks

- [ ] With no tap or MIDI clock present, confirm the rig falls back to the panel
      delay control after startup.
- [ ] If a tap footswitch is installed, stomp four steady quarter notes and
      confirm the stutter timing latches to the external pulse.
- [ ] Stop tapping for roughly 2.5 seconds and confirm the rig falls back to the
      internal tempo source.
- [ ] If a USB MIDI source is available, send clock plus start/continue and
      confirm the rig locks to MIDI tempo.
- [ ] Stop MIDI clock and confirm stale pulses do not keep rewriting tempo.

Pass condition: internal, tap, and MIDI tempo sources all hand off cleanly with
no sticky state.

## 6. Display acceptance

- [ ] LED bar lights cleanly with no stuck segments.
- [ ] LED bar still reflects chaos / level changes during preset cycling.
- [ ] If OLED is installed, confirm the main status page renders without flicker
      or bus lockups.
- [ ] Trigger preset changes and confirm the OLED overlay shows the slot and
      mask information.
- [ ] Drive tempo from each available source and confirm the OLED clock badge
      changes between `INT`, `TAP`, and `MIDI` as expected.
- [ ] Confirm BPM readout updates plausibly and does not freeze after source
      changes.

Pass condition: the display path stays readable and accurately reflects the
active tempo and preset state.

## 7. Preset and EEPROM retention

- [ ] Run `preset list` and confirm the four slot entries appear with their
      factory seed labels.
- [ ] Overwrite one slot with `preset save` or `preset stack save` and confirm
      the new mask applies immediately.
- [ ] Power-cycle the unit.
- [ ] Re-open serial and confirm the modified slot still appears in `preset
      list`.
- [ ] Confirm the previously active slot is restored on boot.
- [ ] In production firmware, verify `preset mask <slot> 0x0` is rejected rather
      than silently storing a mute mask.

Pass condition: slot edits survive reboot, active-slot restore works, and mute
remains a dev-only feature unless intentionally compiled in.

## 8. Line-level sanity and calibration

- [ ] Feed a known steady source at normal instrument/line level and set the rig
      for a mostly dry patch.
- [ ] Confirm the clean path is free of obvious clipping at nominal source
      levels.
- [ ] Increase wet mix and confirm the returned level stays usable without
      requiring extreme monitor gain changes.
- [ ] Compare bypass-ish output vs moderate wet output by ear or meter and note
      whether the box is broadly unity-ish, slightly hot, or slightly soft.
- [ ] If the output is unexpectedly weak or hot, log the source level, knob
      positions, and monitor chain before changing hardware or code.

Pass condition: the rig behaves predictably at normal line level and any gain
offset is documented rather than guessed at.

## 9. Sign-off

- [ ] Unit passes all required steps for its hardware variant.
- [ ] Any `N/A` items are explained below.
- [ ] Remaining quirks are logged with enough detail for the next bench session.

## Failure log

| Date | Initials | Build / Commit | Step | Symptom | Fix / Follow-up |
| --- | --- | --- | --- | --- | --- |
|     |     |     |     |     |     |
