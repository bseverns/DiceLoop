# LED Bar Reference

The LED bar is the default status display. Its meaning is intentionally simple:
in normal play it shows chaos level, and during preset selection it briefly
switches to a dirt-stage mask overlay.

## Normal Mode
`renderStatusUI()` passes a `0..8` level into `updateLEDBar()`, which lights the
lowest `N` segments. When chaos modulators are enabled, the UI forces all 8
segments on as a clear "mods engaged" indicator.

## Preset Overlay Mode
When the preset selector overlay is active, the LED bar stops acting like a
level meter and instead shows the active dirt-stage mask.

| Dirt stage | LED pair | Meaning in firmware |
| --- | --- | --- |
| Bit crush | Segments 1–2 | First pair lit when `DirtStage::BitCrush` is active |
| Wavefold | Segments 3–4 | Second pair lit when `DirtStage::WaveFold` is active |
| Stutter | Segments 5–6 | Third pair lit when `DirtStage::Stutter` is active |
| Fuzz | Segments 7–8 | Fourth pair lit when `DirtStage::Fuzz` is active |

That pairing comes from `maskToLedPattern()` in `src/ui.cpp`. If you re-order
stages in code, update this table too.

## Brightness Tweaks
- Default resistors: 330 Ω. Drop to 220 Ω for sunlight gigs, but log the forward voltage per color so we don't toast the bar.
- Want per-segment dimming? Document your transistor or MOSFET driver so we can replicate.

## Alternate Parts
- Kingbright DC-10GWA is stock. If you try a bi-color bar, capture which color represents what (maybe green for safe, red for chaos).

Snap macro photos of the assembled bar and stash them here. Visual references beat paragraphs.
