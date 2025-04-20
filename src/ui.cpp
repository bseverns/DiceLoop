#include "ui.h"
#include "Arduino.h"

const int ledDataPin = 2;
const int ledLatchPin = 3;
const int ledClockPin = 4;

void setupUI() {
  pinMode(ledDataPin, OUTPUT);
  pinMode(ledLatchPin, OUTPUT);
  pinMode(ledClockPin, OUTPUT);
}

void updateLEDBar(int level) {
  byte ledPattern = 0b11111111 >> (8 - level);
  digitalWrite(ledLatchPin, LOW);
  shiftOut(ledDataPin, ledClockPin, MSBFIRST, ledPattern);
  digitalWrite(ledLatchPin, HIGH);
}
