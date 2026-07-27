// imu_health.h — IMU-Gesundheitsueberwachung: Plausibilitaet (FR-SNS-05) +
// gestufte I2C-/Sensor-Recovery (FR-SNS-04) + Fail-Safe-Signal (FR-STA-04).
// REINE LOGIK, hardwarefrei (NFR-TST-01): kein #include <Arduino.h>. Nimmt
// pro Zyklus das Lese-Ergebnis + Rohwerte entgegen; kennt weder den
// IMU-Treiber noch main.cpp. Host-testbar (siehe firmware/test/test_imu_health/).
#pragma once
#include <cstdint>
#include "config.h"

namespace logic {

enum class ImuHealthState { OK, RECOVERING, FAILED };

struct ImuHealthOutput {
  bool plausible;         // Verdikt DIESES Zyklus (read_ok + Wertebereich + Sprung + nicht eingefroren)
  ImuHealthState state;
  bool request_recovery;  // true genau in dem Zyklus, in dem main.cpp einen Schritt ausfuehren soll
  int  recovery_stage;    // 1 = Soft-Reinit, 2 = SCL-Clock-Release; nur gueltig wenn request_recovery
  bool degraded;          // == (state != OK); main.cpp gated damit R2 auf sicheren Wert (FR-STA-04)
  // true erst nach IMU_ESCALATION_CONFIRM_CYCLES aufeinanderfolgenden
  // plausiblen Zyklen: ein einzelnes Muell-aber-plausibles Sample darf keine
  // Bremseskalation ausloesen (Fehlerinjektionstest SDA-Kurzschluss).
  bool escalation_trusted;
};

class ImuHealth {
 public:
  // read_ok: Ergebnis des Treiber-Reads. ax..gx: Rohwerte (nur ausgewertet,
  // wenn read_ok). now_ms: monotone Zeitbasis (Hintergrund-Reinit-Takt im
  // FAILED-Zustand).
  ImuHealthOutput update(bool read_ok, float ax, float ay, float az, float gx, uint32_t now_ms);

 private:
  ImuHealthState state_ = ImuHealthState::OK;
  uint32_t fail_streak_ = 0;
  uint32_t frozen_count_ = 0;
  bool     has_prev_ = false;
  float    prev_ax_ = 0.0f, prev_ay_ = 0.0f, prev_az_ = 0.0f, prev_gx_ = 0.0f;
  uint8_t  recovery_attempt_ = 0;
  uint32_t last_recovery_attempt_ms_ = 0;
  uint32_t consecutive_plausible_ = 0;  // Komplement zu fail_streak_, fuer escalation_trusted
};

}  // namespace logic
