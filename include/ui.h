// LED bar UI helpers.
//
// Wrapped in their own header so other modules can update the display without
// dragging in Arduino UI specifics.
#ifndef UI
#define UI

struct ChaosSnapshot;

void setupUI();               // configure shift-register pins / optional displays
void updateLEDBar(int level); // display current chaos level (0–8 LEDs)
void renderStatusUI(int chaosLevel, bool modulatorsEnabled, float mix, float feedback,
                    float noise, float density, const ChaosSnapshot &chaosMods);

// Dirt stack selector overlay + navigation. Buttons or footswitches can nudge the
// selection forward/backward; the UI fans out to OLED + LED bar while delegating
// the actual audio routing to the preset manager.
void cycleDirtStackPreset(int direction, bool viaFootswitch);

#endif

