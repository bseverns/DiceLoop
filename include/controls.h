// Control surface public API.
//
// The implementation keeps live state in globals so the audio pipeline can grab
// the latest values each audio block without function call overhead.
#ifndef CONTROLS_H
#define CONTROLS_H

void setupControls();
void updateControl();

extern int noiseAmount; // 0–60 bit-crusher intensity scalar
extern int density;     // 0–100 probability (%) that a sample gets crushed

#endif

