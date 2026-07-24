// lifecycle_fsm.cpp — Umsetzung R1-Zustandsmaschine. Rein, hardwarefrei.
#include "lifecycle_fsm.h"

namespace logic {

LifecycleFsm::LifecycleFsm(const LifecycleParams& params) : params_(params) {}

LifecycleOutput LifecycleFsm::update(bool critical_sensors_ready, uint32_t now_ms) {
  if (state_ == SystemState::Run) {
    return { state_, degraded_ };  // kein Rueckfall nach INIT, kein S_FAULT (FR-STA-06)
  }

  if (!init_start_set_) {
    init_start_set_ = true;
    init_start_ms_ = now_ms;
  }

  // Bereit-Flag zuerst pruefen: bei Gleichzeitigkeit (Flag wird exakt im
  // Timeout-Tick gesetzt) gewinnt ready -> RUN, degraded=false.
  if (critical_sensors_ready) {
    state_ = SystemState::Run;
    degraded_ = false;
    return { state_, degraded_ };
  }

  if (now_ms - init_start_ms_ >= params_.init_timeout_ms) {
    state_ = SystemState::Run;
    degraded_ = true;  // FR-STA-02: degradierter RUN
    return { state_, degraded_ };
  }

  return { state_, degraded_ };  // weiterhin S_INIT
}

}  // namespace logic
