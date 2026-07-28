// telemetry_buffer.cpp — Umsetzung Ringpuffer. Rein, hardwarefrei.
#include "telemetry_buffer.h"
#include <cstring>

namespace logic {

void TelemetryBuffer::push(const uint8_t* frame_bytes) {
  std::memcpy(frames_[head_], frame_bytes, TELEMETRY_FRAME_SIZE);
  head_ = (head_ + 1) % RINGBUFFER_FRAMES;
  if (count_ == RINGBUFFER_FRAMES) {
    // Puffer war schon voll: der Schreibzeiger hat soeben das aelteste
    // Element ueberschrieben (FR-TEL-04) -- Lesezeiger nachziehen.
    tail_ = (tail_ + 1) % RINGBUFFER_FRAMES;
  } else {
    count_++;
  }
}

bool TelemetryBuffer::pop(uint8_t* out) {
  if (count_ == 0) {
    return false;
  }
  std::memcpy(out, frames_[tail_], TELEMETRY_FRAME_SIZE);
  tail_ = (tail_ + 1) % RINGBUFFER_FRAMES;
  count_--;
  return true;
}

}  // namespace logic
