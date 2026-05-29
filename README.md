# DiceLoop – Chaos Delay for the Restless

![PlatformIO Build status](https://github.com/bseverns/DiceLoop/actions/workflows/build.yml/badge.svg)

DiceLoop is a teachable chaos delay built around a **Teensy 4.0** and the PJRC
Audio Shield. The firmware takes a mono line-level input, pushes it through a
delay and a small stack of dirt stages, and returns a stereo-ish output built
from dual taps, crossfeed, and bloom-style feedback shaping. The repo is meant
to do two jobs at once: give you something playable, and make the signal path
legible enough that you can study or change it without guessing.

## Gear Checklist
- **Teensy 4.0** with the PJRC Audio Shield with input and outputs wired
- **Five 10k pots** wired like so:
  - `A0` – delay time
  - `A1` – feedback
  - `A3` – noise amount (bit‑crusher intensity)
  - `A4` – density (how often the glitches strike)
  - `A5` – wet/dry mix
- **Two momentary buttons**:
  - Pin `8` – reseed the chaos
  - Pin `7` – reset to calm
- **LED bar** driven through a shift register on pins `2`, `3`, and `4`
- **(Optional) I²C OLED** on the Teensy 4.0's default SDA/SCL (pins `18`/`19`). Any
  3.3 V-friendly 128×32 or 128×64 SSD1306 panel works.

<p align="center">
  <img src="hardware/wiring/dice-loop-control-harness.svg" alt="DiceLoop Teensy 4.0 wiring harness map" width="780" />
  <br />
  <em>Pots ride the analog bus (A0/A1/A3/A4/A5), buttons dive to pins 7 &amp; 8, the LED bar chews on D2–D4, and the optional OLED taps SDA/SCL on 18/19. Route the harness like this and nothing smokes.</em>
</p>

Pin assignments live in `include/pin_config.h` so you can swap hardware without
spelunking the whole codebase.

## Signal Flow Snapshot

```
┌──────────────┐   Audio Shield I²S   ┌───────────────────────┐
│ Line In (L)  │ ───────────────────► │  filter1 (gentle HPF) │
└──────────────┘                      └──────────┬────────────┘
                                               │
                                               │      clean tap for manual mix
                                               ▼
                                   ┌───────────────────────────┐
                                   │ cleanQueueL / cleanQueueR │
                                   └──────────┬────────────────┘
                                              │
                                              │
                                              ▼
                                 ┌─────────────────────────┐
                                 │  feedbackMixer (4x1)   │◄──────────────┐
                                 └──────────┬──────────────┘               │
                                            │                              │
                                            ▼                              │
                                  ┌──────────────────────┐                 │
                                  │ delay1 (stereo taps) │────────────────┘
                                  └──────────┬───────────┘
                                             │
                                             │   post-delay capture for chaos
                                             ▼
                               ┌────────────────────────────────┐
                               │ queueL / queueR  (dirty tap)   │
                               └──────────┬─────────────────────┘
                                           │
                           chaos modulators│ feed offsets to ↓
                 reseed/reset ladder & pots │
                                           ▼
                 ┌────────────────────────────────────────────────────┐
                 │ processAudioQueues()                               │
                 │   ├─ blend clean tap + dirty tap                   │
                 │   ├─ Chaos Engine: processDirt()                   │
                 │   │     • bit crush core                           │
                 │   │     • wavefold smear                           │
                 │   │     • stutter / hold shards                    │
                 │   └─ trem/fuzz polish + mix routing                │
                 └──────────┬─────────────────────────────────────────┘
                             │
                             ▼
                   ┌────────────────────────────┐
                   │ outputQueueL / outputQueueR│
                   └──────────┬─────────────────┘
                              │
                              ▼
                            I²S out
```

Read the diagram from left to right: `filter1` conditions the incoming signal,
`delay1` creates the repeat structure, and `processAudioQueues()` is where the
firmware explicitly blends clean and dirty paths before writing samples back to
the output queues. The ASCII map is intentionally simple; the matching comments
in `src/audio_pipeline.cpp` provide the implementation-level detail.

### Analog Front-End (Where the ADC actually lives)

If you are tracing where analog becomes digital, start with the Audio Shield.
The active ADC is the SGTL5000 codec, not the MCU's on-chip ADC. In
`src/audio_pipeline.cpp`, `setupAudioPipeline()` brings up
`AudioControlSGTL5000`, selects **line in**, and feeds the current mono input
path from the left channel into the delay graph. The output is intentionally
stereo-ish rather than true stereo-through.

## Control Map & Chaos Knob Lore

`updateControl()` polls these parameters every loop and maps them into DSP
values. If you want to understand the exact curves and guardrails, read the
inline comments in `src/controls.cpp`.

| Physical control | Teensy pin | Firmware symbol | Range & behaviour |
| ---------------- | ---------- | --------------- | ----------------- |
| Delay pot        | `A0`       | `delay1.delay`  | Four-scene macro: dead-dry stop, 0–300 ms slapback, ghost-voice crossfeed, bloom limiter |
| Feedback pot     | `A1`       | `feedbackAmount`| 0.00 – 1.00 linear gain into feedback mixer |
| Noise pot        | `A3`       | `noiseAmount`   | 0 – 60 → maps to 2–8 bit resolution in crusher |
| Density pot      | `A4`       | `density`       | 0 – 100% chance that a sample is crushed |
| Mix pot          | `A5`       | `mixAmount`     | 0.00 – 1.00 dry/wet crossfade |
| Reseed button    | `8`        | chaos ladder    | Adds +5 bits of nastiness per press until clamped |
| Reset button     | `7`        | chaos ladder    | Slams everything back to polite defaults |
| Reseed button (hold ≥600 ms) | `8` | dirt stack select | Steps forward through the four stage-preset slots; LED bar flashes the mask, OLED spells out the slot/mask combo |
| Reset button (hold ≥600 ms)  | `7` | dirt stack select | Steps backward through the same slots; same overlay so you can sanity-check what just loaded |
| Tap footswitch†  | `6`        | tempo sync      | Optional normally-open tap jack. Each stomp recalibrates the tempo grid. |

† Wire the tap footswitch as normally-open to ground. The firmware leans on the
Teensy's internal pull-up and averages the last few hits so a sloppy stomp
doesn't spray the tempo.

The chaos ladder is intentionally dramatic: each reseed press ratchets both the
bit crushing depth and the glitch density, while also reseeding the RNG from an
analog pin so the statistical flavour changes in a way you can actually hear.

**Stage presets (aka curated dirt stacks)**

- The firmware keeps two related concepts separate:
  - a curated stack catalog in `src/audio_pipeline.cpp`
  - four performer-facing preset slots in `src/stage_presets.cpp`
- The preset slots ship seeded from four explicit factory picks:
  `full_send`, `crush_hiccups`, `sine_smear`, and `fuzz_bloom`. Other curated
  catalog entries remain loadable over serial without stealing a front-panel
  slot.
- Long-press the reseed button to hop to the next slot or long-press reset to
  travel backwards. The selector calls the preset helpers directly
  (`selectNextStagePreset()` / `selectPreviousStagePreset()` /
  `loadStagePreset()`), so the audio pipeline stays the single source of truth
  for which dirt stages are actually hot.
- Each hop splashes an overlay: the LED bar paints the current stage mask and,
  if an OLED is wired, the top line spells out `Stack <slot>/<count>` plus
  whether a button or footswitch triggered it. Slot + mask text sticks around
  for ~2 seconds so you can sanity-check patches mid-set.
- There is no built-in preset footswitch input in the shipped hardware map.
  If you want hands-free preset navigation, treat
  `cycleDirtStackPreset(direction, /*viaFootswitch=*/true)` and
  `loadStagePreset()` as extension hooks for your own handler.

**Delay pot macro map**

- **0 – 5 %** → bypass. The firmware forces the mix dry and parks both delay taps
  at zero so you can cue phrases without the buffer lurking.
- **5 – 33 %** → slapback accelerator. A quadratic curve rockets from
  ~10 ms to ~300 ms, making the first third of the travel feel like a tight
  tape echo.
- **33 – 66 %** → ghost voice. The second tap wakes up, offset a few hundred
  milliseconds behind the main buffer, and crossfeeds between channels so the
  tail ping-pongs instead of just stacking louder.
- **66 – 100 %** → bloom. The mix leans wetter, a limiter-style swell clamps the
  return, and the feedback loop gets a gentle shove so the repeats flare without
  self-oscillating (unless you *want* that and crank the feedback pot anyway).

`src/audio_pipeline.cpp` handles the mixing and dirt engines while
`src/controls.cpp` maps the knobs and buttons to those globals. It's all plain
C++ and Arduino APIs—poke around and hack it.

### Status Display Playground

The LED bar remains the default status display. If you want more context while
debugging or performing, you can add an SSD1306 OLED on the shared I2C bus
(`18`/`19`) plus 3.3 V and ground, then enable the display code with:

```ini
; platformio.ini
build_flags =
  -DDICELOOP_ENABLE_OLED=1
  ; optionally override geometry:
  ; -DDICELOOP_OLED_WIDTH=128
  ; -DDICELOOP_OLED_HEIGHT=64
  ; -DDICELOOP_OLED_ADDRESS=0x3C
```

Once flipped on, `renderStatusUI()` paints live readouts for mix, feedback,
noise, density, and the modulation offsets coming back from
`latestChaosSnapshot()`. The final line shows how hard the chaos modulators are
tugging on the mix/feedback/fuzz/bloom/ghost gang, and the stack of slim bipolar
meters on the right turns the screen into a modulation diary: top strip is mix
offset, then feedback shove, bloom limiter gain, and ghost-send feedback. Skip
the macro and the code compiles down to the same LED-only firmware as before.

There is also a tiny tempo HUD squatting on the top row. `ui.cpp` registers a
`TempoListener` so it can cache the measured tempo period/source without
polling, convert that to BPM, and slap two badges next to the pulse meter: one
spells out **INT**, **TAP**, or **MIDI** depending on where the clock came from
and the other prints the zero-padded BPM. External taps keep the side stripe so
you know when the groove is latched to the room instead of the delay pot.

### External Tempo Sync (Tap + MIDI Clock)

Tempo lock can follow either the panel timing or an external clock. The helper
in `src/tempo_sync.cpp` currently listens for two pulse sources:

1. **Tap footswitch (pin 6).** Each time the contact closes to ground we record
   the interval, average the last four swings, and hand the result to
   `setStutterBasePeriodMs()`. That means a quick stomp instantly retunes the
   tempo-locked density windows without touching the delay pot.
2. **USB MIDI clock.** Drop 24 ppqn ticks into the Teensy over the default
   USB Audio + MIDI + Serial device profile and the firmware measures a full
   quarter note, again feeding
   the base-period setter. Start/Continue messages reset the accumulator; Stop
   clears the clock counter so stale pulses don't ghost-write a new tempo.

If neither source speaks up for ~2.5 seconds we fall back to the delay pot's
reading just like the legacy firmware. You get the best of both worlds: a stomp
box feel with DAW-tight sync when the studio rig sends clock.

The tempo glue also sprouted a tiny observer API so side projects can react to
incoming pulses without spelunking the audio engine. Call
`registerTempoListener()` with a lambda (or old-school function pointer) and
you'll get the measured period in milliseconds plus a `TempoSource` flag telling
you whether that beat came from the pot, a tap footswitch, or USB MIDI clock:

```cpp
registerTempoListener([](float periodMs, TempoSource source) {
  Serial.print("tempo @ ");
  Serial.print(60000.0f / periodMs);
  Serial.print(" bpm from ");
  switch (source) {
  case TempoSource::Tap:
    Serial.println("tap footswitch");
    break;
  case TempoSource::Midi:
    Serial.println("USB MIDI clock");
    break;
  case TempoSource::Internal:
  default:
    Serial.println("panel delay pot");
    break;
  }
});
```

Flip on `DICELOOP_ENABLE_OLED` and you'll spot the payoff immediately: the top
row now calls out `Md`, `St`, and a `Clk` badge that spells out whether the
firmware is following **INT**, **TAP**, or **MIDI** clock. The display also
prints the measured BPM (padded so you can read it mid-set) and the little
tempo pulse in the corner now carries a glyph plus stripes/diagonals to show
which clock is in charge. External taps keep the side stripe so you know when
the beat is locked to the room instead of the delay pot.

### Dirt Engine Anatomy

`processDirt()` now juggles a stack of DSP gremlins through a tiny stage
registry so you can pick who clocks in for a given gig:

- **Bit crush core** – The OG reduction to 2–8 bits still anchors the sound and
  tracks the `noiseAmount` pot.
- **Wavefold smear** – A sine-based fold keeps harmonic chaos musical. The fold
  accumulates into a short memory buffer so micro-movements of the knobs or
  chaos ladder read as motion instead of clicks.
- **Sample-and-hold stutter** – Density steers how quickly the engine freezes
  and relaunches audio grains. Slow sweeps feel like a dying tape deck; full
  tilt becomes digital chopper heaven.
  - Long-press the dual-button chord (~1.2 s) to flip the stutter into
    **tempo-lock mode**. Density stops acting like raw probability and instead
    steps through musical subdivisions (whole notes down to 32nds) referenced
    against the main delay time. The hold windows quantise to audio block
    multiples so the chops land exactly on the beat.
  - Want the chops to follow the room instead of your delay pot? Jack a
    footswitch into pin 6 or feed the Teensy MIDI clock over USB. Each tap or
    24 ppqn burst calls `setStutterBasePeriodMs()` behind the scenes so the
    density ladder locks to the external tempo.
- **Controlled fuzz** – Still polite until asked, but now promoted to a
  first-class stage. When active it sprinkles random hairs that scale with
  density and noise, then feeds the tremolo for a breathing, amp-on-the-edge
  sustain.

Those outputs are blended on-the-fly and then handed to the slow tremolo, with a
controlled fuzz stage chiming in only if you let it. The knobs stay expressive
because each axis hits a
different part of the engine: `noiseAmount` sculpts harmonic teeth, `density`
nudges the rhythmic edits, and the chaos ladder reseeds the whole mess so it
never loops in place. Crack open `src/audio_pipeline.cpp` to study the math—it's
annotated so you can rip out pieces or wire in your own machines.

Want only bit crush with no fold? Or fuzz without the stutter hiccups? The
registry exposes helper calls so your EEPROM profiles or serial scripts can
flip stages on the fly:

```cpp
// Switch to a “bit crush only” profile
setActiveDirtStages(dirtStageBit(DirtStage::BitCrush));

// Or surgically toggle by name (case-insensitive)
enableDirtStageById("stutter", false);
enableDirtStageById("fuzz", true);
```

`dirtStageId()` and `dirtStageCount()` let you enumerate the roster if you want
to present menus or save/load presets. No firmware edits required—just push a
new mask down the pipe and the gremlins obey.

### Stage-Morphing Presets

The firmware now wires that registry into a tiny preset rack living in EEPROM.
The curated stack catalog is larger than the front-panel slot count, so the box
ships with four explicit factory slots while leaving the rest of the catalog
available over serial:

- `slot 0` → `full_send`
- `slot 1` → `crush_hiccups`
- `slot 2` → `sine_smear`
- `slot 3` → `fuzz_bloom`

- **Hold the reseed button (~0.6 s)** to step *forward* through the slots.
- **Hold the reset button (~0.6 s)** to step *backward* without touching the
  chaos ladder.

Short taps still reseed/reset the ladder, so you can stoke the density/noise
grid without nuking your stage pick. The active slot survives power cycles;
stage masks are sanitised against the current registry so future firmware
updates won't load ghosts.

Need to roll your own stacks? Crack open a serial monitor at 9600 baud and use
the built-in commands:

```
preset list                     # dump masks + stage combos
preset load 2                   # instantly jump to slot 2
preset save 1 bit_crush fuzz    # overwrite slot 1 with named stages
preset mask 3 0x5               # force a mask (bits follow dirtStageBit order)
preset stack list               # show the full curated catalog
preset stack load stutter_gate  # audition a catalog stack without storing it
preset stack save 0 full_send   # reseed a slot from a named curated stack
```

`preset save` accepts the same case-insensitive IDs exposed by
`dirtStageId()`, so your EEPROM playlist stays in sync with the code comments.
`preset list` prints the active slot with a star so you can sanity check what
the buttons will morph to next, plus the factory stack each slot was originally
seeded from. Command spam is ignored gracefully—the parser only reacts when it
sees the `preset` keyword.

By default the preset layer rejects a zero dirt-stage mask, so production
builds never save a silent "(mute)" slot by accident. If you want that for bench
testing, compile with `-DDICELOOP_ALLOW_MUTED_STAGE_MASK=1` and `preset mask`
will accept `0x0`.

### Optional Chaos Modulators

Feeling brave? Hold both buttons at once to flip a hidden switch that lets a
pair of modulators ride shotgun with your knob moves. The reseed button + reset
button chord calls into `toggleChaosModulators()` and prints the new state over
serial so you always know whether the gremlins are active.

- **Tempo lock** – Keep the same chord held for about a second after the
  modulators flip and the firmware will toggle the stutter engine between the
  legacy probabilistic mode and a tempo-locked subdivision mode. The tempo is
  pulled from the current primary delay time, so dial your slapback to the song
  and the glitch gates will grid themselves automatically.

- **Mix sway** – A slow LFO nudges the wet/dry balance. Higher density means
  faster swings, so glitch hurricanes get a little extra motion.
- **Feedback drift** – A logistic-map driven offset leans the feedback mixer in
  and out of danger. It stays clamped between polite and “about to howl” so you
  can feel the tension without blowing speakers.
- **Fuzz wobble** – The fuzz amount inside `processDirt()` gets a breathing
  multiplier tied to the noise pot. Crank the noise and the grit pulses like a
  busted power rail.

All of that logic lives in `src/chaos.cpp`. The modulators stay dormant until
you ask for them, so conservative players can keep the box tight while everyone
else gets an extra dimension of instability on command.

### Teaching Notes

- **Signal levels:** The Teensy audio library pushes 16-bit signed integers in
  the range ±32767 inside `audio_block_t`. The comments in `processAudioQueues`
  spell out the conversion math so you can swap in your own fixed-point tricks.
- **Why the filter first?** `filter1` is configured as a gentle high-pass to
  keep DC out of the delay feedback path. A low, sticky offset will otherwise
  cause the delay to saturate. Change the frequency/resonance in
  `setupAudioPipeline()` and hear the difference.
- **Probability-based glitching:** `density` is interpreted as a % chance that
  we crush the current sample. This keeps the behaviour simple and makes it
  easy to swap in other distributions (Gaussian, correlated noise, etc.).
- **Feedback safety:** The explicit `constrain()` calls keep the feedback loop
  from rage quitting. Yank them if you want full self-oscillation, but do it
  intentionally.

## Build & Flash
Built with [PlatformIO](https://platformio.org/). From the repo root:

```sh
pio run          # compile
pio run -t upload  # flash it to the board
```

`platformio.ini` already targets the Teensy 4.0 and defaults to the
Audio + MIDI + Serial USB profile, so the above is enough to get code onto the
hardware.

### Native Test Bench (No Hardware Required)

Run the host-side regression suite against the Arduino/Audio stubs:

```sh
pio test -e native
```

The tests live in `tests/native/` and focus on tempo sync, chaos modulator
guardrails, and dirt-stack presets so you can demo the firmware logic on a
laptop before you grab a Teensy.

### Continuous Integration Safety Net

> **Note:** All of the real analog-to-digital work happens inside the SGTL5000
> codec on the Teensy Audio Shield. `setupAudioPipeline()` now explicitly wakes
> the chip, routes the left **line in** channel into the current mono-in delay
> graph, and leaves the MCU's
> bare-metal ADC powered down. That silicon (and the PJRC `input_adc.cpp`
> module) targets the older Kinetis parts, so we ship an `extra_script`
> (`scripts/disable_audio_adc.py`) that uses PlatformIO build middleware to
> skip compiling only that source file. The
> Teensyduino toolchain bundles a
> modern Audio library that already speaks IMXRT, so we lean on the copy PJRC
> ships with Teensyduino instead of yanking an older, Kinetis-only archive from
> the PlatformIO registry. Keeping the dependency list short means CI and local
> builds share the exact toolchain PJRC tests. If you crave the breadboard-friendly
> on-chip ADC, reach for a Teensy 3.x or port the driver to i.MXRT and ditch the
> script.

> **Heads up:** The first `pio run` after cloning will pull in the entire
> [PJRC Audio library](https://www.pjrc.com/teensy/td_libs_Audio.html). Expect a
> minute or two. Future builds are quick.

### Static Analysis Reality Check

Running `pio check -e teensy40` pumps your code through **cppcheck** as well as
several Arduino-specific linters. The report is loud because it scans every
vendor dependency pulled in from the Teensy framework/toolchain and
`.pio/libdeps/teensy40/`. Most of the
“missing return” and “member not initialised” diagnostics come from the upstream
PJRC Audio library and are outside this repo’s control. They are already proven
in hardware, so we treat them as known noise.

What actually matters for DiceLoop lives in `src/`. At the moment cppcheck only
complains about a handful of helper functions (`processAudioQueues()`,
`setupAudioPipeline()`, `setupChaos()`, etc.) when it analyzes translation units
in isolation and misses call paths that are wired in `src/main.cpp`. Those
functions are active in the live firmware; the local `__CPPCHECK__` anchors exist
to reduce this class of false positives, but not all analyzer modes honor them.

**TL;DR:** when you run the check, skim the output but focus on paths inside
`src/`. Vendor noise can be safely ignored unless you decide to fork the audio
library and fix upstream.

## Repo Tour
- `src/` – firmware sources: `main.cpp`, `audio_pipeline.cpp`, `controls.cpp`,
  `ui.cpp`, `chaos.cpp`
- `include/` – headers shared across those files
- `docs/legacy/rough.ino` – retired Arduino sketch kept for archaeology only;
  read it for context, but don't flash it thinking it's the live firmware

## Study Pointers & Further Reading

- [Teensy Audio System Design Tool](https://www.pjrc.com/teensy/gui/index.html)
  – Drag blocks, wire them up, and export Arduino code. Compare the generated
  code to our hand-crafted setup in `src/audio_pipeline.cpp`.
- [PlatformIO Teensy Docs](https://docs.platformio.org/en/latest/boards/teensy/teensy40.html)
  – Explains the board configuration you inherit via `platformio.ini`.
- [Bit crushing primer](https://ccrma.stanford.edu/~jos/filters/Bit_Reduction_Distortion.html)
  – Stanford CCRMA notes on what actually happens when you nuke bit depth.
- [Finite state machine for buttons](https://www.ganssle.com/debouncing.htm)
  – Helpful background for the non-blocking button state machine in
  `src/controls.cpp`.

## Contributing / License
I learned every trick and quick bitshift from others who were kind enough to share.
Please use this, but make that use build others.

MIT License

Copyright (c) 2025 BSSS project team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
