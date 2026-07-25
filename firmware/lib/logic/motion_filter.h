// motion_filter.h — Komplementaerfilter + Gravitationskompensation (R4->R2)
// REINE LOGIK, hardwarefrei (NFR-TST-01): kein #include <Arduino.h>.
// Nimmt IMU-Rohwerte + Zeitschritt als einfache Eingabewerte entgegen (kein
// millis()); kennt nicht den konkreten IMU-Treiber. Host-testbar (siehe
// firmware/test/test_motion_filter/).
//
// TODO(offen): Achsen-/Vorzeichenkonvention unverifiziert, s. Kommentar bei
// MOTION_BRAKE_SIGN in config.h (Y=Fahrtrichtung ist durch die Bible
// gesichert; Z=oben/X=seitlich sowie das Bremsvorzeichen sind Annahmen).
#pragma once
#include "config.h"

namespace logic {

struct MotionParams {
  float alpha       = COMPL_FILTER_ALPHA;  // Komplementärfilter-Gewicht (Bible Kap. 6.4)
  float gravity_ms2 = 9.80665f;            // Standardschwerebeschleunigung
  float brake_sign  = MOTION_BRAKE_SIGN;   // TODO(offen): Achsenkalibrierung, s. config.h
};

struct MotionInput {
  float accel_x_ms2, accel_y_ms2, accel_z_ms2;  // Rohbeschleunigung inkl. g
  float gyro_x_rads;                             // Drehrate um die Nickachse (X)
  float dt_s;                                    // Zeitschritt seit letztem Sample
};

class MotionFilter {
 public:
  explicit MotionFilter(const MotionParams& params = MotionParams());

  // Verarbeitet ein IMU-Sample. Rueckgabe: nur die Beschleunigung in
  // Brems-Richtung (Y-Achse, gravitationskompensiert), als positiver Wert
  // (Eingang fuer FR-TL-06). Beschleunigen in Nicht-Brems-Richtung -> ~0,
  // damit Sprints/Antritte nicht das Bremslicht ausloesen.
  float update(const MotionInput& in);

 private:
  MotionParams params_;
  float pitch_rad_ = 0.0f;
};

}  // namespace logic
