// motion_filter.cpp — Umsetzung Komplementaerfilter. Rein, hardwarefrei.
#include "motion_filter.h"
#include <cmath>

namespace logic {

MotionFilter::MotionFilter(const MotionParams& params) : params_(params) {}

float MotionFilter::update(const MotionInput& in) {
  // Neigungswinkel (Pitch) aus der Beschleunigung: nur im Ruhezustand exakt
  // (misst dann reine Schwerkraftrichtung), driftet nicht, ist aber
  // kurzfristig durch echte Beschleunigung verfaelscht.
  const float pitch_from_accel = atan2f(in.accel_y_ms2, in.accel_z_ms2);

  // Komplementaerfilter: kurzfristig Gyro-Integration (reaktionsschnell,
  // driftet), langfristig durch die Accel-Referenz korrigiert. alpha aus
  // config.h (COMPL_FILTER_ALPHA, Bible Kap. 6.4).
  pitch_rad_ = params_.alpha * (pitch_rad_ + in.gyro_x_rads * in.dt_s)
             + (1.0f - params_.alpha) * pitch_from_accel;

  // Gravitationsanteil auf der Y-Achse (Fahrtrichtung) herausrechnen.
  const float gravity_y = params_.gravity_ms2 * sinf(pitch_rad_);
  const float linear_accel_y = in.accel_y_ms2 - gravity_y;

  // Nur die Brems-Richtung liefern (brake_sign, TODO(offen) s. config.h):
  // Beschleunigen in Nicht-Brems-Richtung (z. B. Sprint) wird auf 0 gekappt,
  // damit es nicht faelschlich das Bremslicht ausloest.
  const float braking_accel_ms2 = params_.brake_sign * linear_accel_y;
  return braking_accel_ms2 > 0.0f ? braking_accel_ms2 : 0.0f;
}

}  // namespace logic
