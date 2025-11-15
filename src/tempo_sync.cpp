// External tempo sync implementation.
//
// Think of this as a translator between the outside world's clocks and the
// stutter engine. A footswitch tap or a MIDI clock tick updates the base period
// used by setStutterBasePeriodMs(), which in turn keeps tempo-locked stutters
// glued to whatever groove the rig is following.
#include "tempo_sync.h"

#include "audio_pipeline.h"
#include <Arduino.h>

#if defined(USB_MIDI) || defined(USB_AUDIO) || defined(USB_MIDI_SERIAL)
#include <usbMIDI.h>
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

unsigned long lastTapMillis = 0;
unsigned long tapIntervals[tapAverageWindow] = {0};
size_t tapIntervalCount = 0;
size_t tapIntervalIndex = 0;
bool lastTapState = HIGH;

bool externalTempoLatched = false;
unsigned long lastExternalUpdate = 0;

#if DICELOOP_TEMPO_HAVE_USB_MIDI
constexpr uint8_t midiClocksPerQuarter = 24;
constexpr unsigned long midiClockTimeoutMicros = 2000000UL; // 2 s timeout
unsigned long midiClockWindowStart = 0;
unsigned long lastMidiClockMicros = 0;
uint8_t midiClockCount = 0;
#endif

void noteExternalTempo(float milliseconds) {
  if (milliseconds <= 0.0f) {
    return;
  }
  setStutterBasePeriodMs(milliseconds);
  externalTempoLatched = true;
  lastExternalUpdate = millis();
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
    return;
  }
  unsigned long now = millis();
  if (externalTempoLatched &&
      (now - lastExternalUpdate) <= tapDecayMillis) {
    return; // stay married to the external clock while it's fresh
  }
  externalTempoLatched = false;
  setStutterBasePeriodMs(milliseconds);
}
