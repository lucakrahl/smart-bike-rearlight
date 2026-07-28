// Host-Unit-Test des Telemetrie-Ringpuffers (NFR-TST-03). Laeuft ohne
// ESP32: pio test -e native
#include <unity.h>
#include <cstring>
#include "telemetry_buffer.h"
#include "config.h"

using namespace logic;

namespace {
// Markiert einen Frame-Slot eindeutig ueber die ersten zwei Bytes (als
// uint16-Index) -- reicht ueber RINGBUFFER_FRAMES (600) hinaus, anders als
// ein einzelnes Marker-Byte (max. 256 unterscheidbare Werte).
void makeMarkedFrame(uint8_t* out, uint16_t index) {
  std::memset(out, 0, TELEMETRY_FRAME_SIZE);
  std::memcpy(out, &index, sizeof(index));
}
uint16_t markerOf(const uint8_t* frame) {
  uint16_t index;
  std::memcpy(&index, frame, sizeof(index));
  return index;
}
}  // namespace

void test_push_below_capacity_reports_correct_size() {
  TelemetryBuffer buf;
  uint8_t frame[TELEMETRY_FRAME_SIZE];
  makeMarkedFrame(frame, 0);
  for (int i = 0; i < 5; ++i) {
    buf.push(frame);
  }
  TEST_ASSERT_EQUAL_UINT32(5, (uint32_t)buf.size());
  TEST_ASSERT_FALSE(buf.full());
  TEST_ASSERT_FALSE(buf.empty());
}

void test_push_to_capacity_is_full() {
  TelemetryBuffer buf;
  uint8_t frame[TELEMETRY_FRAME_SIZE];
  makeMarkedFrame(frame, 0);
  for (size_t i = 0; i < RINGBUFFER_FRAMES; ++i) {
    buf.push(frame);
  }
  TEST_ASSERT_EQUAL_UINT32((uint32_t)RINGBUFFER_FRAMES, (uint32_t)buf.size());
  TEST_ASSERT_TRUE(buf.full());
}

void test_overflow_overwrites_oldest_and_keeps_fifo_order() {
  TelemetryBuffer buf;
  uint8_t frame[TELEMETRY_FRAME_SIZE];
  // RINGBUFFER_FRAMES + 3 Frames pushen: die ersten 3 (Index 0,1,2) muessen
  // ueberschrieben sein (FR-TEL-04); size() bleibt bei RINGBUFFER_FRAMES.
  for (size_t i = 0; i < RINGBUFFER_FRAMES + 3; ++i) {
    makeMarkedFrame(frame, (uint16_t)i);
    buf.push(frame);
  }
  TEST_ASSERT_EQUAL_UINT32((uint32_t)RINGBUFFER_FRAMES, (uint32_t)buf.size());
  TEST_ASSERT_TRUE(buf.full());

  uint8_t out[TELEMETRY_FRAME_SIZE];
  TEST_ASSERT_TRUE(buf.pop(out));
  TEST_ASSERT_EQUAL_UINT16(3, markerOf(out));  // aeltestes verbliebenes Frame
}

void test_pop_returns_false_when_empty() {
  TelemetryBuffer buf;
  uint8_t out[TELEMETRY_FRAME_SIZE];
  TEST_ASSERT_FALSE(buf.pop(out));
}

void test_pop_preserves_fifo_order_and_exact_bytes() {
  TelemetryBuffer buf;
  uint8_t frame[TELEMETRY_FRAME_SIZE];
  for (uint16_t i = 0; i < 3; ++i) {
    makeMarkedFrame(frame, i);
    buf.push(frame);
  }
  uint8_t out[TELEMETRY_FRAME_SIZE];
  TEST_ASSERT_TRUE(buf.pop(out));
  TEST_ASSERT_EQUAL_UINT16(0, markerOf(out));
  TEST_ASSERT_TRUE(buf.pop(out));
  TEST_ASSERT_EQUAL_UINT16(1, markerOf(out));
  TEST_ASSERT_TRUE(buf.pop(out));
  TEST_ASSERT_EQUAL_UINT16(2, markerOf(out));
  TEST_ASSERT_FALSE(buf.pop(out));  // wieder leer
}

void test_pop_reduces_size() {
  TelemetryBuffer buf;
  uint8_t frame[TELEMETRY_FRAME_SIZE];
  makeMarkedFrame(frame, 0);
  buf.push(frame);
  buf.push(frame);
  uint8_t out[TELEMETRY_FRAME_SIZE];
  buf.pop(out);
  TEST_ASSERT_EQUAL_UINT32(1, (uint32_t)buf.size());
  TEST_ASSERT_FALSE(buf.empty());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_push_below_capacity_reports_correct_size);
  RUN_TEST(test_push_to_capacity_is_full);
  RUN_TEST(test_overflow_overwrites_oldest_and_keeps_fifo_order);
  RUN_TEST(test_pop_returns_false_when_empty);
  RUN_TEST(test_pop_preserves_fifo_order_and_exact_bytes);
  RUN_TEST(test_pop_reduces_size);
  return UNITY_END();
}
