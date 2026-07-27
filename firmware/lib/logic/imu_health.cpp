// imu_health.cpp — Umsetzung IMU-Gesundheitsueberwachung. Rein, hardwarefrei.
#include "imu_health.h"
#include <cmath>

namespace logic {

namespace {

bool valueRangeOk(float ax, float ay, float az) {
  const float mag = std::sqrt(ax * ax + ay * ay + az * az);
  return mag >= IMU_ACCEL_MIN_MAGNITUDE_MS2 && mag <= IMU_ACCEL_MAX_MAGNITUDE_MS2;
}

}  // namespace

ImuHealthOutput ImuHealth::update(bool read_ok, float ax, float ay, float az, float gx,
                                   uint32_t now_ms) {
  bool plausible = false;

  if (read_ok) {
    const bool range_ok = valueRangeOk(ax, ay, az);

    bool frozen = false;
    bool slew_ok = true;
    if (has_prev_) {
      if (ax == prev_ax_ && ay == prev_ay_ && az == prev_az_ && gx == prev_gx_) {
        frozen_count_++;
        frozen = frozen_count_ >= IMU_FROZEN_LIMIT;
      } else {
        frozen_count_ = 0;
      }

      // Sprung-Plausibilitaet: ein physikalisch unmoeglich schneller Sprung
      // zum letzten GELESENEN (nicht nur plausiblen) Sample ist implausibel.
      const float dax = ax - prev_ax_;
      const float day = ay - prev_ay_;
      const float daz = az - prev_az_;
      const float accel_delta = std::sqrt(dax * dax + day * day + daz * daz);
      const float gyro_delta = std::fabs(gx - prev_gx_);
      slew_ok = accel_delta <= IMU_ACCEL_MAX_SLEW_MS2 && gyro_delta <= IMU_GYRO_MAX_SLEW_RADS;
    } else {
      frozen_count_ = 0;
    }

    // Vergleichsbasis IMMER aktualisieren, unabhaengig vom Urteil dieses
    // Zyklus: ein abgelehntes Sprung-Sample ist trotzdem die neue Realitaet
    // fuer den naechsten Vergleich -- ein echter harter Bremsstoss wird so
    // hoechstens einen Zyklus (~10 ms) unterdrueckt, nicht dauerhaft.
    prev_ax_ = ax;
    prev_ay_ = ay;
    prev_az_ = az;
    prev_gx_ = gx;
    has_prev_ = true;

    plausible = range_ok && !frozen && slew_ok;
  }

  // Eskalations-Vertrauen: eigener Zaehler, komplementaer zu fail_streak_
  // (der fuer Recovery zustaendig ist). Ein einzelnes plausibles Sample
  // amid einer Ausfallphase soll noch KEINE Bremseskalation freigeben.
  consecutive_plausible_ = plausible ? consecutive_plausible_ + 1 : 0;

  ImuHealthOutput out{};
  out.plausible = plausible;
  out.escalation_trusted = consecutive_plausible_ >= IMU_ESCALATION_CONFIRM_CYCLES;

  if (plausible) {
    fail_streak_ = 0;
    recovery_attempt_ = 0;
    state_ = ImuHealthState::OK;
    out.request_recovery = false;
    out.recovery_stage = 0;
  } else {
    fail_streak_++;
    switch (state_) {
      case ImuHealthState::OK:
        if (fail_streak_ >= IMU_FAIL_LIMIT) {
          state_ = ImuHealthState::RECOVERING;
          recovery_attempt_ = 1;
          out.request_recovery = true;
          out.recovery_stage = 1;  // erster Versuch: Soft-Reinit
        } else {
          out.request_recovery = false;
          out.recovery_stage = 0;
        }
        break;
      case ImuHealthState::RECOVERING:
        recovery_attempt_++;
        if (recovery_attempt_ > IMU_RECOVERY_MAX_ATTEMPTS) {
          state_ = ImuHealthState::FAILED;
          last_recovery_attempt_ms_ = now_ms;  // Hintergrund-Reinit-Takt startet jetzt
          out.request_recovery = false;
          out.recovery_stage = 0;
        } else {
          out.request_recovery = true;
          out.recovery_stage = (recovery_attempt_ == 1) ? 1 : 2;  // ab 2. Versuch eskaliert
        }
        break;
      case ImuHealthState::FAILED:
        if (now_ms - last_recovery_attempt_ms_ >= IMU_RECOVERY_MIN_INTERVAL_MS) {
          out.request_recovery = true;
          out.recovery_stage = 2;  // Hintergrund-Versuche nutzen die staerkere Stufe
          last_recovery_attempt_ms_ = now_ms;
        } else {
          out.request_recovery = false;
          out.recovery_stage = 0;
        }
        break;
    }
  }

  out.state = state_;
  out.degraded = (state_ != ImuHealthState::OK);
  return out;
}

}  // namespace logic
