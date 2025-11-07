# Serial Debug Playbook

When DiceLoop misbehaves, the serial monitor is your front-line ally. Here's how to wield it without losing a night of sleep.

## Quick Commands
- `h` — Help dump (lists available debug toggles).
- `m` — Print current loop meters.
- `c` — Force chaos reseed.
- `d` — Dump ADC raw values for all five pots.

Mirror any firmware changes to this table so the docs stay honest.

## Workflow
1. **Baseline log:** Right after boot, copy the serial output into this file with a timestamp. It becomes your known-good reference.
2. **Toggle features:** Use the commands above to isolate problems. For example, if the LED bar flickers, send `m` repeatedly and watch for jitter in the data.
3. **Record anomalies:** Paste weird logs below along with the fix you applied.

## Saved Logs
```
2024-04-05 23:12Z / JG
DiceLoop boot
ADC: 512 498 501 490 505
Chaos: idle
```

Add more logs in chronological order. Treat this like the margins of a lab notebook.
