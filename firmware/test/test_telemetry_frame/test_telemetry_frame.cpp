// Host-Unit-Test der Telemetrie-Frame-Serialisierung (NFR-TST-03). Laeuft
// ohne ESP32: pio test -e native
#include <unity.h>
#include <cstring>
#include "telemetry_frame.h"
#include "config.h"

using namespace logic;

namespace {
uint16_t readU16(const uint8_t* buf, size_t offset) {
  uint16_t v;
  std::memcpy(&v, buf + offset, sizeof(v));
  return v;
}
uint32_t readU32(const uint8_t* buf, size_t offset) {
  uint32_t v;
  std::memcpy(&v, buf + offset, sizeof(v));
  return v;
}
float readF32(const uint8_t* buf, size_t offset) {
  float v;
  std::memcpy(&v, buf + offset, sizeof(v));
  return v;
}
}  // namespace

void test_frame_size_matches_documented_layout() {
  TEST_ASSERT_EQUAL_UINT32(81, (uint32_t)TELEMETRY_FRAME_SIZE);
}

void test_version_field_at_offset_0() {
  TelemetryFrame frame;
  uint8_t buf[TELEMETRY_FRAME_SIZE];
  telemetryFrameSerialize(frame, buf);
  TEST_ASSERT_EQUAL_UINT16(TELEMETRY_SCHEMA_VERSION, readU16(buf, 0));
}

void test_timestamp_round_trip() {
  TelemetryFrame frame;
  frame.timestamp_ms = 123456789u;
  uint8_t buf[TELEMETRY_FRAME_SIZE];
  telemetryFrameSerialize(frame, buf);
  TEST_ASSERT_EQUAL_UINT32(123456789u, readU32(buf, 2));
}

void test_imu_fields_round_trip_and_offsets() {
  TelemetryFrame frame;
  frame.accel_x_ms2 = 1.5f;
  frame.accel_y_ms2 = -2.5f;
  frame.accel_z_ms2 = 9.81f;
  frame.gyro_x_rads = 0.1f;
  frame.gyro_y_rads = -0.2f;
  frame.gyro_z_rads = 0.3f;
  frame.brake_decel_ms2 = 4.2f;
  uint8_t buf[TELEMETRY_FRAME_SIZE];
  telemetryFrameSerialize(frame, buf);

  TEST_ASSERT_EQUAL_FLOAT(1.5f, readF32(buf, 6));
  TEST_ASSERT_EQUAL_FLOAT(-2.5f, readF32(buf, 10));
  TEST_ASSERT_EQUAL_FLOAT(9.81f, readF32(buf, 14));
  TEST_ASSERT_EQUAL_FLOAT(0.1f, readF32(buf, 18));
  TEST_ASSERT_EQUAL_FLOAT(-0.2f, readF32(buf, 22));
  TEST_ASSERT_EQUAL_FLOAT(0.3f, readF32(buf, 26));
  TEST_ASSERT_EQUAL_FLOAT(4.2f, readF32(buf, 30));
}

void test_bmp_fields_round_trip() {
  TelemetryFrame frame;
  frame.pressure_pa = 100123.5f;
  frame.temperature_c = 21.75f;
  uint8_t buf[TELEMETRY_FRAME_SIZE];
  telemetryFrameSerialize(frame, buf);

  TEST_ASSERT_EQUAL_FLOAT(100123.5f, readF32(buf, 34));
  TEST_ASSERT_EQUAL_FLOAT(21.75f, readF32(buf, 38));
}

void test_gnss_float_fields_round_trip() {
  TelemetryFrame frame;
  frame.lat = 51.2277411;   // double, wie GnssData
  frame.lon = 6.7734556;
  frame.speed_kmph = 18.5f;
  frame.course_deg = 270.0f;
  frame.altitude_m = 45.3f;
  frame.sats = 9;
  frame.hdop = 1.2f;
  uint8_t buf[TELEMETRY_FRAME_SIZE];
  telemetryFrameSerialize(frame, buf);

  // lat/lon: bewusster double->float-Downcast (s. telemetry_frame.h) --
  // Approximation statt exakter Gleichheit erwartet.
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, (float)frame.lat, readF32(buf, 42));
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, (float)frame.lon, readF32(buf, 46));
  TEST_ASSERT_EQUAL_FLOAT(18.5f, readF32(buf, 50));
  TEST_ASSERT_EQUAL_FLOAT(270.0f, readF32(buf, 54));
  TEST_ASSERT_EQUAL_FLOAT(45.3f, readF32(buf, 58));
  TEST_ASSERT_EQUAL_UINT8(9, buf[62]);
  TEST_ASSERT_EQUAL_FLOAT(1.2f, readF32(buf, 63));
}

void test_gnss_utc_fields_round_trip() {
  TelemetryFrame frame;
  frame.utc_year = 2026;
  frame.utc_month = 7;
  frame.utc_day = 28;
  frame.utc_hour = 14;
  frame.utc_minute = 30;
  frame.utc_second = 45;
  uint8_t buf[TELEMETRY_FRAME_SIZE];
  telemetryFrameSerialize(frame, buf);

  TEST_ASSERT_EQUAL_UINT16(2026, readU16(buf, 67));
  TEST_ASSERT_EQUAL_UINT8(7, buf[69]);
  TEST_ASSERT_EQUAL_UINT8(28, buf[70]);
  TEST_ASSERT_EQUAL_UINT8(14, buf[71]);
  TEST_ASSERT_EQUAL_UINT8(30, buf[72]);
  TEST_ASSERT_EQUAL_UINT8(45, buf[73]);
}

void test_status_fields_round_trip_including_edge_values() {
  TelemetryFrame frame;
  frame.system_state = 1;          // SystemState::Run
  frame.init_degraded = true;
  frame.imu_health_state = 2;      // ImuHealthState::FAILED
  frame.baro_valid = false;
  frame.gnss_fix_status = 2;       // GnssFixStatus::FIX_OK
  frame.watchdog_recovered = true;
  uint8_t buf[TELEMETRY_FRAME_SIZE];
  telemetryFrameSerialize(frame, buf);

  TEST_ASSERT_EQUAL_UINT8(1, buf[74]);
  TEST_ASSERT_EQUAL_UINT8(1, buf[75]);
  TEST_ASSERT_EQUAL_UINT8(2, buf[76]);
  TEST_ASSERT_EQUAL_UINT8(0, buf[77]);
  TEST_ASSERT_EQUAL_UINT8(2, buf[78]);
  TEST_ASSERT_EQUAL_UINT8(1, buf[79]);
}

void test_brake_light_pct_field_round_trip_and_offset() {
  TelemetryFrame frame;
  frame.brake_light_pct = 73;
  uint8_t buf[TELEMETRY_FRAME_SIZE];
  telemetryFrameSerialize(frame, buf);
  TEST_ASSERT_EQUAL_UINT8(73, buf[80]);
}

void test_brake_light_pct_edge_values() {
  TelemetryFrame frame;
  uint8_t buf[TELEMETRY_FRAME_SIZE];

  frame.brake_light_pct = 0;
  telemetryFrameSerialize(frame, buf);
  TEST_ASSERT_EQUAL_UINT8(0, buf[80]);

  frame.brake_light_pct = 100;
  telemetryFrameSerialize(frame, buf);
  TEST_ASSERT_EQUAL_UINT8(100, buf[80]);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_frame_size_matches_documented_layout);
  RUN_TEST(test_version_field_at_offset_0);
  RUN_TEST(test_timestamp_round_trip);
  RUN_TEST(test_imu_fields_round_trip_and_offsets);
  RUN_TEST(test_bmp_fields_round_trip);
  RUN_TEST(test_gnss_float_fields_round_trip);
  RUN_TEST(test_gnss_utc_fields_round_trip);
  RUN_TEST(test_status_fields_round_trip_including_edge_values);
  RUN_TEST(test_brake_light_pct_field_round_trip_and_offset);
  RUN_TEST(test_brake_light_pct_edge_values);
  return UNITY_END();
}
