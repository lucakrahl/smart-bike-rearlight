// Host-Unit-Test der v3-Fensteraggregation (NFR-TST-03, docs/
// BLE_Frame_v3_Schnittstelle.md Kap. 3.2/3.3). Laeuft ohne ESP32:
// pio test -e native
#include <unity.h>
#include "telemetry_window_agg.h"

using namespace logic;

void test_normal_case_10_samples() {
  TelemetryWindowAgg agg;
  // 10 Samples bei nominell 10 ms (100 Hz), norm_delta/jerk variierend.
  agg.add(0.010f, 0.05f, 0.10f, MotionRegime::Static, 500u);
  agg.add(0.010f, -0.03f, 0.20f, MotionRegime::Static, 520u);
  agg.add(0.010f, 0.80f, 0.90f, MotionRegime::Dynamic, 510u);
  agg.add(0.011f, 1.20f, 0.30f, MotionRegime::Dynamic, 600u);
  agg.add(0.009f, -0.10f, 1.80f, MotionRegime::Shock, 700u);
  agg.add(0.010f, 0.60f, 0.40f, MotionRegime::Dynamic, 550u);
  agg.add(0.010f, 0.02f, 0.05f, MotionRegime::Static, 480u);
  agg.add(0.010f, 0.95f, 0.60f, MotionRegime::Dynamic, 590u);
  agg.add(0.010f, -0.05f, 0.15f, MotionRegime::Static, 505u);
  agg.add(0.010f, 0.30f, 0.25f, MotionRegime::Dynamic, 520u);

  const WindowAggSnapshot snap = agg.snapshotAndReset();
  TEST_ASSERT_EQUAL_FLOAT(-0.10f, snap.norm_delta_min);
  TEST_ASSERT_EQUAL_FLOAT(1.20f, snap.norm_delta_max);
  TEST_ASSERT_EQUAL_FLOAT(1.80f, snap.jerk_max);  // Betragsmaximum
  TEST_ASSERT_EQUAL_UINT8(4, snap.regime_static_n);
  TEST_ASSERT_EQUAL_UINT8(5, snap.regime_dynamic_n);
  TEST_ASSERT_EQUAL_UINT8(1, snap.regime_shock_n);
  TEST_ASSERT_EQUAL_UINT8(11, snap.dt_max_ms);  // 0,011s -> 11ms (groesster dt_s-Wert)
  TEST_ASSERT_EQUAL_UINT16(700, snap.loop_max_us);
}

void test_empty_window_yields_all_zero() {
  TelemetryWindowAgg agg;
  const WindowAggSnapshot snap = agg.snapshotAndReset();  // kein add() seit Konstruktion
  TEST_ASSERT_EQUAL_FLOAT(0.0f, snap.norm_delta_min);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, snap.norm_delta_max);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, snap.jerk_max);
  TEST_ASSERT_EQUAL_UINT8(0, snap.regime_static_n);
  TEST_ASSERT_EQUAL_UINT8(0, snap.regime_dynamic_n);
  TEST_ASSERT_EQUAL_UINT8(0, snap.regime_shock_n);
  TEST_ASSERT_EQUAL_UINT8(0, snap.dt_max_ms);
  TEST_ASSERT_EQUAL_UINT16(0, snap.loop_max_us);
}

void test_saturation_dt_max_ms_loop_max_us_and_regime_counters() {
  TelemetryWindowAgg agg;
  // dt_s=0,3s -> 300ms, muss bei 255 saettigen (nicht ueberlaufen auf einen
  // kleinen uint8-Wert). loop_us=70000 muss bei 65535 saettigen.
  agg.add(0.300f, 0.0f, 0.0f, MotionRegime::Static, 70000u);
  const WindowAggSnapshot snap_single = agg.snapshotAndReset();
  TEST_ASSERT_EQUAL_UINT8(255, snap_single.dt_max_ms);
  TEST_ASSERT_EQUAL_UINT16(65535, snap_single.loop_max_us);

  // Regime-Zaehler: 300 STATIC-Samples in einem (unrealistisch langen, aber
  // fuer den Saettigungsnachweis zulaessigen) Fenster muessen bei 255
  // stehen bleiben, nicht auf 44 ueberlaufen (300 mod 256).
  TelemetryWindowAgg agg2;
  for (int i = 0; i < 300; ++i) {
    agg2.add(0.010f, 0.0f, 0.0f, MotionRegime::Static, 500u);
  }
  const WindowAggSnapshot snap_regime = agg2.snapshotAndReset();
  TEST_ASSERT_EQUAL_UINT8(255, snap_regime.regime_static_n);
}

void test_reset_behavior_clears_state_for_next_window() {
  TelemetryWindowAgg agg;
  agg.add(0.010f, 5.0f, 3.0f, MotionRegime::Shock, 900u);
  const WindowAggSnapshot first = agg.snapshotAndReset();
  TEST_ASSERT_EQUAL_FLOAT(5.0f, first.norm_delta_max);
  TEST_ASSERT_EQUAL_UINT8(1, first.regime_shock_n);

  // Kein weiteres add() -- der naechste Snapshot muss wieder leer sein,
  // nicht die Werte des vorigen Fensters fortschreiben.
  const WindowAggSnapshot second = agg.snapshotAndReset();
  TEST_ASSERT_EQUAL_FLOAT(0.0f, second.norm_delta_max);
  TEST_ASSERT_EQUAL_UINT8(0, second.regime_shock_n);

  // Nach dem Reset wieder normal nutzbar (kein "kaputter" Zustand).
  agg.add(0.010f, 1.0f, 1.0f, MotionRegime::Static, 400u);
  const WindowAggSnapshot third = agg.snapshotAndReset();
  TEST_ASSERT_EQUAL_FLOAT(1.0f, third.norm_delta_max);
  TEST_ASSERT_EQUAL_UINT8(1, third.regime_static_n);
}

void test_mixed_regime_sequence_counts_correctly() {
  TelemetryWindowAgg agg;
  const MotionRegime sequence[] = {
      MotionRegime::Static,  MotionRegime::Static, MotionRegime::Dynamic, MotionRegime::Shock,
      MotionRegime::Shock,   MotionRegime::Shock,  MotionRegime::Dynamic, MotionRegime::Static,
      MotionRegime::Dynamic, MotionRegime::Dynamic};
  for (MotionRegime r : sequence) {
    agg.add(0.010f, 0.0f, 0.0f, r, 500u);
  }
  const WindowAggSnapshot snap = agg.snapshotAndReset();
  TEST_ASSERT_EQUAL_UINT8(3, snap.regime_static_n);
  TEST_ASSERT_EQUAL_UINT8(4, snap.regime_dynamic_n);
  TEST_ASSERT_EQUAL_UINT8(3, snap.regime_shock_n);
  // Summe entspricht der Anzahl add()-Aufrufe -- wird NICHT auf 10 normiert
  // (hier zufaellig exakt 10, s. B-FW.1 -- keine implizite Normierung im
  // Code, nur eine reine Zaehlung).
  TEST_ASSERT_EQUAL_UINT32(
      10u, static_cast<uint32_t>(snap.regime_static_n) + snap.regime_dynamic_n + snap.regime_shock_n);
}

void test_summe_zaehler_nicht_auf_10_normiert_bei_abweichender_anzahl() {
  // Nur 7 Samples in diesem Fenster (Taktschwankung/kurzer Sensorausfall)
  // -- die Summe MUSS 7 bleiben, nicht auf 10 hochgerechnet werden.
  TelemetryWindowAgg agg;
  for (int i = 0; i < 7; ++i) {
    agg.add(0.010f, 0.0f, 0.0f, MotionRegime::Dynamic, 500u);
  }
  const WindowAggSnapshot snap = agg.snapshotAndReset();
  TEST_ASSERT_EQUAL_UINT8(7, snap.regime_dynamic_n);
  TEST_ASSERT_EQUAL_UINT32(
      7u, static_cast<uint32_t>(snap.regime_static_n) + snap.regime_dynamic_n + snap.regime_shock_n);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_normal_case_10_samples);
  RUN_TEST(test_empty_window_yields_all_zero);
  RUN_TEST(test_saturation_dt_max_ms_loop_max_us_and_regime_counters);
  RUN_TEST(test_reset_behavior_clears_state_for_next_window);
  RUN_TEST(test_mixed_regime_sequence_counts_correctly);
  RUN_TEST(test_summe_zaehler_nicht_auf_10_normiert_bei_abweichender_anzahl);
  return UNITY_END();
}
