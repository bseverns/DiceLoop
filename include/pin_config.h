// Centralized board pin assignments.
//
// Keep all front-panel and status-UI pins here so hardware variants only need a
// single edit surface.
#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#if __has_include(<Arduino.h>)
#include <Arduino.h>
#endif

namespace pin_config {

constexpr int potDelay = A0;
constexpr int potFeedback = A1;
constexpr int potNoiseAmount = A3;
constexpr int potDensity = A4;
constexpr int potMix = A5;

constexpr int tapTempoButton = 6;
constexpr int resetButton = 7;
constexpr int reseedButton = 8;
constexpr int entropySource = 9;

constexpr int ledShiftData = 2;
constexpr int ledShiftLatch = 3;
constexpr int ledShiftClock = 4;

}  // namespace pin_config

#endif
