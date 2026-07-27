// Host-Unit-Test der IMU-Gesundheitsueberwachung (NFR-TST-03). Laeuft ohne
// ESP32: pio test -e native
#include <unity.h>
#include "imu_health.h"
#include "config.h"

using namespace logic;

constexpr float G = 9.80665f;

void test_healthy_varying_gravity_stays_ok() {
  ImuHealth h;
  ImuHealthOutput out{};
  for (int i = 0; i < 10; ++i) {
    // minimal variiert (Sensorrauschen simuliert), damit auch der
    // Frozen-Check nicht faelschlich anspringt.
    out = h.update(true, 0.001f * i, 0.0f, G, 0.0f, i * 10);
  }
  TEST_ASSERT_TRUE(out.plausible);
  TEST_ASSERT_EQUAL_INT((int)ImuHealthState::OK, (int)out.state);
  TEST_ASSERT_FALSE(out.degraded);
  TEST_ASSERT_FALSE(out.request_recovery);
}

void test_near_zero_magnitude_is_implausible() {
  ImuHealth h;
  // Wertebereichsverletzung: Magnitude ~0 (keine Schwerkraft vorhanden).
  const ImuHealthOutput out = h.update(true, 0.001f, 0.0f, 0.0f, 0.0f, 0);
  TEST_ASSERT_FALSE(out.plausible);
}

void test_n_implausible_cycles_trigger_recovery() {
  ImuHealth h;
  ImuHealthOutput out{};
  for (uint32_t i = 0; i < IMU_FAIL_LIMIT; ++i) {
    // pro Zyklus leicht variiert, damit NICHT der Frozen-Pfad mitgreift --
    // isoliert testet das den Wertebereichs-Pfad.
    out = h.update(true, 0.001f * i, 0.0f, 0.0f, 0.0f, i * 10);
  }
  TEST_ASSERT_EQUAL_INT((int)ImuHealthState::RECOVERING, (int)out.state);
  TEST_ASSERT_TRUE(out.degraded);
  TEST_ASSERT_TRUE(out.request_recovery);
  TEST_ASSERT_EQUAL_INT(1, out.recovery_stage);
}

void test_identical_values_over_limit_are_frozen() {
  ImuHealth h;
  // Erster Aufruf legt nur den Vorwert fest (kein Vergleichspartner).
  ImuHealthOutput out = h.update(true, 0.0f, 0.0f, G, 0.0f, 0);
  TEST_ASSERT_TRUE(out.plausible);
  for (uint32_t i = 1; i <= IMU_FROZEN_LIMIT; ++i) {
    out = h.update(true, 0.0f, 0.0f, G, 0.0f, i * 10);
  }
  TEST_ASSERT_FALSE(out.plausible);
}

void test_failed_reads_trigger_recovery() {
  ImuHealth h;
  ImuHealthOutput out{};
  for (uint32_t i = 0; i < IMU_FAIL_LIMIT; ++i) {
    out = h.update(false, 0.0f, 0.0f, 0.0f, 0.0f, i * 10);
  }
  TEST_ASSERT_EQUAL_INT((int)ImuHealthState::RECOVERING, (int)out.state);
  TEST_ASSERT_TRUE(out.degraded);
}

void test_recovery_stage_escalates_from_1_to_2() {
  ImuHealth h;
  uint32_t t = 0;
  ImuHealthOutput first_bad{};
  for (uint32_t i = 0; i < IMU_FAIL_LIMIT; ++i) {
    first_bad = h.update(false, 0.0f, 0.0f, 0.0f, 0.0f, t);
    t += 10;
  }
  TEST_ASSERT_EQUAL_INT(1, first_bad.recovery_stage);  // erster Versuch: Soft-Reinit

  const ImuHealthOutput second = h.update(false, 0.0f, 0.0f, 0.0f, 0.0f, t);
  TEST_ASSERT_EQUAL_INT((int)ImuHealthState::RECOVERING, (int)second.state);
  TEST_ASSERT_EQUAL_INT(2, second.recovery_stage);  // ab 2. Versuch: SCL-Clock-Release
}

void test_recovery_exhausted_enters_failed_and_degraded() {
  ImuHealth h;
  uint32_t t = 0;
  ImuHealthOutput out{};
  // IMU_FAIL_LIMIT Fehlzyklen bis RECOVERING, dann IMU_RECOVERY_MAX_ATTEMPTS
  // weitere erfolglose Versuche bis FAILED.
  for (uint32_t i = 0; i < IMU_FAIL_LIMIT + IMU_RECOVERY_MAX_ATTEMPTS; ++i) {
    out = h.update(false, 0.0f, 0.0f, 0.0f, 0.0f, t);
    t += 10;
  }
  TEST_ASSERT_EQUAL_INT((int)ImuHealthState::FAILED, (int)out.state);
  TEST_ASSERT_TRUE(out.degraded);
  TEST_ASSERT_FALSE(out.request_recovery);  // Uebergangszyklus: kein Schritt mehr
}

void test_background_retry_rate_limited_in_failed() {
  ImuHealth h;
  uint32_t t = 0;
  for (uint32_t i = 0; i < IMU_FAIL_LIMIT + IMU_RECOVERY_MAX_ATTEMPTS; ++i) {
    h.update(false, 0.0f, 0.0f, 0.0f, 0.0f, t);
    t += 10;
  }
  // Direkt danach (< IMU_RECOVERY_MIN_INTERVAL_MS): kein neuer Versuch.
  const ImuHealthOutput soon = h.update(false, 0.0f, 0.0f, 0.0f, 0.0f, t + 100);
  TEST_ASSERT_EQUAL_INT((int)ImuHealthState::FAILED, (int)soon.state);
  TEST_ASSERT_FALSE(soon.request_recovery);

  // Nach Ablauf des Intervalls: neuer Hintergrund-Versuch, Stufe 2.
  const ImuHealthOutput later = h.update(false, 0.0f, 0.0f, 0.0f, 0.0f, t + IMU_RECOVERY_MIN_INTERVAL_MS);
  TEST_ASSERT_TRUE(later.request_recovery);
  TEST_ASSERT_EQUAL_INT(2, later.recovery_stage);
}

void test_successful_read_after_recovery_returns_to_ok_and_resets_counters() {
  ImuHealth h;
  uint32_t t = 0;
  for (uint32_t i = 0; i < IMU_FAIL_LIMIT; ++i) {
    h.update(false, 0.0f, 0.0f, 0.0f, 0.0f, t);
    t += 10;
  }
  // Ein gesunder, plausibler Read genuegt fuer sofortige Erholung.
  const ImuHealthOutput recovered = h.update(true, 0.001f, 0.0f, G, 0.0f, t);
  TEST_ASSERT_TRUE(recovered.plausible);
  TEST_ASSERT_EQUAL_INT((int)ImuHealthState::OK, (int)recovered.state);
  TEST_ASSERT_FALSE(recovered.degraded);

  // Zaehler zurueckgesetzt: erneut IMU_FAIL_LIMIT-1 Fehlzyklen loesen NOCH
  // KEINE Recovery aus (Nachweis, dass fail_streak_ wirklich bei 0 startet).
  ImuHealthOutput out{};
  for (uint32_t i = 0; i < IMU_FAIL_LIMIT - 1; ++i) {
    t += 10;
    out = h.update(false, 0.0f, 0.0f, 0.0f, 0.0f, t);
  }
  TEST_ASSERT_EQUAL_INT((int)ImuHealthState::OK, (int)out.state);
  TEST_ASSERT_FALSE(out.request_recovery);
}

void test_large_jump_between_valid_samples_is_implausible() {
  ImuHealth h;
  h.update(true, 0.0f, 0.0f, G, 0.0f, 0);  // Baseline: ruhige Lage
  // Sprung: weit ausserhalb IMU_ACCEL_MAX_SLEW_MS2, aber selbst noch im
  // gueltigen Wertebereich (Magnitude < IMU_ACCEL_MAX_MAGNITUDE_MS2).
  const ImuHealthOutput out = h.update(true, 90.0f, 0.0f, G, 0.0f, 10);
  TEST_ASSERT_FALSE(out.plausible);
}

void test_gradual_change_within_slew_stays_plausible() {
  ImuHealth h;
  ImuHealthOutput out{};
  // Simulierter echter Bremsvorgang: kleine Schritte pro Zyklus.
  for (int i = 0; i <= 20; ++i) {
    out = h.update(true, 0.0f, 0.2f * i, G, 0.0f, i * 10);
  }
  TEST_ASSERT_TRUE(out.plausible);
}

void test_rejected_jump_still_updates_baseline_so_next_sample_recovers() {
  ImuHealth h;
  h.update(true, 0.0f, 0.0f, G, 0.0f, 0);
  const ImuHealthOutput jump = h.update(true, 90.0f, 0.0f, G, 0.0f, 10);
  TEST_ASSERT_FALSE(jump.plausible);
  // Naechstes Sample nahe am (jetzt aktualisierten) Sprungwert -- kein
  // erneuter Sprung mehr, also wieder plausibel (Ein-Zyklus-Unterdrueckung,
  // kein Dauerblock durch einen einzelnen echten harten Bremsstoss).
  const ImuHealthOutput after = h.update(true, 90.5f, 0.0f, G, 0.0f, 20);
  TEST_ASSERT_TRUE(after.plausible);
}

void test_escalation_trusted_only_after_n_consecutive_plausible() {
  ImuHealth h;
  ImuHealthOutput out{};
  for (uint32_t i = 0; i < IMU_ESCALATION_CONFIRM_CYCLES; ++i) {
    out = h.update(true, 0.001f * i, 0.0f, G, 0.0f, i * 10);
    if (i + 1 < IMU_ESCALATION_CONFIRM_CYCLES) {
      TEST_ASSERT_FALSE(out.escalation_trusted);
    }
  }
  TEST_ASSERT_TRUE(out.escalation_trusted);
}

void test_escalation_trust_resets_immediately_on_implausible_cycle() {
  ImuHealth h;
  uint32_t t = 0;
  for (uint32_t i = 0; i < IMU_ESCALATION_CONFIRM_CYCLES; ++i) {
    h.update(true, 0.001f * i, 0.0f, G, 0.0f, t);
    t += 10;
  }
  // Ein einzelner unplausibler Zyklus (Sprung) wirft das Vertrauen sofort
  // zurueck, obwohl es zuvor schon aufgebaut war.
  const ImuHealthOutput out = h.update(true, 90.0f, 0.0f, G, 0.0f, t);
  TEST_ASSERT_FALSE(out.plausible);
  TEST_ASSERT_FALSE(out.escalation_trusted);
}

void test_single_lucky_sample_amid_outage_does_not_regain_escalation_trust() {
  ImuHealth h;
  uint32_t t = 0;
  // Ausfallphase: mehrere Fehl-Reads (Kurzschluss-Fehlerinjektion).
  for (int i = 0; i < 4; ++i) {
    h.update(false, 0.0f, 0.0f, 0.0f, 0.0f, t);
    t += 10;
  }
  // Ein einzelnes "Glueck"-Sample: besteht Wertebereich, Frozen (kein
  // Vorwert vorhanden, da alle vorherigen Reads fehlgeschlagen sind) und
  // Sprung (aus demselben Grund trivial erfuellt) -- damit technisch
  // plausibel, genau wie im gemeldeten Fehlerfall (imu_plausible=1 waehrend
  // des Ausfalls).
  const ImuHealthOutput out = h.update(true, 0.0f, 0.0f, G, 0.0f, t);
  TEST_ASSERT_TRUE(out.plausible);
  TEST_ASSERT_FALSE(out.escalation_trusted);  // main.cpp erzwingt tail_input=0
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_healthy_varying_gravity_stays_ok);
  RUN_TEST(test_near_zero_magnitude_is_implausible);
  RUN_TEST(test_n_implausible_cycles_trigger_recovery);
  RUN_TEST(test_identical_values_over_limit_are_frozen);
  RUN_TEST(test_failed_reads_trigger_recovery);
  RUN_TEST(test_recovery_stage_escalates_from_1_to_2);
  RUN_TEST(test_recovery_exhausted_enters_failed_and_degraded);
  RUN_TEST(test_background_retry_rate_limited_in_failed);
  RUN_TEST(test_successful_read_after_recovery_returns_to_ok_and_resets_counters);
  RUN_TEST(test_large_jump_between_valid_samples_is_implausible);
  RUN_TEST(test_gradual_change_within_slew_stays_plausible);
  RUN_TEST(test_rejected_jump_still_updates_baseline_so_next_sample_recovers);
  RUN_TEST(test_escalation_trusted_only_after_n_consecutive_plausible);
  RUN_TEST(test_escalation_trust_resets_immediately_on_implausible_cycle);
  RUN_TEST(test_single_lucky_sample_amid_outage_does_not_regain_escalation_trust);
  return UNITY_END();
}
