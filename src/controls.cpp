// Hardware control surface glue code.
//
// Responsibilities:
//   • Read the five analog pots, scale them into musically useful ranges, and
//     push the values into the audio subsystem globals.
//   • Watch the two momentary buttons and ladder up/down the "chaos" state.
//   • Keep the LED bar in sync so performers can see how feral things are.
//
// Design notes live inline; treat this file as both implementation and lab
// notebook.
#include "controls.h"
#include "audio_pipeline.h"
#include "chaos.h"
#include "tempo_sync.h"
#include "ui.h"
#include "stage_presets.h"
#include "Arduino.h"

#ifndef DICELOOP_CONTROL_SERIAL_DEBUG
#define DICELOOP_CONTROL_SERIAL_DEBUG 0
#endif

const int potDelayPin = A0;
const int potFeedbackPin = A1;
const int potNoiseAmountPin = A3;
const int potDensityPin = A4;
const int potMixPin = A5;
const int reseedButtonPin = 8;
const int resetButtonPin = 7;
const int randomSourcePin = 9;  // PWM pin used as an entropy source when read

int buttonPressCount = 0;       // Tracks current chaos ladder position (0..8)
const int maxChaosLevel = 8;    // Upper bound for chaos ladder
bool reseedButtonState = false; // Edge-detect latch for reseed button
bool resetButtonState = false;  // Edge-detect latch for reset button
bool chaosChordLatched = false; // Prevent multiple toggles during dual-button hold
unsigned long chaosChordHoldStart = 0;
bool chaosChordHoldArmed = false;

unsigned long reseedPressStart = 0;
unsigned long resetPressStart = 0;
bool reseedHoldConsumed = false;
bool resetHoldConsumed = false;

constexpr unsigned long stagePresetHoldMillis = 600UL;

constexpr unsigned long tempoLockHoldMillis = 1200UL; // long-press to toggle tempo lock

// Globals exposed in controls.h so the audio pipeline can read them without
// circular dependencies. Defaults are intentionally modest so the unit powers on
// in a polite state.
int noiseAmount = 20;
int density = 5;

void setupControls() {
  // Configure buttons and random source pin. INPUT_PULLUP keeps the buttons
  // stable without external resistors (logic low means "pressed").
  pinMode(reseedButtonPin, INPUT_PULLUP);
  pinMode(resetButtonPin, INPUT_PULLUP);
  pinMode(randomSourcePin, OUTPUT);
  analogWriteFrequency(randomSourcePin, 25000);

  // Seed RNG from the PWM pin. Because we never connect anything to it, its
  // analog readback carries thermal noise that provides a slightly chaotic seed.
  randomSeed(analogRead(randomSourcePin));
}

void updateControl() {
  // === Button handling ===
  // Buttons are debounced with a primitive delay to keep the code legible for
  // beginners. See README for links if you want to replace it with a real FSM.
  bool reseedPressed = digitalRead(reseedButtonPin) == LOW;
  bool resetPressed = digitalRead(resetButtonPin) == LOW;

  // Dual-button chord toggles the optional chaos modulators.
  if (reseedPressed && resetPressed) {
    delay(50);
    bool chordStillDown =
        digitalRead(reseedButtonPin) == LOW && digitalRead(resetButtonPin) == LOW;
    if (chordStillDown) {
      // Block preset holds while the chord is active.
      reseedHoldConsumed = true;
      resetHoldConsumed = true;
      if (!reseedButtonState) {
        reseedButtonState = true;
        reseedPressStart = millis();
      }
      if (!resetButtonState) {
        resetButtonState = true;
        resetPressStart = millis();
      }
      if (!chaosChordLatched) {
        chaosChordLatched = true;
        chaosChordHoldStart = millis();
        chaosChordHoldArmed = true;
        bool enabled = toggleChaosModulators();
        Serial.print("[chaos] modulators ");
        Serial.println(enabled ? "engaged" : "bypassed");
      } else if (chaosChordHoldArmed &&
                 (millis() - chaosChordHoldStart) >= tempoLockHoldMillis) {
        chaosChordHoldArmed = false;
        StutterTimingMode mode = stutterTimingMode();
        StutterTimingMode next =
            (mode == StutterTimingMode::TempoLocked) ? StutterTimingMode::Probability
                                                     : StutterTimingMode::TempoLocked;
        setStutterTimingMode(next);
        Serial.print("[chaos] stutter tempo lock ");
        Serial.println(next == StutterTimingMode::TempoLocked ? "engaged" : "released");
      }
    }
  } else {
    chaosChordLatched = false;
    chaosChordHoldArmed = false;

    // Check reseed button to increase chaos level
    if (reseedPressed) {
      delay(50);
      if (!reseedButtonState && digitalRead(reseedButtonPin) == LOW) {
        reseedButtonState = true;
        reseedHoldConsumed = false;
        reseedPressStart = millis();
      }
      if (reseedButtonState) {
        unsigned long held = millis() - reseedPressStart;
        if (!reseedHoldConsumed && held >= stagePresetHoldMillis) {
          reseedHoldConsumed = true;
          selectNextStagePreset();
        }
      }
    } else {
      if (reseedButtonState) {
        reseedButtonState = false;
        if (!reseedHoldConsumed) {
          buttonPressCount++;
          if (buttonPressCount > maxChaosLevel) buttonPressCount = maxChaosLevel;
          noiseAmount = 20 + buttonPressCount * 5;
          density = 5 + buttonPressCount * 10;
          noiseAmount = constrain(noiseAmount, 20, 60);
          density = constrain(density, 5, 100);
          randomSeed(analogRead(randomSourcePin));
        }
      }
      reseedHoldConsumed = false;
    }

    // Check reset button to clear chaos
    if (resetPressed) {
      delay(50);
      if (!resetButtonState && digitalRead(resetButtonPin) == LOW) {
        resetButtonState = true;
        resetHoldConsumed = false;
        resetPressStart = millis();
      }
      if (resetButtonState) {
        unsigned long held = millis() - resetPressStart;
        if (!resetHoldConsumed && held >= stagePresetHoldMillis) {
          resetHoldConsumed = true;
          selectPreviousStagePreset();
        }
      }
    } else {
      if (resetButtonState) {
        resetButtonState = false;
        if (!resetHoldConsumed) {
          buttonPressCount = 0;
          noiseAmount = 20;
          density = 5;
          randomSeed(analogRead(randomSourcePin));
        }
      }
      resetHoldConsumed = false;
    }
  }

  // === Pot handling ===
  // The Teensy 4.0 ADC reports 0–1023 for 0–3.3 V. Each map()/division converts
  // that range into something meaningful for the DSP stage.

  // Delay macro: slice the knob into four scenes so performers can stage the
  // texture without menu diving. We normalise the raw ADC reading into 0..1 and
  // then fan it out across the regions described in the README and design notes.
  int potDelayValue = analogRead(potDelayPin);
  float delayNorm = static_cast<float>(potDelayValue) / 1023.0f;

  constexpr float noDelayFloor = 0.05f;      // Dead zone for instant dry
  constexpr float secondVoiceThreshold = 1.0f / 3.0f;
  constexpr float bloomThreshold = 2.0f / 3.0f;
  constexpr float maxDelayMs = 1200.0f;      // Keep taps inside delay1's buffer

  float primaryDelayMs = 1.0f;
  float secondaryDelayMs = 1.0f;
  float ghostBlend = 0.0f;
  float wetOverride = -1.0f;
  float wetBias = 0.0f;
  float bloomDepth = 0.0f;
  float bloomPush = 0.0f;

  if (delayNorm <= noDelayFloor) {
    primaryDelayMs = 0.0f;
    secondaryDelayMs = 0.0f;
    wetOverride = 0.0f;  // Force bone-dry when the pot hugs the stop
  } else if (delayNorm <= secondVoiceThreshold) {
    float regionNorm =
        (delayNorm - noDelayFloor) / (secondVoiceThreshold - noDelayFloor);
    regionNorm = constrain(regionNorm, 0.0f, 1.0f);
    float eased = regionNorm * regionNorm;  // Quick acceleration into slapback
    primaryDelayMs = 10.0f + eased * 290.0f;  // ≈10–300 ms
    secondaryDelayMs = primaryDelayMs + 90.0f;
  } else if (delayNorm <= bloomThreshold) {
    float regionNorm =
        (delayNorm - secondVoiceThreshold) / (bloomThreshold - secondVoiceThreshold);
    regionNorm = constrain(regionNorm, 0.0f, 1.0f);
    primaryDelayMs = 300.0f + regionNorm * 180.0f;          // 300–480 ms
    secondaryDelayMs = primaryDelayMs + 120.0f + regionNorm * 160.0f;
    ghostBlend = regionNorm;                                // Fade the ghost in
    wetBias = 0.05f * regionNorm;                           // Encourage wetter tails
  } else {
    float regionNorm =
        (delayNorm - bloomThreshold) / (1.0f - bloomThreshold);
    regionNorm = constrain(regionNorm, 0.0f, 1.0f);
    primaryDelayMs = 480.0f + regionNorm * 220.0f;          // 480–700 ms
    secondaryDelayMs = primaryDelayMs + 320.0f + regionNorm * 160.0f;
    ghostBlend = 1.0f;
    wetBias = 0.05f + 0.2f * regionNorm;
    bloomDepth = regionNorm;
    bloomPush = 0.25f * regionNorm;
  }

  primaryDelayMs = constrain(primaryDelayMs, 0.0f, maxDelayMs);
  secondaryDelayMs = constrain(secondaryDelayMs, 0.0f, maxDelayMs);

  delay1.delay(0, primaryDelayMs);
  delay1.delay(1, secondaryDelayMs);
  applyPotTempoBase(primaryDelayMs);
  macroMixOverride = wetOverride;
  macroWetBias = wetBias;
  secondaryVoiceLevel = ghostBlend;
  bloomAmount = bloomDepth;
  bloomFeedbackBoost = bloomPush;

  // Feedback: convert to a 0.00–1.00 linear gain, then feed into the mixer.
  int potFeedbackValue = analogRead(potFeedbackPin);
  feedbackAmount = map(potFeedbackValue, 0, 1023, 0, 100) / 100.0;
  setFeedbackGain(1, feedbackAmount);

  // Noise + density pots override the ladder when turned. We still keep the
  // ladder counts so the LED bar reflects the most recent button action.
  noiseAmount = map(analogRead(potNoiseAmountPin), 0, 1023, 0, 60);
  density = map(analogRead(potDensityPin), 0, 1023, 0, 100);
  mixAmount = map(analogRead(potMixPin), 0, 1023, 0, 100) / 100.0;

  bool modsEnabled = chaosModulatorsEnabled();

#if DICELOOP_CONTROL_SERIAL_DEBUG
  // Output debug information over serial so you can watch values without a scope.
  Serial.print("Delay: ");
  Serial.print(potDelayValue);
  Serial.print(" | Feedback: ");
  Serial.print(feedbackAmount);
  Serial.print(" | Noise: ");
  Serial.print(noiseAmount);
  Serial.print(" | Density: ");
  Serial.print(density);
  Serial.print(" | Mix: ");
  Serial.print(mixAmount);
  Serial.print(" | ChaosMods: ");
  Serial.println(modsEnabled ? "on" : "off");
  Serial.print("[stutter] mode: ");
  Serial.println(stutterTimingMode() == StutterTimingMode::TempoLocked ?
                     "tempo-locked" : "probability");
#endif

  // Pull the most recent chaos offsets so the UI mirrors what the audio engine
  // is actually doing, not just what the knobs are set to.
  ChaosSnapshot snapshot = latestChaosSnapshot();
  renderStatusUI(buttonPressCount, modsEnabled, mixAmount, feedbackAmount,
                 static_cast<float>(noiseAmount), static_cast<float>(density),
                 snapshot);
}

#ifdef __CPPCHECK__
// See src/audio_pipeline.cpp for the rationale. The helper keeps cppcheck from
// flagging our public control hooks as unused when it inspects this file solo.
[[maybe_unused]] static void __cppcheck_controls_reference() {
  setupControls();
  updateControl();
}
[[maybe_unused]] static const auto __cppcheck_controls_anchor =
    (__cppcheck_controls_reference(), 0);
#endif

