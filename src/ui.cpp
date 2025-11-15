// LED bar user interface helper functions.
//
// The LED bar is driven through a 74HC595 shift register (or similar). We keep
// this file focused on explaining the timing: latch low → shift out byte → latch
// high. Modify `updateLEDBar` if your LED order differs or if you want fancy
// animations.
#include "ui.h"
#include "Arduino.h"
#include "audio_pipeline.h"
#include "chaos.h"
#include <cmath>

#ifndef DICELOOP_ENABLE_OLED
#define DICELOOP_ENABLE_OLED 0
#endif

#if DICELOOP_ENABLE_OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#ifndef DICELOOP_OLED_WIDTH
#define DICELOOP_OLED_WIDTH 128
#endif

#ifndef DICELOOP_OLED_HEIGHT
#define DICELOOP_OLED_HEIGHT 32
#endif

#ifndef DICELOOP_OLED_ADDRESS
#define DICELOOP_OLED_ADDRESS 0x3C
#endif

namespace {
Adafruit_SSD1306 oled(DICELOOP_OLED_WIDTH, DICELOOP_OLED_HEIGHT, &Wire, -1);
bool oledReady = false;

// Return the x coordinate where meters should start so text on the left has
// breathing room. It's intentionally scrappy—good enough for 128 px screens,
// still sane when someone bolts on a postage-stamp display.
int meterX() {
  if (DICELOOP_OLED_WIDTH >= 128) {
    return 64;
  }
  if (DICELOOP_OLED_WIDTH >= 96) {
    return 48;
  }
  return DICELOOP_OLED_WIDTH / 2;
}

// Compute how wide we can draw a meter given the current display width. The
// OLED helper bails out gracefully when there simply isn't room instead of
// scribbling off-screen garbage.
int meterWidth() {
  int width = DICELOOP_OLED_WIDTH - meterX() - 4;
  if (width < 0) {
    width = 0;
  }
  return width;
}

// Draw a 0..1 linear meter. It mirrors the LED bar semantics: 0 means empty,
// 1 means full blast. The meters stay monochrome so the OLED doesn't fight the
// punk aesthetic of the front panel.
void drawMeter(int y, float value) {
  value = constrain(value, 0.0f, 1.0f);
  const int x = meterX();
  const int width = meterWidth();
  const int height = 6;
  if (width <= 4) {
    return; // not enough room to draw a readable meter
  }
  const int innerWidth = width - 2;
  int fill = static_cast<int>(roundf(value * innerWidth));
  oled.drawRect(x, y, width, height, SSD1306_WHITE);
  if (fill > 0) {
    if (fill > innerWidth) fill = innerWidth;
    oled.fillRect(x + 1, y + 1, fill, height - 2, SSD1306_WHITE);
  }
}

// Draw a bipolar meter, centred around zero. Useful for showing chaos
// modulator offsets so you can tell at a glance when the firmware is nudging
// mix/feedback above or below the knob positions.
void drawSignedMeter(int y, float value, float maxMagnitude) {
  const int x = meterX();
  const int width = meterWidth();
  const int height = 6;
  if (width <= 4) {
    return;
  }
  const int innerWidth = width - 2;
  oled.drawRect(x, y, width, height, SSD1306_WHITE);
  int mid = x + width / 2;
  oled.drawFastVLine(mid, y, height, SSD1306_WHITE);
  if (maxMagnitude <= 0.0f) {
    return;
  }
  float normalised = constrain(value / maxMagnitude, -1.0f, 1.0f);
  if (normalised > 0.0f) {
    int fill = static_cast<int>(roundf(normalised * (innerWidth / 2.0f)));
    if (fill > 0) {
      if (fill > innerWidth / 2) fill = innerWidth / 2;
      oled.fillRect(mid + 1, y + 1, fill, height - 2, SSD1306_WHITE);
    }
  } else if (normalised < 0.0f) {
    int fill = static_cast<int>(roundf(-normalised * (innerWidth / 2.0f)));
    if (fill > 0) {
      if (fill > innerWidth / 2) fill = innerWidth / 2;
      oled.fillRect(mid - fill, y + 1, fill, height - 2, SSD1306_WHITE);
    }
  }
}
} // namespace
#endif

const int ledDataPin = 2;
const int ledLatchPin = 3;
const int ledClockPin = 4;

void setupUI() {
  // Configure shift register pins for the LED bar
  pinMode(ledDataPin, OUTPUT);
  pinMode(ledLatchPin, OUTPUT);
  pinMode(ledClockPin, OUTPUT);

#if DICELOOP_ENABLE_OLED
  Wire.begin();
  if (!oled.begin(SSD1306_SWITCHCAPVCC, DICELOOP_OLED_ADDRESS)) {
    Serial.println("[ui] oled display missing or angry – sticking to LEDs");
    oledReady = false;
  } else {
    oledReady = true;
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    oled.println("DiceLoop booting");
    oled.println("mods idle, twist knobs");
    oled.display();
  }
#endif
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

// Fan out the current control state to every UI surface we own. LED bar gets a
// blunt "how wild are we" meter; the OLED (when present) earns a richer readout
// so builders can debug patches without cracking open a serial monitor.
void renderStatusUI(int chaosLevel, bool modulatorsEnabled, float mix, float feedback,
                    float noise, float density, const ChaosSnapshot &chaosMods) {
  int level = chaosLevel;
  if (level < 0) level = 0;
  if (level > 8) level = 8;
  updateLEDBar(modulatorsEnabled ? 8 : level);

#if DICELOOP_ENABLE_OLED
  if (!oledReady) {
    return;
  }

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  bool tempoLocked = stutterTimingMode() == StutterTimingMode::TempoLocked;

  oled.setCursor(0, 0);
  oled.print("Chaos ");
  oled.print(level);
  oled.print("  Mods ");
  oled.print(modulatorsEnabled ? "ON" : "OFF");
  oled.print("  Stut ");
  oled.print(tempoLocked ? "LOCK" : "PROB");

  oled.setCursor(0, 8);
  int mixPercent = static_cast<int>(roundf(constrain(mix, 0.0f, 1.0f) * 100.0f));
  oled.print("Mix ");
  oled.print(mixPercent);
  oled.print('%');
  oled.print("  Den ");
  oled.print(static_cast<int>(constrain(density, 0.0f, 100.0f)));
  drawMeter(8, constrain(mix, 0.0f, 1.0f));

  oled.setCursor(0, 16);
  int feedbackPercent = static_cast<int>(roundf(constrain(feedback, 0.0f, 1.0f) * 100.0f));
  oled.print("FB  ");
  oled.print(feedbackPercent);
  oled.print('%');
  oled.print("  Noi ");
  oled.print(static_cast<int>(constrain(noise, 0.0f, 60.0f)));
  drawMeter(16, constrain(feedback, 0.0f, 1.0f));

  oled.setCursor(0, 24);
  if (modulatorsEnabled) {
    int mixDelta = static_cast<int>(roundf(chaosMods.mixOffset * 100.0f));
    int fbDelta = static_cast<int>(roundf(chaosMods.feedbackOffset * 100.0f));
    int fuzzDelta = static_cast<int>(roundf((chaosMods.fuzzGain - 1.0f) * 100.0f));
    int bloomDelta =
        static_cast<int>(roundf(chaosMods.bloomDepthOffset * 100.0f));
    int panDelta = static_cast<int>(roundf(chaosMods.secondaryVoicePan * 100.0f));
    oled.print("dMix ");
    if (mixDelta >= 0) oled.print('+');
    oled.print(mixDelta);
    oled.print('%');
    oled.print(" dFB ");
    if (fbDelta >= 0) oled.print('+');
    oled.print(fbDelta);
    oled.print('%');
    oled.print(" Fz ");
    if (fuzzDelta >= 0) oled.print('+');
    oled.print(fuzzDelta);
    oled.print('%');
    oled.print(" Bl ");
    if (bloomDelta >= 0) oled.print('+');
    oled.print(bloomDelta);
    oled.print('%');
    oled.print(" Pn ");
    if (panDelta >= 0) {
      oled.print('R');
    } else {
      oled.print('L');
      panDelta = -panDelta;
    }
    oled.print(panDelta);
    oled.print('%');
    drawSignedMeter(24, chaosMods.mixOffset, 0.2f);
  } else {
    oled.print("Chord=chaos, hold=tempo");
  }

  oled.display();
#else
  (void)chaosMods;
  (void)mix;
  (void)feedback;
  (void)noise;
  (void)density;
#endif
}

#ifdef __CPPCHECK__
// Analogous to the other modules: give cppcheck an obvious usage site so it
// keeps quiet about legitimate firmware hooks.
[[maybe_unused]] static void __cppcheck_ui_reference() {
  setupUI();
  updateLEDBar(0);
  renderStatusUI(0, false, 0.5f, 0.25f, 0.0f, 0.0f, ChaosSnapshot{});
}
[[maybe_unused]] static const auto __cppcheck_ui_anchor =
    (__cppcheck_ui_reference(), 0);
#endif

