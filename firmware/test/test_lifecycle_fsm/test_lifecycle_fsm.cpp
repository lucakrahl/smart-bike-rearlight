// Host-Unit-Test der R1-Zustandsmaschine (NFR-TST-03). Laeuft ohne ESP32:
//   pio test -e native
#include <unity.h>
#include "lifecycle_fsm.h"
#include "config.h"

using namespace logic;

void test_stays_init_before_ready_or_timeout() {
  LifecycleFsm fsm;
  auto a = fsm.update(false, 0);
  TEST_ASSERT_EQUAL_INT((int)SystemState::Init, (int)a.state);
  TEST_ASSERT_FALSE(a.degraded);

  auto b = fsm.update(false, INIT_TIMEOUT_MS - 1);
  TEST_ASSERT_EQUAL_INT((int)SystemState::Init, (int)b.state);
  TEST_ASSERT_FALSE(b.degraded);
}

void test_ready_flag_transitions_to_run_before_timeout() {
  LifecycleFsm fsm;
  fsm.update(false, 0);
  auto out = fsm.update(true, 1000);              // deutlich vor INIT_TIMEOUT_MS
  TEST_ASSERT_EQUAL_INT((int)SystemState::Run, (int)out.state);
  TEST_ASSERT_FALSE(out.degraded);
}

void test_timeout_without_ready_flag_yields_degraded_run() {
  LifecycleFsm fsm;
  fsm.update(false, 0);
  auto before = fsm.update(false, INIT_TIMEOUT_MS - 1);
  TEST_ASSERT_EQUAL_INT((int)SystemState::Init, (int)before.state);

  auto after = fsm.update(false, INIT_TIMEOUT_MS);
  TEST_ASSERT_EQUAL_INT((int)SystemState::Run, (int)after.state);
  TEST_ASSERT_TRUE(after.degraded);
}

void test_ready_wins_over_timeout_at_exact_boundary() {
  LifecycleFsm fsm;
  fsm.update(false, 0);
  // Beide Bedingungen treffen im selben Tick zu: ready wird zuerst geprueft.
  auto out = fsm.update(true, INIT_TIMEOUT_MS);
  TEST_ASSERT_EQUAL_INT((int)SystemState::Run, (int)out.state);
  TEST_ASSERT_FALSE(out.degraded);
}

void test_first_call_defines_init_start() {
  LifecycleFsm fsm;
  fsm.update(false, 10000);                        // erster Aufruf definiert t0
  auto before = fsm.update(false, 10000 + INIT_TIMEOUT_MS - 1);
  TEST_ASSERT_EQUAL_INT((int)SystemState::Init, (int)before.state);

  auto after = fsm.update(false, 10000 + INIT_TIMEOUT_MS);
  TEST_ASSERT_EQUAL_INT((int)SystemState::Run, (int)after.state);
  TEST_ASSERT_TRUE(after.degraded);
}

void test_run_is_sticky_even_if_ready_flag_drops_again() {
  LifecycleFsm fsm;
  fsm.update(false, 0);
  fsm.update(true, 1000);                          // -> Run, degraded=false
  auto out = fsm.update(false, 2000);               // ready faellt wieder
  TEST_ASSERT_EQUAL_INT((int)SystemState::Run, (int)out.state);
  TEST_ASSERT_FALSE(out.degraded);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_stays_init_before_ready_or_timeout);
  RUN_TEST(test_ready_flag_transitions_to_run_before_timeout);
  RUN_TEST(test_timeout_without_ready_flag_yields_degraded_run);
  RUN_TEST(test_ready_wins_over_timeout_at_exact_boundary);
  RUN_TEST(test_first_call_defines_init_start);
  RUN_TEST(test_run_is_sticky_even_if_ready_flag_drops_again);
  return UNITY_END();
}
