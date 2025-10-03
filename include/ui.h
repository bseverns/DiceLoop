// LED bar UI helpers.
//
// Wrapped in their own header so other modules can update the display without
// dragging in Arduino UI specifics.
#ifndef UI
#define UI

void setupUI();               // configure shift-register pins
void updateLEDBar(int level); // display current chaos level (0–8 LEDs)

#endif

