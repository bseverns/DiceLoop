#pragma once

#include <vector>

class UsbMidiMock {
 public:
  enum MidiType {
    InvalidType = 0,
    Clock,
    Start,
    Continue,
    Stop,
  };

  bool read() {
    if (events_.empty()) {
      current_ = InvalidType;
      return false;
    }
    current_ = events_.front();
    events_.erase(events_.begin());
    return true;
  }

  MidiType getType() const { return current_; }

  void push(MidiType type) { events_.push_back(type); }

  void reset() {
    events_.clear();
    current_ = InvalidType;
  }

 private:
  std::vector<MidiType> events_;
  MidiType current_ = InvalidType;
};

inline UsbMidiMock usbMIDI;

namespace dice_loop_stub {

inline void reset_usb_midi() { usbMIDI.reset(); }

inline void push_usb_midi_event(UsbMidiMock::MidiType type) { usbMIDI.push(type); }

}  // namespace dice_loop_stub
