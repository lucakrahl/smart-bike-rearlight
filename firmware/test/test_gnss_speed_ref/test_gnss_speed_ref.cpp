// Host-Unit-Test der GNSS-Referenzbeschleunigung (NFR-TST-03, E1, docs/
// BLE_Frame_v3_Schnittstelle.md Kap. 1a/6). Laeuft ohne ESP32:
// pio test -e native
#include <unity.h>
#include "gnss_speed_ref.h"

using namespace logic;

namespace {
// Ein individuell gueltiges Sample (alle vier Kriterien komfortabel erfuellt).
GnssSpeedRefInput validSample(float speed_mps) {
  GnssSpeedRefInput in;
  in.speed_mps = speed_mps;
  in.sats = 8;
  in.hdop = 1.0f;
  in.location_valid = true;
  in.dt_s = 1.0f;
  return in;
}
}  // namespace

void test_first_fix_without_predecessor_is_invalid() {
  GnssSpeedRef ref;
  const GnssSpeedRefOutput out = ref.update(validSample(5.0f));
  TEST_ASSERT_FALSE(out.valid);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, out.accel_ms2);
}

void test_two_consecutive_valid_fixes_yield_valid_output() {
  GnssSpeedRef ref;
  ref.update(validSample(5.0f));
  const GnssSpeedRefOutput out = ref.update(validSample(4.0f));
  TEST_ASSERT_TRUE(out.valid);
}

void test_sign_convention_deceleration_is_positive() {
  GnssSpeedRef ref;
  ref.update(validSample(10.0f));
  const GnssSpeedRefOutput out = ref.update(validSample(8.0f));  // 2 m/s Abnahme in 1 s
  TEST_ASSERT_TRUE(out.valid);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, out.accel_ms2);  // positiv = Verzoegerung
}

void test_sign_convention_acceleration_is_negative() {
  GnssSpeedRef ref;
  ref.update(validSample(5.0f));
  const GnssSpeedRefOutput out = ref.update(validSample(7.0f));  // Zunahme
  TEST_ASSERT_TRUE(out.valid);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -2.0f, out.accel_ms2);
}

// -- Jedes der vier Gueltigkeitskriterien einzeln verletzt --

void test_criterion_location_invalid_breaks_the_chain() {
  GnssSpeedRef ref;
  ref.update(validSample(5.0f));
  GnssSpeedRefInput bad = validSample(5.0f);
  bad.location_valid = false;
  const GnssSpeedRefOutput out_bad = ref.update(bad);
  TEST_ASSERT_FALSE(out_bad.valid);

  // Kette gebrochen: das naechste (wieder gueltige) Sample ist erneut "der
  // erste Fix ohne Vorgaenger" -- noch nicht valid.
  const GnssSpeedRefOutput out_next = ref.update(validSample(5.0f));
  TEST_ASSERT_FALSE(out_next.valid);
}

void test_criterion_too_few_sats_breaks_the_chain() {
  GnssSpeedRef ref;
  ref.update(validSample(5.0f));
  GnssSpeedRefInput bad = validSample(5.0f);
  bad.sats = 3;
  const GnssSpeedRefOutput out_bad = ref.update(bad);
  TEST_ASSERT_FALSE(out_bad.valid);
}

void test_criterion_hdop_too_high_breaks_the_chain() {
  GnssSpeedRef ref;
  ref.update(validSample(5.0f));
  GnssSpeedRefInput bad = validSample(5.0f);
  bad.hdop = 3.0f;
  const GnssSpeedRefOutput out_bad = ref.update(bad);
  TEST_ASSERT_FALSE(out_bad.valid);
}

void test_criterion_speed_too_low_breaks_the_chain() {
  GnssSpeedRef ref;
  ref.update(validSample(5.0f));
  GnssSpeedRefInput bad = validSample(1.0f);  // < 1,5 m/s
  const GnssSpeedRefOutput out_bad = ref.update(bad);
  TEST_ASSERT_FALSE(out_bad.valid);
}

// -- Grenzfaelle exakt auf der Schwelle --

void test_boundary_sats_exactly_5_is_valid() {
  GnssSpeedRef ref;
  GnssSpeedRefInput a = validSample(5.0f);
  a.sats = 5;  // Schwelle: sats >= 5
  ref.update(a);
  GnssSpeedRefInput b = validSample(5.0f);
  b.sats = 5;
  const GnssSpeedRefOutput out = ref.update(b);
  TEST_ASSERT_TRUE(out.valid);
}

void test_boundary_sats_4_is_invalid() {
  GnssSpeedRef ref;
  ref.update(validSample(5.0f));
  GnssSpeedRefInput bad = validSample(5.0f);
  bad.sats = 4;
  const GnssSpeedRefOutput out = ref.update(bad);
  TEST_ASSERT_FALSE(out.valid);
}

void test_boundary_hdop_exactly_2p5_is_valid() {
  GnssSpeedRef ref;
  GnssSpeedRefInput a = validSample(5.0f);
  a.hdop = 2.5f;  // Schwelle: hdop <= 2,5
  ref.update(a);
  GnssSpeedRefInput b = validSample(5.0f);
  b.hdop = 2.5f;
  const GnssSpeedRefOutput out = ref.update(b);
  TEST_ASSERT_TRUE(out.valid);
}

void test_boundary_hdop_above_2p5_is_invalid() {
  GnssSpeedRef ref;
  ref.update(validSample(5.0f));
  GnssSpeedRefInput bad = validSample(5.0f);
  bad.hdop = 2.51f;
  const GnssSpeedRefOutput out = ref.update(bad);
  TEST_ASSERT_FALSE(out.valid);
}

void test_boundary_speed_exactly_1p5_is_invalid() {
  // Schwelle ist strikt: Geschwindigkeit > 1,5 m/s (nicht >=).
  GnssSpeedRef ref;
  ref.update(validSample(5.0f));
  GnssSpeedRefInput bad = validSample(1.5f);
  const GnssSpeedRefOutput out = ref.update(bad);
  TEST_ASSERT_FALSE(out.valid);
}

void test_boundary_speed_just_above_1p5_is_valid() {
  GnssSpeedRef ref;
  GnssSpeedRefInput a = validSample(1.6f);
  ref.update(a);
  GnssSpeedRefInput b = validSample(1.6f);
  const GnssSpeedRefOutput out = ref.update(b);
  TEST_ASSERT_TRUE(out.valid);
}

void test_reset_forces_fresh_pair() {
  GnssSpeedRef ref;
  ref.update(validSample(5.0f));
  ref.reset();
  const GnssSpeedRefOutput out = ref.update(validSample(5.0f));
  TEST_ASSERT_FALSE(out.valid);  // wieder "erster Fix ohne Vorgaenger"
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_first_fix_without_predecessor_is_invalid);
  RUN_TEST(test_two_consecutive_valid_fixes_yield_valid_output);
  RUN_TEST(test_sign_convention_deceleration_is_positive);
  RUN_TEST(test_sign_convention_acceleration_is_negative);
  RUN_TEST(test_criterion_location_invalid_breaks_the_chain);
  RUN_TEST(test_criterion_too_few_sats_breaks_the_chain);
  RUN_TEST(test_criterion_hdop_too_high_breaks_the_chain);
  RUN_TEST(test_criterion_speed_too_low_breaks_the_chain);
  RUN_TEST(test_boundary_sats_exactly_5_is_valid);
  RUN_TEST(test_boundary_sats_4_is_invalid);
  RUN_TEST(test_boundary_hdop_exactly_2p5_is_valid);
  RUN_TEST(test_boundary_hdop_above_2p5_is_invalid);
  RUN_TEST(test_boundary_speed_exactly_1p5_is_invalid);
  RUN_TEST(test_boundary_speed_just_above_1p5_is_valid);
  RUN_TEST(test_reset_forces_fresh_pair);
  return UNITY_END();
}
