// External tempo sync hooks.
//
// Gives the chaos engine a way to follow outside clocks. Tap-tempo pulses and
// MIDI clock both end up calling setStutterBasePeriodMs() so the stutter window
// can quantise against the world instead of just the delay pot.
#ifndef TEMPO_SYNC_H
#define TEMPO_SYNC_H

void setupTempoSync();
void updateTempoSync();
void applyPotTempoBase(float milliseconds);

#endif
