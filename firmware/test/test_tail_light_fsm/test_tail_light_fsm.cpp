// Host-Unit-Test der R2-Zustandsmaschine (NFR-TST-03). Laeuft ohne ESP32:
//   pio test -e native
#include <unity.h>
#include "tail_light_fsm.h"
#include "config.h"

using namespace logic;

void test_init_blink_alternates() {
  TailLightFsm fsm;
  auto on = fsm.update(0.0f, SystemState::Init, 0);       // Periode 500 ms, an 0..250 ms
  TEST_ASSERT_EQUAL_INT((int)TailLightState::InitBlink, (int)on.state);
  TEST_ASSERT_EQUAL_INT(INIT_BLINK_HIGH_PCT, on.duty_pct);

  auto off = fsm.update(0.0f, SystemState::Init, 300);    // 250..500 ms -> aus
  TEST_ASSERT_EQUAL_INT((int)TailLightState::InitBlink, (int)off.state);
  TEST_ASSERT_EQUAL_INT(0, off.duty_pct);
}

void test_init_to_run_enters_taillight() {
  TailLightFsm fsm;
  fsm.update(0.0f, SystemState::Init, 0);
  auto out = fsm.update(0.0f, SystemState::Run, 10);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Taillight, (int)out.state);
  TEST_ASSERT_EQUAL_INT(TAILLIGHT_DUTY_PCT, out.duty_pct);
}

void test_brakelight_entry_uses_curve() {
  TailLightFsm fsm;
  fsm.update(0.0f, SystemState::Run, 0);                  // -> Taillight
  auto out = fsm.update(3.5f, SystemState::Run, 10);      // > on_ms2 -> Brakelight
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)out.state);
  TEST_ASSERT_EQUAL_INT(60, out.duty_pct);                // (3.5-2)/(5-2)=0.5 -> 20+0.5*80
}

void test_brakelight_exit_needs_min_hold() {
  TailLightFsm fsm;
  fsm.update(0.0f, SystemState::Run, 0);
  fsm.update(3.0f, SystemState::Run, 10);                 // -> Brakelight

  auto start = fsm.update(1.0f, SystemState::Run, 20);    // < off_ms2, Timer startet
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)start.state);

  auto before = fsm.update(1.0f, SystemState::Run, 20 + BRAKE_MIN_HOLD_MS - 1);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)before.state);

  auto after = fsm.update(1.0f, SystemState::Run, 20 + BRAKE_MIN_HOLD_MS);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Taillight, (int)after.state);
}

void test_brakelight_hold_freezes_duty_until_timeout() {
  TailLightFsm fsm;
  fsm.update(0.0f, SystemState::Run, 0);
  auto peak = fsm.update(5.0f, SystemState::Run, 10);      // volle Bremsleistung -> 100 %
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)peak.state);
  TEST_ASSERT_EQUAL_INT(100, peak.duty_pct);

  auto dip = fsm.update(0.0f, SystemState::Run, 20);       // Verzoegerung faellt abrupt weg
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)dip.state);
  TEST_ASSERT_EQUAL_INT(100, dip.duty_pct);                // Duty haelt den Spitzenwert

  auto still_held = fsm.update(0.0f, SystemState::Run, 20 + BRAKE_MIN_HOLD_MS - 1);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)still_held.state);
  TEST_ASSERT_EQUAL_INT(100, still_held.duty_pct);
  TEST_ASSERT_TRUE(still_held.duty_pct > TAILLIGHT_DUTY_PCT);  // bleibt oberhalb Grundniveau

  auto after = fsm.update(0.0f, SystemState::Run, 20 + BRAKE_MIN_HOLD_MS);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Taillight, (int)after.state);
  TEST_ASSERT_EQUAL_INT(TAILLIGHT_DUTY_PCT, after.duty_pct);   // erst jetzt faellt sie
}

void test_brakelight_hold_timer_resets_on_reentry_above_off() {
  TailLightFsm fsm;
  fsm.update(0.0f, SystemState::Run, 0);
  fsm.update(3.0f, SystemState::Run, 10);                 // -> Brakelight
  fsm.update(1.0f, SystemState::Run, 20);                 // Timer startet bei 20
  fsm.update(2.0f, SystemState::Run, 150);                // >= off_ms2 -> Timer verworfen
  fsm.update(1.0f, SystemState::Run, 160);                // Timer startet neu bei 160

  // Ohne Reset waere bei 20 + BRAKE_MIN_HOLD_MS bereits Taillight erreicht.
  auto still_brakelight = fsm.update(1.0f, SystemState::Run, 20 + BRAKE_MIN_HOLD_MS);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)still_brakelight.state);

  auto after_reset_hold = fsm.update(1.0f, SystemState::Run, 160 + BRAKE_MIN_HOLD_MS);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Taillight, (int)after_reset_hold.state);
}

// Defekt B5 (Feldtest 08.08.2026): der else-Zweig in TailLightState::Brakelight
// lief bereits ab decel_ms2 >= off_ms2 (1,5) und ueberschrieb
// held_brake_duty_pct_ unbedingt mit brakeDutyPercent(decel_ms2) -- unterhalb
// on_ms2 (2,0) liefert das aber genau tail_pct (20 %). Ein monoton
// abklingender Bremsvorgang, der durch das Hystereseband (off_ms2..on_ms2)
// faellt, ueberschrieb den gehaltenen Wert dadurch VOR dem eigentlichen
// Unterschreiten von off_ms2 mit dem Schlusslichtwert -- die 300-ms-Haltezeit
// war optisch wirkungslos (14 von 14 Feldfaellen).
//
// Die drei bestehenden Haltezeit-Tests sehen den Defekt nicht:
//   test_brakelight_exit_needs_min_hold und
//   test_brakelight_hold_timer_resets_on_reentry_above_off springen direkt
//   von einem Wert oberhalb on_ms2 (3,0) auf einen Wert unterhalb off_ms2
//   (1,0/2,0) und ueberspringen damit das Hystereseband komplett.
//   test_brakelight_hold_freezes_duty_until_timeout faehrt mit 0,0 ebenfalls
//   direkt unter off_ms2. Keiner der drei Tests durchlaeuft einen
//   Zwischenwert im Band, an dem sich der Fehler zeigt.
void test_brakelight_hold_survives_declining_through_hysteresis_band() {
  TailLightFsm fsm;
  fsm.update(0.0f, SystemState::Run, 0);                    // -> Taillight

  auto peak = fsm.update(4.0f, SystemState::Run, 10);       // -> Brakelight, ueber on_ms2
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)peak.state);
  TEST_ASSERT_EQUAL_INT(73, peak.duty_pct);                 // (4-2)/(5-2)=0.667 -> 20+53.3

  auto still_above_on = fsm.update(2.5f, SystemState::Run, 20);  // weiterhin > on_ms2
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)still_above_on.state);
  TEST_ASSERT_EQUAL_INT(33, still_above_on.duty_pct);       // (2.5-2)/(5-2)=0.167 -> 20+13.3

  // Im Hystereseband (off_ms2=1,5 .. on_ms2=2,0): darf held_brake_duty_pct_
  // NICHT auf den Schlusslichtwert ueberschreiben (das war der Defekt).
  auto in_band = fsm.update(1.8f, SystemState::Run, 30);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)in_band.state);
  TEST_ASSERT_EQUAL_INT(TAILLIGHT_DUTY_PCT, in_band.duty_pct);  // Live-Kurvenwert, korrekt so

  // Unterschreiten von off_ms2 -> Haltezeit startet. Der gehaltene Wert muss
  // der letzte oberhalb on_ms2 erreichte sein (33), nicht der Zwischenwert
  // aus dem Hystereseband (20 = tail_pct, der urspruengliche Defekt).
  auto below_off = fsm.update(1.0f, SystemState::Run, 40);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)below_off.state);
  TEST_ASSERT_EQUAL_INT(33, below_off.duty_pct);
  TEST_ASSERT_TRUE(below_off.duty_pct > TAILLIGHT_DUTY_PCT);

  auto still_held = fsm.update(1.0f, SystemState::Run, 40 + BRAKE_MIN_HOLD_MS - 1);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)still_held.state);
  TEST_ASSERT_EQUAL_INT(33, still_held.duty_pct);
  TEST_ASSERT_TRUE(still_held.duty_pct > TAILLIGHT_DUTY_PCT);  // haelt durch bis zum Ablauf

  auto after = fsm.update(1.0f, SystemState::Run, 40 + BRAKE_MIN_HOLD_MS);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Taillight, (int)after.state);
  TEST_ASSERT_EQUAL_INT(TAILLIGHT_DUTY_PCT, after.duty_pct);   // erst jetzt faellt sie ab
}

void test_ess_hysteresis_on_5_off_3() {
  TailLightParams p;
  p.ess_enabled = true;
  TailLightFsm fsm(p);
  fsm.update(0.0f, SystemState::Run, 0);
  fsm.update(3.0f, SystemState::Run, 10);                 // -> Brakelight

  auto ess_on = fsm.update(ESS_ON_MS2, SystemState::Run, 20);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::EssBlink, (int)ess_on.state);

  auto ess_hold = fsm.update(4.0f, SystemState::Run, 30); // zwischen off/on -> Hysterese haelt
  TEST_ASSERT_EQUAL_INT((int)TailLightState::EssBlink, (int)ess_hold.state);

  auto ess_off = fsm.update(ESS_OFF_MS2 - 0.1f, SystemState::Run, 40);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)ess_off.state);
}

void test_ess_disabled_by_default_saturates_instead_of_blinking() {
  TailLightFsm fsm;                                       // ess_enabled = ESS_ENABLED_DEFAULT (false)
  fsm.update(0.0f, SystemState::Run, 0);
  fsm.update(3.0f, SystemState::Run, 10);                 // -> Brakelight
  auto out = fsm.update(8.0f, SystemState::Run, 20);
  TEST_ASSERT_EQUAL_INT((int)TailLightState::Brakelight, (int)out.state);
  TEST_ASSERT_EQUAL_INT(100, out.duty_pct);
}

void test_ess_blink_never_reaches_zero_percent() {
  TailLightParams p;
  p.ess_enabled = true;
  TailLightFsm fsm(p);
  fsm.update(0.0f, SystemState::Run, 0);
  fsm.update(3.0f, SystemState::Run, 10);
  fsm.update(6.0f, SystemState::Run, 20);                 // -> EssBlink

  bool saw_high = false, saw_low = false;
  for (uint32_t t = 20; t < 20 + 2000; t += 13) {
    auto out = fsm.update(6.0f, SystemState::Run, t);
    TEST_ASSERT_EQUAL_INT((int)TailLightState::EssBlink, (int)out.state);
    TEST_ASSERT_TRUE(out.duty_pct > 0);                   // nie 0 %
    if (out.duty_pct == 100) saw_high = true;
    if (out.duty_pct == TAILLIGHT_DUTY_PCT) saw_low = true;
  }
  TEST_ASSERT_TRUE(saw_high);
  TEST_ASSERT_TRUE(saw_low);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_init_blink_alternates);
  RUN_TEST(test_init_to_run_enters_taillight);
  RUN_TEST(test_brakelight_entry_uses_curve);
  RUN_TEST(test_brakelight_exit_needs_min_hold);
  RUN_TEST(test_brakelight_hold_freezes_duty_until_timeout);
  RUN_TEST(test_brakelight_hold_timer_resets_on_reentry_above_off);
  RUN_TEST(test_brakelight_hold_survives_declining_through_hysteresis_band);
  RUN_TEST(test_ess_hysteresis_on_5_off_3);
  RUN_TEST(test_ess_disabled_by_default_saturates_instead_of_blinking);
  RUN_TEST(test_ess_blink_never_reaches_zero_percent);
  return UNITY_END();
}
