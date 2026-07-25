// Host-Unit-Test des Komplementaerfilters (NFR-TST-03). Laeuft ohne ESP32:
//   pio test -e native
#include <unity.h>
#include <cmath>
#include "motion_filter.h"
#include "config.h"

using namespace logic;

constexpr float G = 9.80665f;

void test_calm_level_pose_yields_near_zero() {
  MotionFilter filter;
  MotionInput in{0.0f, 0.0f, G, 0.0f, 0.01f};
  const float decel = filter.update(in);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, decel);
}

void test_simulated_braking_exceeds_threshold() {
  MotionFilter filter;
  // Eben (Z=g). Brems-Richtung am realen Board verifiziert: Bremsen =
  // positive Y-Beschleunigung (MOTION_BRAKE_SIGN, config.h).
  MotionInput in{0.0f, 3.5f, G, 0.0f, 0.01f};
  const float decel = filter.update(in);
  TEST_ASSERT_TRUE(decel > BRAKE_ON_MS2);
}

void test_accelerating_in_non_braking_direction_yields_near_zero() {
  MotionFilter filter;
  // Sprint/Antritt: negative Y-Beschleunigung (Nicht-Brems-Richtung, s.
  // verifizierte Konvention oben) darf das Bremslicht nicht ausloesen.
  MotionInput in{0.0f, -3.5f, G, 0.0f, 0.01f};
  const float decel = filter.update(in);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, decel);
}

void test_gravity_compensation_at_tilted_pose() {
  MotionFilter filter;
  const float tilt_rad = 0.5236f;  // 30 Grad, fester Neigungswinkel
  const float ay = G * sinf(tilt_rad);
  const float az = G * cosf(tilt_rad);
  MotionInput in{0.0f, ay, az, 0.0f, 0.01f};  // gx=0: keine echte Drehung, nur Neigung

  // Filter einschwingen lassen (alpha=0.98 -> Restfehler ~0.98^n).
  float decel = 0.0f;
  for (int i = 0; i < 500; ++i) {
    decel = filter.update(in);
  }
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, decel);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_calm_level_pose_yields_near_zero);
  RUN_TEST(test_simulated_braking_exceeds_threshold);
  RUN_TEST(test_accelerating_in_non_braking_direction_yields_near_zero);
  RUN_TEST(test_gravity_compensation_at_tilted_pose);
  return UNITY_END();
}
