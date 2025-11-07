# Breadboard Layout Cheat Sheet

Use this doc when you're assembling the first proto or teaching a workshop. It's the map from BOM to breadboard.

## Core Placements
- Teensy straddles the center gap, USB port hanging off the edge for cable clearance.
- Audio shield sits on short female headers 2 rows back to leave room for jumpers.
- Pots live on a separate perf strip—wire them in with Dupont leads so you can swap quickly.

## Power Rails
- Left rail: 5 V from USB (only during bench tests).
- Right rail: 3.3 V regulator output feeding the analog section. Label the rails with tape to avoid cross-connecting.

## Signal Jumpers
- Use color coding: red for 3.3 V, black for ground, yellow for pot wipers, blue for digital controls.
- Route the shift register lines along the bottom edge to stay out of the audio path.

## TODO Shots
- [ ] Insert photo of the complete breadboard.
- [ ] Add annotated diagram once the Fritzing file is finished (`../wiring/dice-loop-wiring.fzz`).

Add your own layout variants—especially if you find a cleaner way to mount the LED bar.
