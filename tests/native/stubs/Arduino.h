#pragma once

// Host-only Arduino shim for native tests.
// Keep it minimal: just enough surface area for the firmware to compile and
// for tests to poke time/pin state deterministically.

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

using boolean = bool;
using byte = uint8_t;
using word = uint16_t;

constexpr int LOW = 0;
constexpr int HIGH = 1;
constexpr int INPUT = 0;
constexpr int OUTPUT = 1;
constexpr int INPUT_PULLUP = 2;
constexpr int MSBFIRST = 1;
constexpr int LSBFIRST = 0;

namespace dice_loop_stub {

inline unsigned long millis_value = 0;
inline unsigned long micros_value = 0;
inline int digital_pins[64] = {HIGH};
inline int analog_pins[64] = {0};
inline int pin_modes[64] = {INPUT};

inline void reset_state() {
    millis_value = 0;
    micros_value = 0;
    std::fill(std::begin(digital_pins), std::end(digital_pins), HIGH);
    std::fill(std::begin(analog_pins), std::end(analog_pins), 0);
    std::fill(std::begin(pin_modes), std::end(pin_modes), INPUT);
}

inline void set_millis(unsigned long value) { millis_value = value; }
inline void set_micros(unsigned long value) { micros_value = value; }

inline void advance_millis(unsigned long delta) {
    millis_value += delta;
    micros_value += delta * 1000UL;
}

inline void advance_micros(unsigned long delta) {
    micros_value += delta;
    millis_value += delta / 1000UL;
}

inline void set_digital_pin(uint8_t pin, int value) {
    if (pin < 64) {
        digital_pins[pin] = value;
    }
}

inline void set_analog_pin(uint8_t pin, int value) {
    if (pin < 64) {
        analog_pins[pin] = value;
    }
}

}  // namespace dice_loop_stub

inline unsigned long millis() { return dice_loop_stub::millis_value; }
inline unsigned long micros() { return dice_loop_stub::micros_value; }

inline void delay(unsigned long ms) { dice_loop_stub::advance_millis(ms); }

inline void pinMode(uint8_t pin, uint8_t mode) {
    if (pin < 64) {
        dice_loop_stub::pin_modes[pin] = mode;
    }
}

inline int digitalRead(uint8_t pin) {
    if (pin < 64) {
        return dice_loop_stub::digital_pins[pin];
    }
    return LOW;
}

inline void digitalWrite(uint8_t pin, uint8_t value) {
    if (pin < 64) {
        dice_loop_stub::digital_pins[pin] = value;
    }
}

inline int analogRead(uint8_t pin) {
    if (pin < 64) {
        return dice_loop_stub::analog_pins[pin];
    }
    return 0;
}

inline void analogWriteFrequency(uint8_t, uint32_t) {}

inline void randomSeed(unsigned long seed) { std::srand(static_cast<unsigned int>(seed)); }

inline long random(long max) {
    if (max <= 0) {
        return 0;
    }
    return std::rand() % max;
}

inline long random(long min, long max) {
    if (max <= min) {
        return min;
    }
    return min + (std::rand() % (max - min));
}

template <typename T>
inline T constrain(T x, T lo, T hi) {
    return (x < lo) ? lo : (x > hi ? hi : x);
}

template <typename T>
inline constexpr const T &min(const T &a, const T &b) {
    return (a < b) ? a : b;
}

template <typename T>
inline constexpr const T &max(const T &a, const T &b) {
    return (a > b) ? a : b;
}

inline void shiftOut(uint8_t, uint8_t, uint8_t, uint8_t) {}

class SerialMock {
  public:
    void begin(unsigned long) {}
    int available() const { return 0; }
    int read() { return -1; }
    void print(const char *) {}
    void print(int) {}
    void print(unsigned long, int = 10) {}
    void print(float, int = 2) {}
    void println(const char * = "") {}
    void println(int) {}
    void println(unsigned long, int = 10) {}
    void println(float, int = 2) {}
};

inline SerialMock Serial;
