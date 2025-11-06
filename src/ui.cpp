// LED bar user interface helper functions.
//
// The LED bar is driven through a 74HC595 shift register (or similar). We keep
// this file focused on explaining the timing: latch low → shift out byte → latch
// high. Modify `updateLEDBar` if your LED order differs or if you want fancy
// animations.
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
  // Display the given level using a simple shifting pattern. `level` is expected
  // to be 0–8. We guard against out-of-range values to avoid shifting garbage.
  if (level < 0) level = 0;
  if (level > 8) level = 8;

  // Using MSBFIRST means bit 7 maps to the LED closest to the data pin. If your
  // hardware is flipped, adjust the shift direction.
  byte ledPattern = (level == 0) ? 0 : (0xFF >> (8 - level));
  digitalWrite(ledLatchPin, LOW);
  shiftOut(ledDataPin, ledClockPin, MSBFIRST, ledPattern);
  digitalWrite(ledLatchPin, HIGH);
}

#ifdef __CPPCHECK__
// Analogous to the other modules: give cppcheck an obvious usage site so it
// keeps quiet about legitimate firmware hooks.
[[maybe_unused]] static void __cppcheck_ui_reference() {
  setupUI();
  updateLEDBar(0);
}
[[maybe_unused]] static const auto __cppcheck_ui_anchor =
    (__cppcheck_ui_reference(), 0);
#endif

