// lifecycle_fsm.h — R1 Lebenszyklus: S_INIT -> S_RUN (FR-STA-01/02/06)
// REINE LOGIK, hardwarefrei (NFR-TST-01): kein #include <Arduino.h>.
// Nimmt das "kritische Sensoren bereit"-Flag als einfachen Eingabewert
// entgegen; kennt nicht die konkrete IMU-Anbindung (liefert das Flag ab M3).
// Host-testbar (siehe firmware/test/test_lifecycle_fsm/).
//
// TODO(FR-SAF-03, M2/main): Watchdog-Aktivierung ist Hardware — gehoert in
// main.cpp/Treiber, nicht in diese Logik.
#pragma once
#include <cstdint>
#include "system_state.h"
#include "config.h"

namespace logic {

struct LifecycleParams {
  uint32_t init_timeout_ms = INIT_TIMEOUT_MS;  // FR-STA-01
};

struct LifecycleOutput {
  SystemState state;  // logic::SystemState — derselbe Typ, den tail_light_fsm erwartet
  // true, wenn RUN ueber den Init-Timeout statt ueber das Bereit-Flag erreicht
  // wurde (FR-STA-02). Ist das Ergebnis des INIT-Uebergangs und bleibt danach
  // gehalten — sagt nichts ueber die laufende Sensor-Gesundheit im RUN-Betrieb
  // aus (das ist Aufgabe von M3/Telemetrie, FR-STA-04/05).
  bool degraded;
};

class LifecycleFsm {
 public:
  explicit LifecycleFsm(const LifecycleParams& params = LifecycleParams());

  // critical_sensors_ready: Eingabewert, den M3 (IMU-Treiber) spaeter liefert.
  // now_ms: monotone Zeitbasis.
  LifecycleOutput update(bool critical_sensors_ready, uint32_t now_ms);

 private:
  LifecycleParams params_;
  SystemState state_ = SystemState::Init;
  bool degraded_ = false;
  bool init_start_set_ = false;
  uint32_t init_start_ms_ = 0;
};

}  // namespace logic
