// imu_mount_orientation.h — Rueckabbildung der Einbaulage (IMU_MOUNT_SIGN_*)
// REINE LOGIK, hardwarefrei (NFR-TST-01): kein #include <Arduino.h>. Wird
// ausschliesslich von lib/drivers/imu_driver.cpp (Treibergrenze) aufgerufen,
// s. Kommentar bei IMU_MOUNT_SIGN_X/Y/Z in config.h fuer den mechanischen
// Hintergrund (180-Grad-Drehung der Platine in ihrer eigenen Ebene).
//
// Eigener, zu drivers::ImuSample strukturgleicher Typ statt eines Includes
// von lib/drivers/imu_driver.h: so bleibt dieser Header (und sein Host-Test)
// frei vom Adafruit-/Arduino-Abhaengigkeitsbaum des Treibers.
#pragma once
#include "config.h"

namespace logic {

struct RawImuSample {
  float accel_x_ms2, accel_y_ms2, accel_z_ms2;
  float gyro_x_rads, gyro_y_rads, gyro_z_rads;
};

// Accel UND Gyro erhalten dieselbe Transformation (s. config.h-Kommentar) --
// sonst gilt dtheta/dt = omega_x nach der Rueckabbildung nicht mehr.
inline void applyMountOrientation(RawImuSample& s) {
  s.accel_x_ms2 *= IMU_MOUNT_SIGN_X;
  s.accel_y_ms2 *= IMU_MOUNT_SIGN_Y;
  s.accel_z_ms2 *= IMU_MOUNT_SIGN_Z;
  s.gyro_x_rads *= IMU_MOUNT_SIGN_X;
  s.gyro_y_rads *= IMU_MOUNT_SIGN_Y;
  s.gyro_z_rads *= IMU_MOUNT_SIGN_Z;
}

}  // namespace logic
