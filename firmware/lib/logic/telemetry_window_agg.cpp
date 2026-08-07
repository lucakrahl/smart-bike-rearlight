// telemetry_window_agg.cpp — Umsetzung Fensteraggregation. Rein, hardwarefrei.
#include "telemetry_window_agg.h"
#include <cmath>

namespace logic {

void TelemetryWindowAgg::add(float dt_s, float norm_delta, float jerk, MotionRegime regime,
                              uint32_t loop_us) {
  const float jerk_abs = fabsf(jerk);
  if (!has_samples_) {
    norm_delta_min_ = norm_delta;
    norm_delta_max_ = norm_delta;
    jerk_max_ = jerk_abs;
    has_samples_ = true;
  } else {
    if (norm_delta < norm_delta_min_) norm_delta_min_ = norm_delta;
    if (norm_delta > norm_delta_max_) norm_delta_max_ = norm_delta;
    if (jerk_abs > jerk_max_) jerk_max_ = jerk_abs;
  }

  switch (regime) {
    case MotionRegime::Static:
      if (regime_static_n_ < 255u) ++regime_static_n_;
      break;
    case MotionRegime::Dynamic:
      if (regime_dynamic_n_ < 255u) ++regime_dynamic_n_;
      break;
    case MotionRegime::Shock:
      if (regime_shock_n_ < 255u) ++regime_shock_n_;
      break;
  }

  // Saettigen VOR dem Vergleich (nicht erst beim Zusammenfassen in
  // snapshotAndReset()), damit ein einzelner extremer Ausreisser den
  // gesaettigten Maximalwert nicht durch einen spaeteren, kleineren-aber-
  // noch-unsaettigten Wert wieder "senkt".
  uint32_t dt_ms = static_cast<uint32_t>(dt_s * 1000.0f + 0.5f);
  if (dt_ms > 255u) dt_ms = 255u;
  if (dt_ms > dt_max_ms_) dt_max_ms_ = dt_ms;

  uint32_t loop_us_clamped = loop_us;
  if (loop_us_clamped > 65535u) loop_us_clamped = 65535u;
  if (loop_us_clamped > loop_max_us_) loop_max_us_ = loop_us_clamped;
}

WindowAggSnapshot TelemetryWindowAgg::snapshotAndReset() {
  WindowAggSnapshot snap;
  if (has_samples_) {
    snap.norm_delta_min = norm_delta_min_;
    snap.norm_delta_max = norm_delta_max_;
    snap.jerk_max = jerk_max_;
  }
  // Sonst bleiben die WindowAggSnapshot-Defaults (0.0f) stehen -- leeres
  // Fenster, s. Vertrag Kap. 3.3.
  snap.regime_static_n = static_cast<uint8_t>(regime_static_n_);
  snap.regime_dynamic_n = static_cast<uint8_t>(regime_dynamic_n_);
  snap.regime_shock_n = static_cast<uint8_t>(regime_shock_n_);
  snap.dt_max_ms = static_cast<uint8_t>(dt_max_ms_);
  snap.loop_max_us = static_cast<uint16_t>(loop_max_us_);

  has_samples_ = false;
  norm_delta_min_ = 0.0f;
  norm_delta_max_ = 0.0f;
  jerk_max_ = 0.0f;
  regime_static_n_ = 0;
  regime_dynamic_n_ = 0;
  regime_shock_n_ = 0;
  dt_max_ms_ = 0;
  loop_max_us_ = 0;
  return snap;
}

}  // namespace logic
