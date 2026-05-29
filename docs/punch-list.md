# DiceLoop Punch List

This is a working TODO list built from the current repo state, not just from
existing `TODO` markers. It is ordered by leverage: fix the truth gap first,
then harden controls, then widen test coverage, then polish the hardware/docs
story.

## Current snapshot

- Native regression suite passes: `pio test -e native`
- The firmware already has meaningful depth: stage presets, tempo sync, OLED
  UI, chaos modulators, and offline sample rendering all exist
- The biggest gaps are product definition, hardware/firmware truthfulness, and
  test coverage around the trickiest control paths

## P0: Close the truth gap between docs, build config, and runtime behavior

- [x] Make USB MIDI clock part of the default build.
  Decision: USB MIDI clock is a default feature.
  Done when:
  - `platformio.ini` ships with a MIDI-capable USB profile
  - README/build docs describe MIDI clock as default behavior
  Next: add native coverage for the MIDI path so the default stays honest.

- [x] Record the signal-path decision as mono-in with stereo-ish output.
  Decision: the product is intentionally mono-in with stereo-ish delay output.
  Done when:
  - README and hardware docs describe the left-channel mono feed honestly
  - future work stops assuming a full stereo-through architecture
  Next: keep the audio graph as-is and focus effort on control/test hardening.

- [x] Clean up remaining documentation drift.
  Why: the repo mostly reads well, but some details are stale enough to erode
  trust.
  Done:
  - top-level and hardware docs now describe the mono-in / stereo-ish signal
    path consistently
  - OLED docs and generated schematic artifacts now reflect the current I2C-only
    wiring instead of the older `RES`/`DC` model
  - hardware control, LED bar, and serial docs now match the shipped firmware
    gestures and preset naming

## P1: Harden the control surface path

- [x] Replace the blocking `delay(50)` debounce logic with a non-blocking button
  state machine.
  Done:
  - button handling now uses debounced edge/hold tracking instead of blocking
    `delay()` calls in `updateControl()`
  - short press, long hold, and dual-button chord behavior are covered in
    native tests

- [x] Add tests for the real control logic, not just the DSP helpers.
  Done:
  - `test_controls.cpp` now covers reseed/reset short presses
  - long-press preset cycling is covered
  - the dual-button modulator/tempo-lock chord is covered

- [ ] Decide whether preset navigation should stay button-only or become a real
  first-class footswitch feature.
  Why: the UI helper already supports `cycleDirtStackPreset(direction,
  viaFootswitch)`, but there is no shipped hardware handler for preset
  footswitch navigation.
  Evidence: `src/ui.cpp`, `README.md`
  Tackle it: either formalize one or two extra digital inputs and document them,
  or keep this as an extension hook and remove any implication that it is built
  in.
  Deliverable: explicit product boundary around preset-footswitch control.

## P2: Turn the native suite into a real safety net

- [x] Add baseline coverage for the USB MIDI clock path.
  Done:
  - native tests can inject USB MIDI events through a stubbed `usbMIDI`
  - the suite now verifies MIDI clock latching and BPM calculation
  Next: add timeout/reset edge cases if the MIDI path grows more stateful.

- [x] Add baseline behavior coverage for `processAudioQueues()`.
  Done:
  - native Audio stubs can now inject record-queue blocks and capture play-queue
    output
  - block-level tests now verify the clean/dirty dry-wet blend path and ghost
    crossfeed path
  - bloom coverage now verifies that the bloom path compresses dynamic range
  - bypass coverage now verifies a macro-forced dry scene stays dry
  Next: broaden render regression and serial/parser coverage.

- [x] Upgrade the sample-render test from "files were written" to "audio still
  sounds like the intended preset."
  Done:
  - native render regression now checks peak, RMS, and zero-crossing
    fingerprints for each rendered sample output
  - the native RNG shim is deterministic, so render fingerprints stay stable
    across environments

- [x] Add serial preset parser tests.
  Done:
  - native `Serial` now supports deterministic input/output buffering
  - parser coverage now exercises `preset list`, named-stage save, and invalid
    mask handling

## P3: Simplify configuration and productize variants

- [x] Centralize board pin assignments into a single config header.
  Done:
  - front-panel, tap-tempo, and LED bar pins now live in `include/pin_config.h`
  - controls, tempo sync, UI, and docs all point at the same source of truth

- [x] Decide how curated stacks relate to stored preset slots.
  Done:
  - the curated stack catalog remains global, but the four preset slots now use
    an explicit factory seed map: `full_send`, `crush_hiccups`, `sine_smear`,
    and `fuzz_bloom`
  - the remaining curated entries stay discoverable via `preset stack list`
    / `preset stack load` without pretending they ship on the front panel
  - tests now assert the factory slot mapping instead of relying on catalog
    ordering

- [x] Clarify whether a mute preset is supported.
  Done:
  - production builds now reject zero stage masks instead of silently coercing
    them into "all stages on"
  - mute is still available as a deliberate dev-only escape hatch behind
    `-DDICELOOP_ALLOW_MUTED_STAGE_MASK=1`
  - README and native tests now describe and enforce that contract

## P4: Finish the hardware handoff

- [ ] Complete the missing hardware media/documentation called out in the repo.
  Why: the hardware folder is close to being build-friendly, but there are still
  obvious placeholders.
  Evidence:
  - `hardware/enclosure/tabletop-enclosure/knob-clearance-guide.md`
  - `hardware/prototyping/breadboard-layout.md`
  Tackle it: add the first-batch photos, side-profile shots, and annotated
  breadboard image once the next physical prototype is assembled.
  Deliverable: hardware docs that are actually build-assistive, not just
  descriptive.

- [x] Expand the smoke test into a repeatable acceptance checklist.
  Done:
  - the hardware smoke doc now includes explicit pass/fail steps for tap tempo,
    USB MIDI clock handoff, OLED tempo badges, EEPROM preset retention, and
    line-level sanity checks
  - the checklist now includes required bench gear, expected outcomes, and a
    structured failure log for build-by-build bring-up notes

## Suggested first sprint

- [ ] Remove blocking debounce and add `test_controls.cpp`.
- [x] Add at least one MIDI-clock native test.
- [x] Add one `processAudioQueues()` behavior test.
- [x] Do one docs sync pass after those decisions land.
