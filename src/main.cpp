#include <Arduino.h>
#include "audio_pipeline.h"
#include "controls.h"
#include "ui.h"
#include "chaos.h"

void setup() {
  Serial.begin(9600);
  setupUI();
  setupControls();
  setupAudioPipeline();
  setupChaos();
}

void loop() {
  updateControl();
  processAudioQueues();
}