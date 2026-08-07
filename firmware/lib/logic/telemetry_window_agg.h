// telemetry_window_agg.h — Fensteraggregation der 100-Hz-Groessen fuer das
// v3-Telemetrie-Frame (s. docs/BLE_Frame_v3_Schnittstelle.md Kap. 3.2/3.3).
// REINE LOGIK, hardwarefrei (NFR-TST-01): kein #include <Arduino.h>, kein
// Serial. Host-testbar (siehe firmware/test/test_telemetry_window_agg/).
//
// Zweck: Die Regime-Entscheidung und das Jerk-Kriterium leben auf dem
// 100-Hz-IMU-Takt, das Telemetrie-Frame nur auf 10 Hz -- ohne Aggregation
// wuerden Stoesse (SHOCK-Episoden von oft nur 1-3 Samples) im 10-Hz-Raster
// verschwinden. add() akkumuliert pro IMU-Sample, snapshotAndReset()
// liefert den Zustand des 100-ms-Fensters zum Sendezeitpunkt und setzt
// zurueck.
//
// NAMENSKOLLISION BEACHTEN: "norm_delta" ist hier die Abweichung des
// Beschleunigungsbetrags von g (‖a‖-g, s. Vertrag Kap. 3.2), NICHT der
// gleichnamige motion_filter::normDelta()-Getter (der liefert den Jerk!).
// Der Aufrufer (main.cpp) muss ‖a‖-g separat aus motion_filter::accel_norm()
// berechnen und getrennt vom Jerk (motion_filter::normDelta()) uebergeben.
#pragma once
#include <cstdint>
#include "motion_filter.h"  // fuer MotionRegime

namespace logic {

struct WindowAggSnapshot {
  float norm_delta_min = 0.0f;  // m/s^2, Minimum von (‖a‖-g) im Fenster
  float norm_delta_max = 0.0f;  // m/s^2, Maximum von (‖a‖-g) im Fenster
  float jerk_max = 0.0f;        // m/s^2 je 10 ms, Maximum von |Jerk| im Fenster
  uint8_t regime_static_n = 0;
  uint8_t regime_dynamic_n = 0;
  uint8_t regime_shock_n = 0;
  uint8_t dt_max_ms = 0;      // ms, saettigt bei 255
  uint16_t loop_max_us = 0;   // us, saettigt bei 65535
};

class TelemetryWindowAgg {
 public:
  // Pro 100-Hz-IMU-Sample genau einmal aufrufen.
  //   dt_s:        real gemessener Abtastabstand dieses Samples.
  //   norm_delta:  ‖a‖-g dieses Samples (NICHT motion_filter::normDelta()!).
  //   jerk:        motion_filter::normDelta() dieses Samples (Jerk, bereits
  //                10-ms-normiert) -- das Betragsmaximum wird gebildet.
  //   regime:      motion_filter::regime() dieses Samples.
  //   loop_us:     gemessene Dauer des zugehoerigen loop()-Durchlaufs.
  void add(float dt_s, float norm_delta, float jerk, MotionRegime regime, uint32_t loop_us);

  // Liefert den Zustand des Fensters seit dem letzten Reset (bzw. seit
  // Konstruktion) und setzt den Akkumulator zurueck. Kein add() seit dem
  // letzten Reset -> alle Felder 0 (Vertrag Kap. 3.3, Sensorausfall-Fall).
  // Die Zaehlersumme wird NICHT auf 10 normiert -- sie ist selbst eine
  // Messgroesse (Taktschwankung im Fenster).
  WindowAggSnapshot snapshotAndReset();

 private:
  bool has_samples_ = false;
  float norm_delta_min_ = 0.0f;
  float norm_delta_max_ = 0.0f;
  float jerk_max_ = 0.0f;
  uint32_t regime_static_n_ = 0;
  uint32_t regime_dynamic_n_ = 0;
  uint32_t regime_shock_n_ = 0;
  uint32_t dt_max_ms_ = 0;
  uint32_t loop_max_us_ = 0;
};

}  // namespace logic
