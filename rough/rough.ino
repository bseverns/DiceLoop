//teensy chaos delay

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// === Pin Definitions ===
const int potDelayPin = A0;
const int potFeedbackPin = A1;
const int potNoiseAmountPin = A3;
const int potDensityPin = A4;
const int potMixPin = A5; // NEW: wet/dry mix control
const int randomSourcePin = 9;
const int reseedButtonPin = 8;
const int resetButtonPin = 7;
const int ledDataPin = 2;
const int ledLatchPin = 3;
const int ledClockPin = 4;

// === Chaos Parameters ===
int buttonPressCount = 0;
const int maxChaosLevel = 8;
int noiseAmount = 20;
int density = 5;
bool reseedButtonState = false;
bool resetButtonState = false;

float mixAmount = 0.5; // Wet/dry mix

// === Audio Objects ===
AudioInputI2S          i2sIn;            // Input from Teensy Audio Shield
AudioEffectDelay       delay1;
AudioFilterStateVariable filter1;
AudioPlayQueue         queueL, queueR;
AudioPlayQueue         cleanQueueL, cleanQueueR;
AudioEffectEnvelope    limiter1;
AudioOutputI2S         i2sOut;

// === Audio Connections ===
AudioConnection patchCord1(i2sIn, 0, filter1, 0);
AudioConnection patchCord2(filter1, 0, delay1, 0);
AudioConnection patchCord3(filter1, 0, cleanQueueL, 0);
AudioConnection patchCord4(filter1, 0, cleanQueueR, 0);
AudioConnection patchCord5(delay1, 0, queueL, 0);
AudioConnection patchCord6(delay1, 1, queueR, 0);
AudioConnection patchCord7(limiter1, 0, i2sOut, 0);
AudioConnection patchCord8(limiter1, 1, i2sOut, 1);

// === Function to apply dirt (bitcrush + fuzz) ===
float processDirt(float sample) {
  const int crushBits = 4;
  int crushed = int(sample * (1 << crushBits));
  float crushedSample = float(crushed) / (1 << crushBits);
  if (crushedSample > 0.8) crushedSample = 0.8;
  if (crushedSample < -0.8) crushedSample = -0.8;
  return crushedSample;
}

// === Mix clean and dirty ===
void processAudioQueues() {
  if (queueL.available() && cleanQueueL.available()) {
    audio_block_t *dirty = queueL.readBuffer();
    audio_block_t *clean = cleanQueueL.readBuffer();
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      float c = (float)clean->data[i] / 32768.0f;
      float d = (float)dirty->data[i] / 32768.0f;
      d = processDirt(d);
      float mixed = (1.0f - mixAmount) * c + mixAmount * d;
      mixed = constrain(mixed, -1.0f, 1.0f);
      clean->data[i] = (int16_t)(mixed * 32767.0f);
    }
    limiter1.input(clean, 0);
    queueL.freeBuffer();
  }

  if (queueR.available() && cleanQueueR.available()) {
    audio_block_t *dirty = queueR.readBuffer();
    audio_block_t *clean = cleanQueueR.readBuffer();
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      float c = (float)clean->data[i] / 32768.0f;
      float d = (float)dirty->data[i] / 32768.0f;
      d = processDirt(d);
      float mixed = (1.0f - mixAmount) * c + mixAmount * d;
      mixed = constrain(mixed, -1.0f, 1.0f);
      clean->data[i] = (int16_t)(mixed * 32767.0f);
    }
    limiter1.input(clean, 1);
    queueR.freeBuffer();
  }
}

void updateLEDBar() {
  byte ledPattern = 0b11111111 >> (8 - buttonPressCount);
  digitalWrite(ledLatchPin, LOW);
  shiftOut(ledDataPin, ledClockPin, MSBFIRST, ledPattern);
  digitalWrite(ledLatchPin, HIGH);
}

void setup() {
  Serial.begin(9600);

  pinMode(reseedButtonPin, INPUT_PULLUP);
  pinMode(resetButtonPin, INPUT_PULLUP);
  pinMode(ledDataPin, OUTPUT);
  pinMode(ledLatchPin, OUTPUT);
  pinMode(ledClockPin, OUTPUT);
  pinMode(randomSourcePin, OUTPUT);
  analogWriteFrequency(randomSourcePin, 25000);

  AudioMemory(60);
  delay1.delay(0, 200);
  filter1.frequency(500);
  filter1.resonance(0.7);

  limiter1.attack(5);
  limiter1.release(100);
  limiter1.hold(50);

  randomSeed(analogRead(randomSourcePin));
  updateLEDBar();
}

void updateControl() {
  if (digitalRead(reseedButtonPin) == LOW) {
    delay(50);
    if (!reseedButtonState) {
      reseedButtonState = true;
      buttonPressCount++;
      if (buttonPressCount > maxChaosLevel) buttonPressCount = maxChaosLevel;
      noiseAmount = 20 + buttonPressCount * 5;
      density = 5 + buttonPressCount * 10;
      noiseAmount = constrain(noiseAmount, 20, 60);
      density = constrain(density, 5, 100);
      randomSeed(analogRead(randomSourcePin));
      updateLEDBar();
    }
  } else {
    reseedButtonState = false;
  }

  if (digitalRead(resetButtonPin) == LOW) {
    delay(50);
    if (!resetButtonState) {
      resetButtonState = true;
      buttonPressCount = 0;
      noiseAmount = 20;
      density = 5;
      randomSeed(analogRead(randomSourcePin));
      updateLEDBar();
    }
  } else {
    resetButtonState = false;
  }

  int potDelayValue = analogRead(potDelayPin);
  delay1.delay(0, map(potDelayValue, 0, 1023, 1, 300));

  int potFeedbackValue = analogRead(potFeedbackPin);
  float feedback = map(potFeedbackValue, 0, 1023, 0, 100) / 100.0;

  noiseAmount = map(analogRead(potNoiseAmountPin), 0, 1023, 0, 60);
  density = map(analogRead(potDensityPin), 0, 1023, 0, 100);

  mixAmount = map(analogRead(potMixPin), 0, 1023, 0, 100) / 100.0;

  Serial.print("Delay: "); Serial.print(potDelayValue);
  Serial.print(" | Feedback: "); Serial.print(feedback);
  Serial.print(" | Noise: "); Serial.print(noiseAmount);
  Serial.print(" | Density: "); Serial.print(density);
  Serial.print(" | Mix: "); Serial.println(mixAmount);
}

void loop() {
  updateControl();
  processAudioQueues();
}