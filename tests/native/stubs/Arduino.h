#pragma once

// Host-only Arduino shim for native tests.
// Keep it minimal: just enough surface area for the firmware to compile and
// for tests to poke time/pin state deterministically.

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <deque>
#include <string>

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
constexpr int DEC = 10;
constexpr int HEX = 16;

constexpr int A0 = 14;
constexpr int A1 = 15;
constexpr int A2 = 16;
constexpr int A3 = 17;
constexpr int A4 = 18;
constexpr int A5 = 19;

namespace dice_loop_stub {

inline unsigned long millis_value = 0;
inline unsigned long micros_value = 0;
inline int digital_pins[64] = {HIGH};
inline int analog_pins[64] = {0};
inline int pin_modes[64] = {INPUT};
inline std::deque<int> serial_input;
inline std::string serial_output;
inline uint32_t random_state = 1u;

inline void reset_state() {
    millis_value = 0;
    micros_value = 0;
    std::fill(digital_pins, digital_pins + 64, HIGH);
    std::fill(analog_pins, analog_pins + 64, 0);
    std::fill(pin_modes, pin_modes + 64, INPUT);
    serial_input.clear();
    serial_output.clear();
    random_state = 1u;
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

inline void push_serial_input(const char *text) {
    if (!text) {
        return;
    }
    while (*text) {
        serial_input.push_back(static_cast<unsigned char>(*text));
        ++text;
    }
}

inline void clear_serial_output() { serial_output.clear(); }

inline const std::string &serial_output_text() { return serial_output; }

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

inline void randomSeed(unsigned long seed) {
    dice_loop_stub::random_state = static_cast<uint32_t>(seed);
    if (dice_loop_stub::random_state == 0u) {
        dice_loop_stub::random_state = 1u;
    }
}

inline uint32_t next_random_state() {
    dice_loop_stub::random_state =
        dice_loop_stub::random_state * 1664525u + 1013904223u;
    return dice_loop_stub::random_state;
}

inline long random(long max) {
    if (max <= 0) {
        return 0;
    }
    return static_cast<long>(next_random_state() % static_cast<uint32_t>(max));
}

inline long random(long min, long max) {
    if (max <= min) {
        return min;
    }
    return min + random(max - min);
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

inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    if (in_max == in_min) {
        return out_min;
    }
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

class SerialMock {
  public:
    void begin(unsigned long) {}
    int available() const { return static_cast<int>(dice_loop_stub::serial_input.size()); }
    int read() {
        if (dice_loop_stub::serial_input.empty()) {
            return -1;
        }
        int value = dice_loop_stub::serial_input.front();
        dice_loop_stub::serial_input.pop_front();
        return value;
    }
    void print(const char *value) {
        if (value) {
            dice_loop_stub::serial_output += value;
        }
    }
    void print(int value) { dice_loop_stub::serial_output += std::to_string(value); }
    void print(uint8_t value, int base = 10) { print(static_cast<unsigned long>(value), base); }
    void print(unsigned long value, int base = 10) {
        char buffer[32];
        if (base == HEX) {
            std::snprintf(buffer, sizeof(buffer), "%lx", value);
        } else {
            std::snprintf(buffer, sizeof(buffer), "%lu", value);
        }
        dice_loop_stub::serial_output += buffer;
    }
    void print(float value, int precision = 2) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%.*f", precision, static_cast<double>(value));
        dice_loop_stub::serial_output += buffer;
    }
    void println(const char *value = "") {
        print(value);
        dice_loop_stub::serial_output += '\n';
    }
    void println(int value) {
        print(value);
        dice_loop_stub::serial_output += '\n';
    }
    void println(unsigned long value, int base = 10) {
        print(value, base);
        dice_loop_stub::serial_output += '\n';
    }
    void println(float value, int precision = 2) {
        print(value, precision);
        dice_loop_stub::serial_output += '\n';
    }
};

inline SerialMock Serial;
