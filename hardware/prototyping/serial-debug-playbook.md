# Serial Debug Playbook

When DiceLoop misbehaves, the serial monitor is your front-line ally. Here's how to wield it without losing a night of sleep.

## Quick Commands
- `help` — Print the preset command help.
- `preset help` — Same help text, namespaced.
- `preset list` — Show all preset slots, masks, and the active slot.
- `preset load <slot>` — Load slot `0..3`.
- `preset save <slot> <stage ids...>` — Save a stage combo into a slot.
- `preset mask <slot> <hex>` — Save a raw mask value (example: `0x5`).
- `preset stack list` — Show curated stack IDs.
- `preset stack load <id>` — Load a curated stack by ID.

There is no single-character command surface (`h`, `m`, `c`, `d`) in current firmware. Keep this table in lockstep with `pollStagePresetSerial()` in `src/stage_presets.cpp`.

## Workflow
1. **Baseline log:** Right after boot, copy the serial output into this file with a timestamp. It becomes your known-good reference.
2. **Exercise parser commands:** Run `preset list`, then load a slot and confirm the `[presets] loaded slot ...` response.
3. **Record anomalies:** Paste weird logs below along with the fix you applied.

## Saved Logs
```
2026-03-09 14:12Z / BS
[chaos] subsystem armed – modulators idle
[presets] loaded slot 0 → bit_crush+wave_fold+stutter+fuzz
preset list
[presets] slots
  0* factory:full_send  mask 0xF  stages: bit_crush+wave_fold+stutter+fuzz
  1  factory:crush_hiccups  mask 0x5  stages: bit_crush+stutter
  2  factory:sine_smear  mask 0x3  stages: bit_crush+wave_fold
  3  factory:fuzz_bloom  mask 0xA  stages: wave_fold+fuzz
```

Add more logs in chronological order. Treat this like the margins of a lab notebook.
