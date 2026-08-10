// Host-Unit-Test der Einbaulage-Rueckabbildung (NFR-TST-03). Laeuft ohne
// ESP32: pio test -e native. Deckt die 180-Grad-Drehung (R_z(180)) der
// Platine ab, s. IMU_MOUNT_SIGN_X/Y/Z in config.h.
#include <unity.h>
#include <cmath>
#include "imu_mount_orientation.h"

using namespace logic;

namespace {
RawImuSample sample() {
  RawImuSample s;
  s.accel_x_ms2 = 1.0f;
  s.accel_y_ms2 = 2.0f;
  s.accel_z_ms2 = 3.0f;
  s.gyro_x_rads = 0.1f;
  s.gyro_y_rads = 0.2f;
  s.gyro_z_rads = 0.3f;
  return s;
}

float norm(const RawImuSample& s) {
  return std::sqrt(s.accel_x_ms2 * s.accel_x_ms2 + s.accel_y_ms2 * s.accel_y_ms2 +
                    s.accel_z_ms2 * s.accel_z_ms2);
}
}  // namespace

void test_x_and_y_are_inverted() {
  RawImuSample s = sample();
  applyMountOrientation(s);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, -1.0f, s.accel_x_ms2);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, -2.0f, s.accel_y_ms2);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, -0.1f, s.gyro_x_rads);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, -0.2f, s.gyro_y_rads);
}

void test_z_stays_unchanged() {
  RawImuSample s = sample();
  applyMountOrientation(s);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 3.0f, s.accel_z_ms2);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.3f, s.gyro_z_rads);
}

void test_accel_norm_is_invariant() {
  RawImuSample s = sample();
  const float norm_before = norm(s);
  applyMountOrientation(s);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, norm_before, norm(s));
}

void test_double_application_is_identity() {
  RawImuSample s = sample();
  const RawImuSample original = s;
  applyMountOrientation(s);
  applyMountOrientation(s);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, original.accel_x_ms2, s.accel_x_ms2);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, original.accel_y_ms2, s.accel_y_ms2);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, original.accel_z_ms2, s.accel_z_ms2);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, original.gyro_x_rads, s.gyro_x_rads);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, original.gyro_y_rads, s.gyro_y_rads);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, original.gyro_z_rads, s.gyro_z_rads);
}

void test_zero_sample_stays_zero() {
  RawImuSample s{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  applyMountOrientation(s);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, s.accel_x_ms2);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, s.accel_y_ms2);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, s.accel_z_ms2);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, s.gyro_x_rads);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, s.gyro_y_rads);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, s.gyro_z_rads);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_x_and_y_are_inverted);
  RUN_TEST(test_z_stays_unchanged);
  RUN_TEST(test_accel_norm_is_invariant);
  RUN_TEST(test_double_application_is_identity);
  RUN_TEST(test_zero_sample_stays_zero);
  return UNITY_END();
}
