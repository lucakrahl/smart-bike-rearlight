// telemetry_frame.h — Telemetrie-Frame-Layout + Serialisierung
// (FR-TEL-02/03/06, FR-STA-05). REINE LOGIK, hardwarefrei (NFR-TST-01):
// kein #include <Arduino.h>. Nimmt Sensor-/Statuswerte als einfache Structs
// entgegen; kennt weder Treiber noch main.cpp. Host-testbar (siehe
// firmware/test/test_telemetry_frame/).
//
// Byte-Layout (gepackt, Little-Endian, 80 Byte gesamt). Serialisierung
// erfolgt per memcpy an fortlaufenden Offsets, NICHT per struct-Cast --
// vermeidet Compiler-Padding- und Alignment-Annahmen (einige Felder unten
// liegen bewusst nicht typ-aligned, z. B. "hdop" bei Offset 63).
//
//   Offset  Groesse  Typ     Feld
//   0       2        uint16  version (TELEMETRY_SCHEMA_VERSION, FR-TEL-06)
//   2       4        uint32  timestamp_ms
//   6       4        float   accel_x_ms2
//   10      4        float   accel_y_ms2
//   14      4        float   accel_z_ms2
//   18      4        float   gyro_x_rads
//   22      4        float   gyro_y_rads
//   26      4        float   gyro_z_rads
//   30      4        float   brake_decel_ms2
//   34      4        float   pressure_pa
//   38      4        float   temperature_c
//   42      4        float   lat            (von double downcast)
//   46      4        float   lon            (von double downcast)
//   50      4        float   speed_kmph
//   54      4        float   course_deg
//   58      4        float   altitude_m
//   62      1        uint8   sats
//   63      4        float   hdop
//   67      2        uint16  utc_year
//   69      1        uint8   utc_month
//   70      1        uint8   utc_day
//   71      1        uint8   utc_hour
//   72      1        uint8   utc_minute
//   73      1        uint8   utc_second
//   74      1        uint8   system_state       (SystemState: 0=Init,1=Run)
//   75      1        uint8   init_degraded      (0/1)
//   76      1        uint8   imu_health_state   (0=OK,1=RECOVERING,2=FAILED)
//   77      1        uint8   baro_valid         (0/1, FR-STA-05)
//   78      1        uint8   gnss_fix_status    (0=NO_DATA,1=NO_FIX,2=FIX_OK)
//   79      1        uint8   watchdog_recovered (0/1)
#pragma once
#include <cstdint>
#include <cstddef>

namespace logic {

constexpr size_t TELEMETRY_FRAME_SIZE = 80;

struct TelemetryFrame {
  uint32_t timestamp_ms = 0;

  // IMU (FR-TEL-03: 6 Achsen + Verzoegerung). brake_decel_ms2 ist der
  // motion_filter-Rohwert (nicht der fail-safe-gegatete Bremslicht-Wert) --
  // die App soll auch waehrend eines Sensorfehlers sehen koennen, was
  // tatsaechlich gemessen wurde; imu_health_state zeigt die Verlaesslichkeit.
  float accel_x_ms2 = 0.0f, accel_y_ms2 = 0.0f, accel_z_ms2 = 0.0f;
  float gyro_x_rads = 0.0f, gyro_y_rads = 0.0f, gyro_z_rads = 0.0f;
  float brake_decel_ms2 = 0.0f;

  // BMP
  float pressure_pa = 0.0f;
  float temperature_c = 0.0f;

  // GNSS. lat/lon als double (wie GnssData) -- der Downcast auf float
  // passiert erst in telemetryFrameSerialize(), an einer Stelle dokumentiert.
  double lat = 0.0, lon = 0.0;
  float speed_kmph = 0.0f, course_deg = 0.0f, altitude_m = 0.0f;
  uint8_t sats = 0;
  float hdop = 0.0f;
  uint16_t utc_year = 0;
  uint8_t utc_month = 0, utc_day = 0, utc_hour = 0, utc_minute = 0, utc_second = 0;

  // Status
  uint8_t system_state = 0;      // logic::SystemState als uint8_t
  bool init_degraded = false;
  uint8_t imu_health_state = 0;  // logic::ImuHealthState als uint8_t
  bool baro_valid = false;
  uint8_t gnss_fix_status = 0;   // logic::GnssFixStatus als uint8_t
  bool watchdog_recovered = false;
};

// Serialisiert "frame" nach "out" gemaess obigem Layout. "out" muss
// mindestens TELEMETRY_FRAME_SIZE Byte fassen.
void telemetryFrameSerialize(const TelemetryFrame& frame, uint8_t* out);

}  // namespace logic
