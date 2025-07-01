// Interface for reading hardware controls and exposing
// the resulting global parameters used by the audio engine.
#ifndef CONTROLS_H
#define CONTROLS_H

void setupControls();
void updateControl();

extern int noiseAmount;
extern int density;

#endif
