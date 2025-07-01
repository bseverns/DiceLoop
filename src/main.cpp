// Entry point for the Teensy Chaos Delay firmware.
// Initialises subsystems and services the main loop.
#include <Arduino.h>
#include "audio_pipeline.h"
#include "controls.h"
#include "ui.h"
#include "chaos.h"

void setup() {
  Serial.begin(9600);
  // Initialise hardware subsystems
  setupUI();
  setupControls();
  setupAudioPipeline();
  setupChaos();
}

void loop() {
  // Poll controls and then process audio buffers
  updateControl();
  processAudioQueues();
}