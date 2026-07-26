// blinker_fsm.h — R3 Blinker: Zustandsmaschine AUS/LINKS/RECHTS/WARN
// (FR-BLK-01..09). REINE LOGIK, hardwarefrei (NFR-TST-01): kein
// #include <Arduino.h>. Nimmt ein ButtonEvent (aus button_decoder) und die
// Zeit als einfache Eingabewerte entgegen (kein millis()); kennt weder den
// RF-Treiber noch die R1-Lifecycle-Implementierung (FR-BLK-09-Gating gegen
// SystemState erfolgt in main.cpp, nicht hier). Host-testbar (siehe
// firmware/test/test_blinker_fsm/).
#pragma once
#include <cstdint>
#include "button_decoder.h"  // ButtonEvent: gemeinsamer R3-Eingabetyp
#include "config.h"

namespace logic {

enum class BlinkerState { Aus, Links, Rechts, Warn };

struct BlinkerParams {
  uint32_t timeout_ms     = BLINKER_TIMEOUT_MS;  // FR-BLK-03
  float    blink_hz       = BLINK_FREQ_HZ;        // FR-BLK-08
  uint8_t  blink_duty_pct = BLINK_DUTY_PCT;       // FR-BLK-08
};

struct BlinkerOutput {
  bool left_on;
  bool right_on;
};

class BlinkerFsm {
 public:
  explicit BlinkerFsm(const BlinkerParams& params = BlinkerParams());

  // event: SHORT_LINKS/SHORT_RECHTS/LONG/NONE dieses Ticks. now_ms: monotone
  // Zeitbasis (fuer 60-s-Timeout und Blinktakt).
  BlinkerOutput update(ButtonEvent event, uint32_t now_ms);

 private:
  BlinkerParams params_;
  BlinkerState  state_ = BlinkerState::Aus;
  uint32_t      timeout_start_ms_ = 0;  // Anker fuer die 60-s-Marke (Links/Rechts)
};

}  // namespace logic
