// teensyDee2 firmware entry point.
//
// This file intentionally stays tiny: it documents the overall boot flow and
// leaves the heavy lifting to the subsystems in `src/`. Treat it as the
// high-level syllabus for the firmware.
//
// Boot order matters on the Teensy audio stack, so we call `setupUI()` first to
// ensure indicator LEDs don't flicker garbage, wire up controls, allocate audio
// memory, and only then prime the chaos helpers.
#include <Arduino.h>
#include "audio_pipeline.h"
#include "controls.h"
#include "ui.h"
#include "chaos.h"

void setup() {
  Serial.begin(9600);
  // Initialise hardware subsystems. Each setup routine documents its own
  // dependencies and side effects so you can reorder or swap components on your
  // own builds.
  setupUI();
  setupControls();
  setupAudioPipeline();
  setupChaos();
}

void loop() {
  // Poll controls to refresh globals (pot values, button presses, chaos state)
  // and then drain audio queues. The order keeps latency low: we always process
  // control changes before pushing a fresh audio block.
  updateControl();
  processAudioQueues();
}

