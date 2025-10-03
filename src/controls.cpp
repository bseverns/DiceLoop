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
#include "ui.h"
#include "Arduino.h"

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

  // Check reseed button to increase chaos level
  if (digitalRead(reseedButtonPin) == LOW) {
    delay(50);
    if (!reseedButtonState) {
      reseedButtonState = true;
      buttonPressCount++;
      if (buttonPressCount > maxChaosLevel) buttonPressCount = maxChaosLevel;
      noiseAmount = 20 + buttonPressCount * 5;
      density = 5 + buttonPressCount * 10;
      noiseAmount = constrain(noiseAmount, 20, 60);
      density = constrain(density, 5, 100);
      randomSeed(analogRead(randomSourcePin));
      updateLEDBar(buttonPressCount);
    }
  } else {
    reseedButtonState = false;
  }

  // Check reset button to clear chaos
  if (digitalRead(resetButtonPin) == LOW) {
    delay(50);
    if (!resetButtonState) {
      resetButtonState = true;
      buttonPressCount = 0;
      noiseAmount = 20;
      density = 5;
      randomSeed(analogRead(randomSourcePin));
      updateLEDBar(buttonPressCount);
    }
  } else {
    resetButtonState = false;
  }

  // === Pot handling ===
  // The Teensy 4.0 ADC reports 0–1023 for 0–3.3 V. Each map()/division converts
  // that range into something meaningful for the DSP stage.

  // Delay time: map to 1–300 ms on tap 0.
  int potDelayValue = analogRead(potDelayPin);
  delay1.delay(0, map(potDelayValue, 0, 1023, 1, 300));

  // Feedback: convert to a 0.00–1.00 linear gain, then feed into the mixer.
  int potFeedbackValue = analogRead(potFeedbackPin);
  feedbackAmount = map(potFeedbackValue, 0, 1023, 0, 100) / 100.0;
  feedbackMixer.gain(1, feedbackAmount);

  // Noise + density pots override the ladder when turned. We still keep the
  // ladder counts so the LED bar reflects the most recent button action.
  noiseAmount = map(analogRead(potNoiseAmountPin), 0, 1023, 0, 60);
  density = map(analogRead(potDensityPin), 0, 1023, 0, 100);
  mixAmount = map(analogRead(potMixPin), 0, 1023, 0, 100) / 100.0;

  // Output debug information over serial so you can watch values without a scope.
  Serial.print("Delay: "); Serial.print(potDelayValue);
  Serial.print(" | Feedback: "); Serial.print(feedbackAmount);
  Serial.print(" | Noise: "); Serial.print(noiseAmount);
  Serial.print(" | Density: "); Serial.print(density);
  Serial.print(" | Mix: "); Serial.println(mixAmount);
}

