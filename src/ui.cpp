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
#include "tempo_sync.h"
#include <cstdio>
#include "stage_presets.h"
#include <cmath>
#include <cstring>

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

// Cached tempo state from the tempo_sync observer so we can render badges
// without polling every frame.
float cachedTempoPeriodMs = 0.0f;
TempoSource cachedTempoSource = TempoSource::Internal;
bool tempoListenerRegistered = false;

const char *tempoSourceLabel(TempoSource source) {
  switch (source) {
  case TempoSource::Tap:
    return "TAP";
  case TempoSource::Midi:
    return "MIDI";
  case TempoSource::Internal:
  default:
    return "INT";
  }
}

char tempoSourceGlyph(TempoSource source) {
  switch (source) {
  case TempoSource::Tap:
    return 'T';
  case TempoSource::Midi:
    return 'M';
  case TempoSource::Internal:
  default:
    return 'I';
  }
}

bool tempoSourceIsExternal(TempoSource source) {
  return source == TempoSource::Tap || source == TempoSource::Midi;
}

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

// Slim bipolar meter strips so we can stack multiple chaos signals in the
// bottom row without clobbering the knob readouts. Each strip is 3 px tall with
// a centre tick marking zero.
void drawSlimSignedMeter(int y, float value, float maxMagnitude) {
  const int x = meterX();
  const int width = meterWidth();
  const int height = 3;
  if (width <= 4 || maxMagnitude <= 0.0f) {
    return;
  }
  const int innerWidth = width - 2;
  const int mid = x + width / 2;
  oled.drawRect(x, y, width, height, SSD1306_WHITE);
  oled.drawFastVLine(mid, y, height, SSD1306_WHITE);

  float normalised = constrain(value / maxMagnitude, -1.0f, 1.0f);
  int fill = static_cast<int>(roundf(std::fabs(normalised) * (innerWidth / 2.0f)));
  if (fill <= 0) {
    return;
  }
  if (fill > innerWidth / 2) {
    fill = innerWidth / 2;
  }
  if (normalised >= 0.0f) {
    oled.fillRect(mid + 1, y + 1, fill, height - 2, SSD1306_WHITE);
  } else {
    oled.fillRect(mid - fill, y + 1, fill, height - 2, SSD1306_WHITE);
  }
}

void onTempoUpdate(float periodMs, TempoSource source) {
  if (periodMs > 0.0f) {
    cachedTempoPeriodMs = periodMs;
  }
  cachedTempoSource = source;
}

void ensureTempoListenerRegistered() {
  if (tempoListenerRegistered) {
    return;
  }
  registerTempoListener(onTempoUpdate);
  cachedTempoPeriodMs = tempoSyncCurrentPeriodMs();
  cachedTempoSource = tempoSyncCurrentSource();
  tempoListenerRegistered = true;
}

float cachedTempoBpm() {
  if (cachedTempoPeriodMs <= 0.0f) {
    return 0.0f;
  }
  return 60000.0f / cachedTempoPeriodMs;
}

// Pulse indicator constants so the badge painter can respect the same geometry.
constexpr int tempoPulseSize = 7;
constexpr int tempoPulsePadding = 2;

int tempoPulseLeftEdge() {
  int x = DICELOOP_OLED_WIDTH - tempoPulseSize - tempoPulsePadding;
  if (x < 0) {
    x = 0;
  }
  return x;
}

void drawChaosMeterStack(const ChaosSnapshot &chaosMods) {
  const float values[] = {chaosMods.mixOffset,
                          chaosMods.feedbackOffset,
                          chaosMods.fuzzGain - 1.0f,
                          chaosMods.bloomDepthOffset};
  const float magnitudes[] = {0.2f, 0.4f, 0.5f, 0.5f};
  const int laneCount = 4;
  const int baseY = 20;
  const int spacing = 3; // 3 px meter stacked with a 0 px gap
  for (int i = 0; i < laneCount; ++i) {
    drawSlimSignedMeter(baseY + i * spacing, values[i], magnitudes[i]);
  }
}

int drawTempoPulse(float progress, TempoSource source, bool externalLatched) {
  const int size = tempoPulseSize;
  const int padding = tempoPulsePadding;
  int x = tempoPulseLeftEdge();
  const int y = 0;
  oled.drawRect(x, y, size, size, SSD1306_WHITE);
  progress = constrain(progress, 0.0f, 1.0f);
  if (progress < 0.25f) {
    oled.fillRect(x + 1, y + 1, size - 2, size - 2, SSD1306_WHITE);
  } else {
    int height = static_cast<int>(roundf((1.0f - progress) * (size - 2)));
    if (height < 1) {
      height = 1;
    }
    oled.fillRect(x + 1, y + 1, size - 2, height, SSD1306_WHITE);
  }
  if (tempoSourceIsExternal(source) && externalLatched && x > 0) {
    oled.drawFastVLine(x - 1, y, size, SSD1306_WHITE);
  }
  char glyph = tempoSourceGlyph(source);
  if (x >= 6) {
    oled.setCursor(x - 6, y);
    oled.print(glyph);
  }
  if (source == TempoSource::Tap) {
    oled.drawLine(x + 1, y + size - 2, x + size - 2, y + 1, SSD1306_WHITE);
  } else if (source == TempoSource::Midi) {
    oled.drawFastVLine(x + 2, y + 1, size - 2, SSD1306_WHITE);
    oled.drawFastVLine(x + size - 3, y + 1, size - 2, SSD1306_WHITE);
  }
  return x;
}

int drawTempoBadge(int rightEdge, const char *label) {
  if (!label) {
    return rightEdge;
  }
  const int padding = 1;
  const int glyphWidth = 6; // default font width at size 1
  const int glyphHeight = 8;
  int textWidth = strlen(label) * glyphWidth;
  int badgeWidth = textWidth + padding * 2;
  int left = rightEdge - badgeWidth;
  if (left < 0) {
    return rightEdge; // no room, skip drawing
  }
  oled.drawRect(left, 0, badgeWidth, glyphHeight, SSD1306_WHITE);
  oled.setCursor(left + padding, 0);
  oled.print(label);
  return left - 2; // leave a 2 px gap before the next badge
}

void drawTempoHud(float bpm, TempoSource source, bool haveTempo,
                  bool externalLatched, float pulseProgress) {
  int pulseLeft = drawTempoPulse(pulseProgress, source, externalLatched);
  int badgeCursor = pulseLeft - 2;
  char bpmLabel[8] = "--";
  if (haveTempo) {
    int bpmInt = static_cast<int>(
        roundf(constrain(bpm, 1.0f, 999.0f)));
    snprintf(bpmLabel, sizeof(bpmLabel), "%3d", bpmInt);
  }
  badgeCursor = drawTempoBadge(badgeCursor, tempoSourceLabel(source));
  drawTempoBadge(badgeCursor, bpmLabel);
}
} // namespace
#endif

const int ledDataPin = 2;
const int ledLatchPin = 3;
const int ledClockPin = 4;

namespace {
struct DirtStackSelectorState {
  bool visible = false;
  uint8_t slot = 0;
  uint8_t mask = 0;
  bool viaFootswitch = false;
  unsigned long lastChangeMs = 0;
};

constexpr unsigned long selectorOverlayDurationMs = 2000;
DirtStackSelectorState selectorState;

byte maskToLedPattern(uint8_t mask) {
  byte pattern = 0;
  const size_t stageCount = dirtStageCount();
  for (size_t i = 0; i < stageCount && i < 4; ++i) {
    uint8_t bit = dirtStageBit(static_cast<DirtStage>(i));
    if ((mask & bit) == 0) {
      continue;
    }
    int led = static_cast<int>(i) * 2; // fan the four stages across eight LEDs
    pattern |= static_cast<byte>(1u << led);
    if (led + 1 < 8) {
      pattern |= static_cast<byte>(1u << (led + 1));
    }
  }
  if (pattern == 0) {
    pattern = 0x81; // keep the bar alive even if a rogue mask sneaks in
  }
  return pattern;
}

void shiftLedPattern(byte pattern) {
  digitalWrite(ledLatchPin, LOW);
  shiftOut(ledDataPin, ledClockPin, MSBFIRST, pattern);
  digitalWrite(ledLatchPin, HIGH);
}

bool selectorActive() {
  return selectorState.visible &&
         (millis() - selectorState.lastChangeMs) < selectorOverlayDurationMs;
}

void stashSelectorState(uint8_t slot, uint8_t mask, bool viaFootswitch) {
  selectorState.visible = true;
  selectorState.slot = slot;
  selectorState.mask = mask;
  selectorState.viaFootswitch = viaFootswitch;
  selectorState.lastChangeMs = millis();
}

const char *maskToLabel(uint8_t mask) {
  static char label[64];
  label[0] = '\0';
  size_t len = 0;
  for (size_t i = 0; i < dirtStageCount(); ++i) {
    uint8_t bit = dirtStageBit(static_cast<DirtStage>(i));
    if ((mask & bit) == 0) {
      continue;
    }
    const char *id = dirtStageId(static_cast<DirtStage>(i));
    if (!id || id[0] == '\0') {
      continue;
    }
    if (len > 0 && len < sizeof(label) - 1) {
      label[len++] = '+';
    }
    size_t remaining = (len < sizeof(label)) ? sizeof(label) - len : 0;
    if (remaining > 1) {
      strncat(label + len, id, remaining - 1);
      len = strlen(label);
    }
  }
  if (len == 0) {
    strncpy(label, "(mute)", sizeof(label) - 1);
    label[sizeof(label) - 1] = '\0';
  }
  return label;
}

} // namespace

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
    // Hook into tempo_sync so we can paint source/bpm badges without polling.
    ensureTempoListenerRegistered();
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
  shiftLedPattern(ledPattern);
}

// Fan out the current control state to every UI surface we own. LED bar gets a
// blunt "how wild are we" meter; the OLED (when present) earns a richer readout
// so builders can debug patches without cracking open a serial monitor.
void renderStatusUI(int chaosLevel, bool modulatorsEnabled, float mix, float feedback,
                    float noise, float density, const ChaosSnapshot &chaosMods) {
  int level = chaosLevel;
  if (level < 0) level = 0;
  if (level > 8) level = 8;
  if (selectorActive()) {
    shiftLedPattern(maskToLedPattern(selectorState.mask));
  } else {
    updateLEDBar(modulatorsEnabled ? 8 : level);
  }

#if DICELOOP_ENABLE_OLED
  if (!oledReady) {
    return;
  }

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  ensureTempoListenerRegistered();
  if (selectorActive()) {
    uint8_t slots = stagePresetSlotCount();
    oled.setCursor(0, 0);
    oled.print("Stack ");
    oled.print(selectorState.slot + 1);
    oled.print('/');
    oled.print(slots);
    oled.print(selectorState.viaFootswitch ? " foot" : " btn");

    oled.setCursor(0, 8);
    oled.print("Mask 0x");
    oled.print(selectorState.mask, HEX);

    oled.setCursor(0, 16);
    oled.print(maskToLabel(selectorState.mask));

    oled.display();
    return;
  }

  bool tempoLocked = stutterTimingMode() == StutterTimingMode::TempoLocked;
  TempoSource tempoSource = cachedTempoSource;
  float tempoBpm = cachedTempoBpm();
  bool haveTempo = tempoBpm > 0.0f;
  float pulseProgress = tempoSyncPulseProgress();
  bool externalClockLatched = tempoSyncHasExternalClock();

  oled.setCursor(0, 0);
  oled.print("Md:");
  oled.print(modulatorsEnabled ? "ON" : "OFF");
  oled.print(' ');
  oled.print("St:");
  oled.print(tempoLocked ? "LK" : "PR");
  drawTempoHud(tempoBpm, tempoSource, haveTempo, externalClockLatched,
               pulseProgress);
  ChaosSnapshot meterMods = chaosMods;
  if (!modulatorsEnabled) {
    meterMods = ChaosSnapshot{};
  }
  drawChaosMeterStack(meterMods);

  oled.setCursor(0, 8);
  oled.print("Chs ");
  oled.print(level);
  oled.print(" Mix ");
  int mixPercent = static_cast<int>(roundf(constrain(mix, 0.0f, 1.0f) * 100.0f));
  oled.print(mixPercent);
  oled.print('%');
  oled.print(" Dn ");
  oled.print(static_cast<int>(constrain(density, 0.0f, 100.0f)));
  drawMeter(8, constrain(mix, 0.0f, 1.0f));

  oled.setCursor(0, 16);
  int feedbackPercent = static_cast<int>(roundf(constrain(feedback, 0.0f, 1.0f) * 100.0f));
  oled.print("FB ");
  oled.print(feedbackPercent);
  oled.print('%');
  oled.print(" Noi ");
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
    int limiterDelta =
        static_cast<int>(roundf((chaosMods.bloomLimiterGain - 1.0f) * 100.0f));
    oled.print(" Lm ");
    if (limiterDelta >= 0) oled.print('+');
    oled.print(limiterDelta);
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
    int ghostDelta =
        static_cast<int>(roundf(chaosMods.secondaryFeedbackOffset * 100.0f));
    oled.print(" Gh ");
    if (ghostDelta >= 0) oled.print('+');
    oled.print(ghostDelta);
    oled.print('%');
    drawChaosMeterStack(chaosMods);
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

void cycleDirtStackPreset(int direction, bool viaFootswitch) {
  auto renderActivePreset = [viaFootswitch]() {
    uint8_t slot = currentStagePresetIndex();
    uint8_t mask = stagePresetMask(slot);
    stashSelectorState(slot, mask, viaFootswitch);
    shiftLedPattern(maskToLedPattern(mask));
  };

  bool applied = false;
  if (direction > 0) {
    applied = selectNextStagePreset();
  } else if (direction < 0) {
    applied = selectPreviousStagePreset();
  } else {
    applied = loadStagePreset(currentStagePresetIndex(), true);
  }
  if (applied) {
    renderActivePreset();
  }
}

#ifdef __CPPCHECK__
// Analogous to the other modules: give cppcheck an obvious usage site so it
// keeps quiet about legitimate firmware hooks.
[[maybe_unused]] static void __cppcheck_ui_reference() {
  setupUI();
  updateLEDBar(0);
  ChaosSnapshot snapshot{};
  snapshot.fuzzGain = 1.0f;
  snapshot.bloomLimiterGain = 1.0f;
  renderStatusUI(0, false, 0.5f, 0.25f, 0.0f, 0.0f, snapshot);
}
[[maybe_unused]] static const auto __cppcheck_ui_anchor =
    (__cppcheck_ui_reference(), 0);
#endif

