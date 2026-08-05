// telemetry_frame.cpp — Umsetzung Telemetrie-Frame-Serialisierung. Rein,
// hardwarefrei.
#include "telemetry_frame.h"
#include <cstring>
#include "config.h"

namespace logic {

namespace {

// memcpy statt Pointer-Cast: einige Felder liegen nicht typ-aligned
// (s. Layout-Tabelle in telemetry_frame.h) -- ein *(float*)-Zugriff darauf
// waere undefiniertes Verhalten auf manchen Architekturen.
size_t writeU8(uint8_t* out, size_t offset, uint8_t v) {
  out[offset] = v;
  return offset + sizeof(v);
}
size_t writeU16(uint8_t* out, size_t offset, uint16_t v) {
  std::memcpy(out + offset, &v, sizeof(v));
  return offset + sizeof(v);
}
size_t writeU32(uint8_t* out, size_t offset, uint32_t v) {
  std::memcpy(out + offset, &v, sizeof(v));
  return offset + sizeof(v);
}
size_t writeF32(uint8_t* out, size_t offset, float v) {
  std::memcpy(out + offset, &v, sizeof(v));
  return offset + sizeof(v);
}

}  // namespace

void telemetryFrameSerialize(const TelemetryFrame& frame, uint8_t* out) {
  size_t o = 0;
  o = writeU16(out, o, TELEMETRY_SCHEMA_VERSION);
  o = writeU32(out, o, frame.timestamp_ms);

  o = writeF32(out, o, frame.accel_x_ms2);
  o = writeF32(out, o, frame.accel_y_ms2);
  o = writeF32(out, o, frame.accel_z_ms2);
  o = writeF32(out, o, frame.gyro_x_rads);
  o = writeF32(out, o, frame.gyro_y_rads);
  o = writeF32(out, o, frame.gyro_z_rads);
  o = writeF32(out, o, frame.brake_decel_ms2);

  o = writeF32(out, o, frame.pressure_pa);
  o = writeF32(out, o, frame.temperature_c);

  o = writeF32(out, o, static_cast<float>(frame.lat));  // Downcast, s. Header-Kommentar
  o = writeF32(out, o, static_cast<float>(frame.lon));
  o = writeF32(out, o, frame.speed_kmph);
  o = writeF32(out, o, frame.course_deg);
  o = writeF32(out, o, frame.altitude_m);
  o = writeU8(out, o, frame.sats);
  o = writeF32(out, o, frame.hdop);
  o = writeU16(out, o, frame.utc_year);
  o = writeU8(out, o, frame.utc_month);
  o = writeU8(out, o, frame.utc_day);
  o = writeU8(out, o, frame.utc_hour);
  o = writeU8(out, o, frame.utc_minute);
  o = writeU8(out, o, frame.utc_second);

  o = writeU8(out, o, frame.system_state);
  o = writeU8(out, o, frame.init_degraded ? 1 : 0);
  o = writeU8(out, o, frame.imu_health_state);
  o = writeU8(out, o, frame.baro_valid ? 1 : 0);
  o = writeU8(out, o, frame.gnss_fix_status);
  o = writeU8(out, o, frame.watchdog_recovered ? 1 : 0);

  o = writeU8(out, o, frame.brake_light_pct);

  (void)o;  // == TELEMETRY_FRAME_SIZE, s. test_telemetry_frame Sanity-Test
}

}  // namespace logic
