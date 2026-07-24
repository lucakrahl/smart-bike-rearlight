// tail_light_fsm.cpp — Umsetzung R2-Zustandsmaschine. Rein, hardwarefrei.
#include "tail_light_fsm.h"

namespace logic {

namespace {

// Rechteck-Blinken: period_ms aus freq_hz, "an" fuer duty_pct % der Periode.
uint8_t blinkDuty(uint32_t now_ms, float freq_hz, uint8_t duty_pct,
                   uint8_t low_pct, uint8_t high_pct) {
  const uint32_t period_ms = static_cast<uint32_t>(1000.0f / freq_hz + 0.5f);
  const uint32_t on_ms = period_ms * duty_pct / 100u;
  const uint32_t phase = now_ms % period_ms;
  return (phase < on_ms) ? high_pct : low_pct;
}

}  // namespace

TailLightFsm::TailLightFsm(const TailLightParams& params) : params_(params) {}

TailLightOutput TailLightFsm::update(float decel_ms2, SystemState sys_state, uint32_t now_ms) {
  if (sys_state == SystemState::Init) {
    state_ = TailLightState::InitBlink;
    below_off_pending_ = false;
    const uint8_t duty = blinkDuty(now_ms, params_.init_blink_hz, params_.init_blink_duty_pct,
                                    0, params_.init_blink_high_pct);
    return { state_, duty };
  }

  // Einstieg RUN: erster Zustand ist immer das Schlusslicht (FR-TL-04).
  if (state_ == TailLightState::InitBlink) {
    state_ = TailLightState::Taillight;
    below_off_pending_ = false;
  }

  switch (state_) {
    case TailLightState::Taillight: {
      if (decel_ms2 > params_.brake.on_ms2) {
        state_ = TailLightState::Brakelight;
        below_off_pending_ = false;
        held_brake_duty_pct_ = static_cast<uint8_t>(brakeDutyPercent(decel_ms2, params_.brake));
      }
      break;
    }
    case TailLightState::Brakelight: {
      // ESS hat Vorrang vor Bremslicht (FR-TL-07), nur wenn freigeschaltet.
      if (params_.ess_enabled && decel_ms2 >= params_.ess_on_ms2) {
        state_ = TailLightState::EssBlink;
        below_off_pending_ = false;
        break;
      }
      if (decel_ms2 < params_.off_ms2) {
        if (!below_off_pending_) {
          // held_brake_duty_pct_ traegt bereits den zuletzt (vor dem
          // Unterschreiten) erreichten Bremslicht-Wert aus dem "else"-Zweig
          // unten und bleibt jetzt bis zum Ablauf der Haltezeit eingefroren.
          below_off_pending_ = true;
          below_off_since_ms_ = now_ms;
        } else if (now_ms - below_off_since_ms_ >= params_.min_hold_ms) {
          state_ = TailLightState::Taillight;   // Mindesthaltezeit erfuellt
          below_off_pending_ = false;
        }
      } else {
        below_off_pending_ = false;  // wieder ueber der Hysteresegrenze
        held_brake_duty_pct_ = static_cast<uint8_t>(brakeDutyPercent(decel_ms2, params_.brake));
      }
      break;
    }
    case TailLightState::EssBlink: {
      if (decel_ms2 < params_.ess_off_ms2) {
        state_ = TailLightState::Brakelight;
        below_off_pending_ = false;
        held_brake_duty_pct_ = static_cast<uint8_t>(brakeDutyPercent(decel_ms2, params_.brake));
      }
      break;
    }
    case TailLightState::InitBlink:
      break;  // durch den Uebergang oben in diesem Aufruf bereits verlassen
  }

  if (state_ == TailLightState::EssBlink) {
    // Modulation 100 % <-> Schlusslicht-Grundniveau, nie 0 % (FR-TL-07).
    const uint8_t duty = blinkDuty(now_ms, params_.ess_blink_hz, params_.ess_blink_duty_pct,
                                    static_cast<uint8_t>(params_.brake.tail_pct), 100);
    return { state_, duty };
  }

  if (state_ == TailLightState::Brakelight && below_off_pending_) {
    // FR-TL-06: waehrend der Mindesthaltezeit nicht sofort auf Schlusslicht
    // abfallen, sondern den zuletzt erreichten Bremslicht-Wert halten.
    return { state_, held_brake_duty_pct_ };
  }

  const uint8_t duty = static_cast<uint8_t>(brakeDutyPercent(decel_ms2, params_.brake));
  return { state_, duty };
}

}  // namespace logic
