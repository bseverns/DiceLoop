// External tempo sync hooks.
//
// Gives the chaos engine a way to follow outside clocks. Tap-tempo pulses and
// MIDI clock both end up calling setStutterBasePeriodMs() so the stutter window
// can quantise against the world instead of just the delay pot.
#ifndef TEMPO_SYNC_H
#define TEMPO_SYNC_H

enum class TempoSource {
  Internal,
  Tap,
  Midi,
};

using TempoListener = void (*)(float periodMs, TempoSource source);

void setupTempoSync();
void updateTempoSync();
void applyPotTempoBase(float milliseconds);

void registerTempoListener(TempoListener listener);
void unregisterTempoListener(TempoListener listener);

float tempoSyncCurrentPeriodMs();
float tempoSyncCurrentBpm();
TempoSource tempoSyncCurrentSource();
float tempoSyncPulseProgress();
bool tempoSyncHasExternalClock();

#endif
