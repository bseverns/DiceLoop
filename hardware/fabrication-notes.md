# Fabrication Notes & Shop Lore

These are the hacks and scars collected while getting DiceLoop off the breadboard and onto stages. Treat it like a zine—add updates, cross out bad ideas, and leave vivid warnings.

## Shielding & Grounding
- **Audio Shield hums without a ground star.** Run all pot grounds back to a single lug on the power jack before hitting the Teensy ground plane. Twisted pair on the audio path helps keep the chaos oscillator from bleeding into the mix bus.
- **Enclosure paint is an insulator.** Scratch the powder coat under jack washers if you expect the box to share shield ground.

## Power Filtering
- The firmware does aggressive oversampling, so the 3.3 V rail can get noisy. A 47 µF electrolytic across 3.3 V to GND near the Teensy helps. Document any cap swaps here so the firmware maintainers can re-measure noise floors.

## Thermal Considerations
- The LED bar gets warm during long drones. Leave at least 3 mm air gap above the diffuser and list any alternative light engines you test.

## Open Questions
- Can we run the control surface off a ribbon cable longer than 200 mm without clock skew? Capture scope shots when you try.
- Is the OLED still legible in outdoor daylight gigs? Please note part numbers for polarizer film experiments.

_Add your own sections. Dates + initials make this notebook richer._
