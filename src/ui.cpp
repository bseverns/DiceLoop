// Simple LED bar UI handled via shift register pins.
// Provides setupUI() and updateLEDBar().
#include "ui.h"
#include "Arduino.h"

const int ledDataPin = 2;
const int ledLatchPin = 3;
const int ledClockPin = 4;

void setupUI() {
  // Configure shift register pins for the LED bar
  pinMode(ledDataPin, OUTPUT);
  pinMode(ledLatchPin, OUTPUT);
  pinMode(ledClockPin, OUTPUT);
}

void updateLEDBar(int level) {
  // Display the given level using a simple shifting pattern
  byte ledPattern = 0b11111111 >> (8 - level);
  digitalWrite(ledLatchPin, LOW);
  shiftOut(ledDataPin, ledClockPin, MSBFIRST, ledPattern);
  digitalWrite(ledLatchPin, HIGH);
}
