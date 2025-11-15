// External tempo sync implementation.
//
// Think of this as a translator between the outside world's clocks and the
// stutter engine. A footswitch tap or a MIDI clock tick updates the base period
// used by setStutterBasePeriodMs(), which in turn keeps tempo-locked stutters
// glued to whatever groove the rig is following.
#include "tempo_sync.h"

#include "audio_pipeline.h"
#include <Arduino.h>
#include <cmath>

using std::fabsf;
using std::fmodf;

#if defined(USB_MIDI) || defined(USB_MIDI_SERIAL) ||                               \
    defined(USB_MIDI_AUDIO_SERIAL) || defined(USB_AUDIO_MIDI_SERIAL) ||            \
    defined(USB_MIDI4_SERIAL) || defined(USB_MIDI16_SERIAL) ||                     \
    defined(USB_EVERYTHING)
#include <usb_midi.h>
#define DICELOOP_TEMPO_HAVE_USB_MIDI 1
#else
#define DICELOOP_TEMPO_HAVE_USB_MIDI 0
#endif

namespace {
constexpr int tapTempoPin = 6;             // Spare digital pin for tap footswitch
constexpr unsigned long minTapInterval = 60;   // ~1000 BPM ceiling guardrail
constexpr unsigned long maxTapInterval = 2000; // ~30 BPM floor before we reset
constexpr unsigned long tapDecayMillis = 2500; // fall back to pot tempo after idle
constexpr size_t tapAverageWindow = 4;         // rolling average to calm jitter

constexpr size_t tempoListenerSlots = 4;       // handful of hooks for UI/loggers

unsigned long lastTapMillis = 0;
unsigned long tapIntervals[tapAverageWindow] = {0};
size_t tapIntervalCount = 0;
size_t tapIntervalIndex = 0;
bool lastTapState = HIGH;

bool externalTempoLatched = false;
unsigned long lastExternalUpdate = 0;
float lastTempoPeriodMs = 0.0f;
TempoSource currentTempoSource = TempoSource::Internal;
unsigned long lastTempoPulseMillis = 0;
TempoListener tempoListeners[tempoListenerSlots] = {nullptr};

#if DICELOOP_TEMPO_HAVE_USB_MIDI
constexpr uint8_t midiClocksPerQuarter = 24;
constexpr unsigned long midiClockTimeoutMicros = 2000000UL; // 2 s timeout
unsigned long midiClockWindowStart = 0;
unsigned long lastMidiClockMicros = 0;
uint8_t midiClockCount = 0;
#endif

void notifyTempoListeners(float periodMs, TempoSource source, bool forceFire) {
  bool sourceChanged = (source != currentTempoSource);
  bool periodChanged = fabsf(periodMs - lastTempoPeriodMs) > 0.01f;

  lastTempoPeriodMs = periodMs;
  currentTempoSource = source;

  if (forceFire || sourceChanged || periodChanged) {
    for (TempoListener &slot : tempoListeners) {
      if (slot) {
        slot(periodMs, source);
      }
    }
  }
}

void noteExternalTempo(float milliseconds) {
  if (milliseconds <= 0.0f) {
    return;
  }
  setStutterBasePeriodMs(milliseconds);
  externalTempoLatched = true;
  unsigned long now = millis();
  lastExternalUpdate = now;
  lastTempoPulseMillis = now;
  notifyTempoListeners(milliseconds, TempoSource::External, true);
}

void resetTapAverager() {
  tapIntervalCount = 0;
  tapIntervalIndex = 0;
  for (size_t i = 0; i < tapAverageWindow; ++i) {
    tapIntervals[i] = 0;
  }
}

void updateTapTempo() {
  bool currentState = digitalRead(tapTempoPin);
  if (lastTapState == HIGH && currentState == LOW) {
    unsigned long now = millis();
    if (lastTapMillis != 0) {
      unsigned long interval = now - lastTapMillis;
      if (interval < minTapInterval || interval > maxTapInterval) {
        resetTapAverager();
      } else {
        tapIntervals[tapIntervalIndex] = interval;
        tapIntervalIndex = (tapIntervalIndex + 1) % tapAverageWindow;
        if (tapIntervalCount < tapAverageWindow) {
          ++tapIntervalCount;
        }
        unsigned long sum = 0;
        for (size_t i = 0; i < tapIntervalCount; ++i) {
          sum += tapIntervals[i];
        }
        float averaged = static_cast<float>(sum) /
                          static_cast<float>(tapIntervalCount);
        noteExternalTempo(averaged);
      }
    }
    lastTapMillis = now;
  }
  lastTapState = currentState;
}

#if DICELOOP_TEMPO_HAVE_USB_MIDI
void handleMidiClockMessage() {
  unsigned long nowMicros = micros();
  if (lastMidiClockMicros != 0 &&
      (nowMicros - lastMidiClockMicros) > midiClockTimeoutMicros) {
    midiClockCount = 0;
  }
  lastMidiClockMicros = nowMicros;

  if (midiClockCount == 0) {
    midiClockWindowStart = nowMicros;
  }
  ++midiClockCount;

  if (midiClockCount >= midiClocksPerQuarter) {
    unsigned long elapsed = nowMicros - midiClockWindowStart;
    float quarterMs = static_cast<float>(elapsed) / 1000.0f;
    noteExternalTempo(quarterMs);
    midiClockCount = 0;
    midiClockWindowStart = nowMicros;
  }
}

void pollUsbMidi() {
  while (usbMIDI.read()) {
    auto type = usbMIDI.getType();
    if (type == usbMIDI.Clock) {
      handleMidiClockMessage();
    } else if (type == usbMIDI.Start || type == usbMIDI.Continue) {
      midiClockCount = 0;
      midiClockWindowStart = micros();
      lastMidiClockMicros = midiClockWindowStart;
    } else if (type == usbMIDI.Stop) {
      midiClockCount = 0;
    }
  }
}
#endif

} // namespace

void setupTempoSync() {
  pinMode(tapTempoPin, INPUT_PULLUP);
  lastTapState = digitalRead(tapTempoPin);
  resetTapAverager();
  externalTempoLatched = false;
  lastExternalUpdate = 0;
  lastTempoPeriodMs = 0.0f;
  currentTempoSource = TempoSource::Internal;
  lastTempoPulseMillis = 0;
  for (TempoListener &slot : tempoListeners) {
    slot = nullptr;
  }
#if DICELOOP_TEMPO_HAVE_USB_MIDI
  midiClockCount = 0;
  midiClockWindowStart = 0;
  lastMidiClockMicros = 0;
#endif
}

void updateTempoSync() {
  updateTapTempo();
#if DICELOOP_TEMPO_HAVE_USB_MIDI
  pollUsbMidi();
#endif
}

void applyPotTempoBase(float milliseconds) {
  if (milliseconds <= 0.0f) {
    setStutterBasePeriodMs(milliseconds);
    externalTempoLatched = false;
    currentTempoSource = TempoSource::Internal;
    return;
  }
  unsigned long now = millis();
  if (externalTempoLatched &&
      (now - lastExternalUpdate) <= tapDecayMillis) {
    return; // stay married to the external clock while it's fresh
  }
  externalTempoLatched = false;
  setStutterBasePeriodMs(milliseconds);
  lastTempoPulseMillis = now;
  notifyTempoListeners(milliseconds, TempoSource::Internal, false);
}

void registerTempoListener(TempoListener listener) {
  if (!listener) {
    return;
  }
  for (TempoListener &slot : tempoListeners) {
    if (slot == listener) {
      return; // already registered
    }
  }
  for (TempoListener &slot : tempoListeners) {
    if (!slot) {
      slot = listener;
      return;
    }
  }
}

void unregisterTempoListener(TempoListener listener) {
  if (!listener) {
    return;
  }
  for (TempoListener &slot : tempoListeners) {
    if (slot == listener) {
      slot = nullptr;
    }
  }
}

float tempoSyncCurrentPeriodMs() { return lastTempoPeriodMs; }

float tempoSyncCurrentBpm() {
  if (lastTempoPeriodMs <= 0.0f) {
    return 0.0f;
  }
  return 60000.0f / lastTempoPeriodMs;
}

TempoSource tempoSyncCurrentSource() { return currentTempoSource; }

float tempoSyncPulseProgress() {
  if (lastTempoPeriodMs <= 0.0f || lastTempoPulseMillis == 0) {
    return 0.0f;
  }
  unsigned long now = millis();
  unsigned long elapsed = now - lastTempoPulseMillis;
  float period = lastTempoPeriodMs;
  if (period <= 0.0f) {
    return 0.0f;
  }
  float progress = fmodf(static_cast<float>(elapsed), period) / period;
  if (progress < 0.0f) {
    progress = 0.0f;
  }
  return progress;
}

bool tempoSyncHasExternalClock() { return externalTempoLatched; }
