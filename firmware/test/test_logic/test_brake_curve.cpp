// Host-Unit-Test der Bremskennlinie (NFR-TST-03). Laeuft ohne ESP32:
//   pio test -e native
#include <unity.h>
#include "brake_curve.h"

using namespace logic;

static BrakeCurveParams P;  // Defaults: on=2, full=5, tail=20

void test_below_threshold_is_taillight() {
  TEST_ASSERT_EQUAL_INT(20, brakeDutyPercent(0.0f, P));
  TEST_ASSERT_EQUAL_INT(20, brakeDutyPercent(1.9f, P));
  TEST_ASSERT_EQUAL_INT(20, brakeDutyPercent(2.0f, P));  // Schwelle inklusive
}

void test_saturation_is_full() {
  TEST_ASSERT_EQUAL_INT(100, brakeDutyPercent(5.0f, P));
  TEST_ASSERT_EQUAL_INT(100, brakeDutyPercent(8.0f, P));
}

void test_midpoint_is_linear() {
  // decel = 3.5 -> (3.5-2)/(5-2)=0.5 -> 20 + 0.5*80 = 60 %
  TEST_ASSERT_EQUAL_INT(60, brakeDutyPercent(3.5f, P));
}

void test_monotonic() {
  int prev = brakeDutyPercent(2.0f, P);
  for (float d = 2.1f; d <= 5.0f; d += 0.1f) {
    int cur = brakeDutyPercent(d, P);
    TEST_ASSERT_TRUE(cur >= prev);
    prev = cur;
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_below_threshold_is_taillight);
  RUN_TEST(test_saturation_is_full);
  RUN_TEST(test_midpoint_is_linear);
  RUN_TEST(test_monotonic);
  return UNITY_END();
}
