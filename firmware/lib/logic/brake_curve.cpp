// brake_curve.cpp — Umsetzung FR-TL-06 (Proportionalteil). Rein, hardwarefrei.
#include "brake_curve.h"

namespace logic {

int brakeDutyPercent(float decel_ms2, const BrakeCurveParams& p) {
  if (decel_ms2 <= p.on_ms2) {
    return p.tail_pct;                       // reines Schlusslicht
  }
  if (decel_ms2 >= p.full_ms2) {
    return 100;                              // Saettigung
  }
  // Lineare Interpolation tail_pct -> 100 % zwischen on_ms2 und full_ms2.
  const float span = p.full_ms2 - p.on_ms2;  // > 0 (Vorbedingung)
  const float frac = (decel_ms2 - p.on_ms2) / span;
  const float duty = p.tail_pct + frac * (100.0f - p.tail_pct);
  return static_cast<int>(duty + 0.5f);      // kaufmaennisch runden
}

}  // namespace logic
