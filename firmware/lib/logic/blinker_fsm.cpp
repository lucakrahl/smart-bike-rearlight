// blinker_fsm.cpp — Umsetzung R3-Zustandsmaschine. Rein, hardwarefrei.
#include "blinker_fsm.h"

namespace logic {

namespace {

// Rechteck-Blinken: period_ms aus freq_hz, "an" fuer duty_pct % der Periode.
bool blinkOn(uint32_t now_ms, float freq_hz, uint8_t duty_pct) {
  const uint32_t period_ms = static_cast<uint32_t>(1000.0f / freq_hz + 0.5f);
  const uint32_t on_ms = period_ms * duty_pct / 100u;
  const uint32_t phase = now_ms % period_ms;
  return phase < on_ms;
}

}  // namespace

BlinkerFsm::BlinkerFsm(const BlinkerParams& params) : params_(params) {}

BlinkerOutput BlinkerFsm::update(ButtonEvent event, uint32_t now_ms) {
  switch (state_) {
    case BlinkerState::Aus: {
      if (event == ButtonEvent::SHORT_LINKS) {
        state_ = BlinkerState::Links;
        timeout_start_ms_ = now_ms;
      } else if (event == ButtonEvent::SHORT_RECHTS) {
        state_ = BlinkerState::Rechts;
        timeout_start_ms_ = now_ms;
      } else if (event == ButtonEvent::LONG) {
        state_ = BlinkerState::Warn;  // FR-BLK-04
      }
      break;
    }
    case BlinkerState::Links: {
      if (event == ButtonEvent::SHORT_LINKS) {
        state_ = BlinkerState::Aus;  // FR-BLK-01: Toggle gleiche Seite
      } else if (event == ButtonEvent::SHORT_RECHTS) {
        state_ = BlinkerState::Rechts;  // FR-BLK-02: Seitenwechsel
        timeout_start_ms_ = now_ms;      // Timer neu setzen
      } else if (event == ButtonEvent::LONG) {
        state_ = BlinkerState::Warn;  // FR-BLK-04
      } else if (now_ms - timeout_start_ms_ >= params_.timeout_ms) {
        state_ = BlinkerState::Aus;  // FR-BLK-03: 60-s-Selbstabschaltung
      }
      break;
    }
    case BlinkerState::Rechts: {
      if (event == ButtonEvent::SHORT_RECHTS) {
        state_ = BlinkerState::Aus;  // FR-BLK-01: Toggle gleiche Seite
      } else if (event == ButtonEvent::SHORT_LINKS) {
        state_ = BlinkerState::Links;  // FR-BLK-02: Seitenwechsel
        timeout_start_ms_ = now_ms;     // Timer neu setzen
      } else if (event == ButtonEvent::LONG) {
        state_ = BlinkerState::Warn;  // FR-BLK-04
      } else if (now_ms - timeout_start_ms_ >= params_.timeout_ms) {
        state_ = BlinkerState::Aus;  // FR-BLK-03: 60-s-Selbstabschaltung
      }
      break;
    }
    case BlinkerState::Warn: {
      // FR-BLK-05: jeder kurze Druck beendet WARN -> AUS, verbraucht (kein
      // Richtungsblinker danach). Kein Timeout (FR-BLK-04).
      if (event == ButtonEvent::SHORT_LINKS || event == ButtonEvent::SHORT_RECHTS) {
        state_ = BlinkerState::Aus;
      }
      break;
    }
  }

  BlinkerOutput out{false, false};
  const bool on = blinkOn(now_ms, params_.blink_hz, params_.blink_duty_pct);
  switch (state_) {
    case BlinkerState::Links:
      out.left_on = on;
      break;
    case BlinkerState::Rechts:
      out.right_on = on;
      break;
    case BlinkerState::Warn:
      out.left_on = on;
      out.right_on = on;  // FR-BLK-06: einziger Zustand mit beidseitigem Blinken
      break;
    case BlinkerState::Aus:
      break;
  }
  return out;
}

}  // namespace logic
