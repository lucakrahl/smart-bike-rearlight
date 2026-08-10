// tail_light_fsm.h — R2 Ruecklicht: Zustandsmaschine (FR-TL-01..07, FR-SAF-01/02)
// REINE LOGIK, hardwarefrei (NFR-TST-01): kein #include <Arduino.h>.
// Nimmt Verzoegerung und Systemzustand als einfache Eingabewerte entgegen;
// kennt weder IMU noch die R1-Lifecycle-Implementierung (Kopplung nur ueber
// SystemState, s. system_state.h). IMU-Anbindung folgt in M3, R1-FSM in M2.
// Host-testbar (siehe firmware/test/test_tail_light_fsm/).
#pragma once
#include <cstdint>
#include "brake_curve.h"
#include "system_state.h"
#include "config.h"

namespace logic {

enum class TailLightState {
  InitBlink,   // FR-TL-03
  Taillight,   // FR-TL-04
  Brakelight,  // FR-TL-05/06
  EssBlink,    // FR-TL-07 (experimentell, default AUS)
};

// Alle Schwellwerte/Dutys als benannte Konstanten aus config.h (FR-CFG-01);
// Uebersetzungszeit-Konstanten, keine Laufzeit-Konfiguration (FR-CFG-02/03
// nicht Teil des Arbeitsumfangs, Umfangsschnitt 10.08.2026, s. Project Bible
// Kap. 12). Keine Magic Numbers im .cpp.
struct TailLightParams {
  BrakeCurveParams brake{ BRAKE_ON_MS2, BRAKE_FULL_MS2, TAILLIGHT_DUTY_PCT };
  float    off_ms2             = BRAKE_OFF_MS2;        // FR-TL-06 Ausschalthysterese
  uint32_t min_hold_ms         = BRAKE_MIN_HOLD_MS;     // FR-TL-06 Mindesthaltezeit
  float    ess_on_ms2          = ESS_ON_MS2;            // FR-TL-07 ein
  float    ess_off_ms2         = ESS_OFF_MS2;           // FR-TL-07 aus (Hysterese)
  bool     ess_enabled         = ESS_ENABLED_DEFAULT;   // FR-TL-07 default AUS (§ 67 Abs. 4)
  float    init_blink_hz       = INIT_BLINK_FREQ_HZ;    // FR-TL-03
  uint8_t  init_blink_duty_pct = INIT_BLINK_DUTY_PCT;   // FR-TL-03 Zeit-Duty-Cycle
  uint8_t  init_blink_high_pct = INIT_BLINK_HIGH_PCT;   // FR-TL-03, C3.1 (unten fix 0 %)
  float    ess_blink_hz        = ESS_BLINK_FREQ_HZ;     // FR-TL-07
  uint8_t  ess_blink_duty_pct  = ESS_BLINK_DUTY_PCT;    // FR-TL-07 Zeit-Duty-Cycle
};

struct TailLightOutput {
  TailLightState state;
  uint8_t duty_pct;  // 0..100, rote LED (PWM-Duty)
};

class TailLightFsm {
 public:
  explicit TailLightFsm(const TailLightParams& params = TailLightParams());

  // decel_ms2: Betrag der gravitationskompensierten Verzoegerung [m/s^2].
  // sys_state: von R1 gelieferter Lebenszyklus-Zustand (Init/Run).
  // now_ms:    monotone Zeitbasis (Mindesthaltezeit/Blinktakt).
  TailLightOutput update(float decel_ms2, SystemState sys_state, uint32_t now_ms);

 private:
  TailLightParams params_;
  TailLightState  state_ = TailLightState::InitBlink;
  uint32_t below_off_since_ms_ = 0;
  bool     below_off_pending_  = false;
  uint8_t  held_brake_duty_pct_ = 0;  // FR-TL-06: eingefroren waehrend Haltezeit
};

}  // namespace logic
