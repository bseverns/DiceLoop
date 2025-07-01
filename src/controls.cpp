// Reads pots and buttons and updates global parameters
// controlling the delay and chaos behaviour.
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
const int randomSourcePin = 9;

int buttonPressCount = 0;
const int maxChaosLevel = 8;
bool reseedButtonState = false;
bool resetButtonState = false;

int noiseAmount = 20;
int density = 5;

void setupControls() {
  // Configure buttons and random source pin
  pinMode(reseedButtonPin, INPUT_PULLUP);
  pinMode(resetButtonPin, INPUT_PULLUP);
  pinMode(randomSourcePin, OUTPUT);
  analogWriteFrequency(randomSourcePin, 25000);

  // Seed RNG from an unconnected analog pin
  randomSeed(analogRead(randomSourcePin));
}

void updateControl() {
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

  // Read pots and map them to parameter ranges
  int potDelayValue = analogRead(potDelayPin);
  delay1.delay(0, map(potDelayValue, 0, 1023, 1, 300));

  int potFeedbackValue = analogRead(potFeedbackPin);
  float feedback = map(potFeedbackValue, 0, 1023, 0, 100) / 100.0;

  noiseAmount = map(analogRead(potNoiseAmountPin), 0, 1023, 0, 60);
  density = map(analogRead(potDensityPin), 0, 1023, 0, 100);
  mixAmount = map(analogRead(potMixPin), 0, 1023, 0, 100) / 100.0;

  // Output debug information over serial
  Serial.print("Delay: "); Serial.print(potDelayValue);
  Serial.print(" | Feedback: "); Serial.print(feedback);
  Serial.print(" | Noise: "); Serial.print(noiseAmount);
  Serial.print(" | Density: "); Serial.print(density);
  Serial.print(" | Mix: "); Serial.println(mixAmount);
}
