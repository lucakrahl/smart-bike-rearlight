// gnss_speed_ref.cpp — Umsetzung GNSS-Referenzbeschleunigung. Rein,
// hardwarefrei.
#include "gnss_speed_ref.h"
#include "config.h"

namespace logic {

namespace {
bool sampleIndividuallyValid(const GnssSpeedRefInput& in) {
  return in.location_valid && in.sats >= GNSS_SPEED_REF_MIN_SATS &&
         in.hdop <= GNSS_SPEED_REF_MAX_HDOP && in.speed_mps > GNSS_SPEED_REF_MIN_SPEED_MPS;
}
}  // namespace

GnssSpeedRefOutput GnssSpeedRef::update(const GnssSpeedRefInput& in) {
  GnssSpeedRefOutput out;  // Default: accel_ms2=0.0f, valid=false

  if (!sampleIndividuallyValid(in)) {
    // Ein einzelner Ausreisser bricht die Kette -- "aufeinanderfolgend"
    // heisst kein invalides Sample dazwischen (s. Header-Kommentar).
    has_prev_ = false;
    return out;
  }

  if (has_prev_ && in.dt_s > 0.0f) {
    // positiv = Verzoegerung: sinkende Geschwindigkeit -> positiver Wert.
    out.accel_ms2 = (prev_speed_mps_ - in.speed_mps) / in.dt_s;
    out.valid = true;
  }
  // has_prev_ war false (erster Fix ohne Vorgaenger) ODER dt_s<=0 (nicht
  // plausibel): out bleibt invalid, dieses Sample wird trotzdem als neuer
  // "letzter gueltiger Fix" gemerkt.

  prev_speed_mps_ = in.speed_mps;
  has_prev_ = true;
  return out;
}

void GnssSpeedRef::reset() {
  has_prev_ = false;
  prev_speed_mps_ = 0.0f;
}

}  // namespace logic
