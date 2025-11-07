# LED Bar Reference

The LED bar is the default loop status display. This file exists so you can rewire or replace it without reverse-engineering the firmware every time.

## Segment Mapping
| Segment | Shift Register Output | Meaning in Firmware |
| --- | --- | --- |
| 1 (bottom) | QA | Input level floor |
| 2 | QB | Input level low |
| 3 | QC | Input level mid |
| 4 | QD | Input level hot |
| 5 | QE | Loop headroom |
| 6 | QF | Loop density |
| 7 | QG | Feedback warning |
| 8 | QH | Chaos modulation depth |
| 9 | QH chained or manual | Dice status |
| 10 (top) | QH chained or manual | Clip alert |

Update this map if you shuffle the order—the firmware expects the table above when it packs bits.

## Brightness Tweaks
- Default resistors: 330 Ω. Drop to 220 Ω for sunlight gigs, but log the forward voltage per color so we don't toast the bar.
- Want per-segment dimming? Document your transistor or MOSFET driver so we can replicate.

## Alternate Parts
- Kingbright DC-10GWA is stock. If you try a bi-color bar, capture which color represents what (maybe green for safe, red for chaos).

Snap macro photos of the assembled bar and stash them here. Visual references beat paragraphs.
