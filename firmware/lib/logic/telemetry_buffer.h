// telemetry_buffer.h — RAM-Ringpuffer fuer serialisierte Telemetrie-Frames
// (FR-TEL-04, NFR-RES-01). REINE LOGIK, hardwarefrei (NFR-TST-01): kein
// #include <Arduino.h>. Statisch dimensioniert (NFR-RES-02: keine
// dynamische Allokation) -- kennt telemetry_frame.h bewusst NICHT, arbeitet
// nur auf festgroessen Byte-Bloecken (Entkopplung, NFR-EXT-01). Host-testbar
// (siehe firmware/test/test_telemetry_buffer/).
#pragma once
#include <cstdint>
#include <cstddef>
#include "config.h"
#include "telemetry_frame.h"

namespace logic {

// Kapazitaet RINGBUFFER_FRAMES (config.h, ~60 s @ 10 Hz). Ist der Puffer
// voll, ueberschreibt push() das aelteste Element (FR-TEL-04).
class TelemetryBuffer {
 public:
  void push(const uint8_t* frame_bytes);

  // FIFO: liest UND entfernt das aelteste Frame. false, wenn leer (out
  // bleibt unveraendert).
  bool pop(uint8_t* out);

  size_t size() const { return count_; }
  bool empty() const { return count_ == 0; }
  bool full() const { return count_ == RINGBUFFER_FRAMES; }

 private:
  uint8_t frames_[RINGBUFFER_FRAMES][TELEMETRY_FRAME_SIZE];
  size_t head_ = 0;   // naechster Schreib-Index
  size_t tail_ = 0;   // naechster Lese-Index (aeltestes Element)
  size_t count_ = 0;  // Anzahl belegter Slots
};

}  // namespace logic
