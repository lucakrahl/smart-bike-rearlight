// Host-Unit-Test der R3-Zustandsmaschine (NFR-TST-03). Laeuft ohne ESP32:
//   pio test -e native
//
// Hinweis zu Zeitwerten: der AUS-Zustand ist immer phasenunabhaengig
// (left_on/right_on == false). Wo ein Zustand indirekt bewiesen werden soll
// (z. B. "ist der Blinker nach dem Timeout wirklich aus?"), wird deshalb ein
// nachfolgendes Ereignis geschickt und dessen Wirkung geprueft, statt sich
// auf die Blinkphase an einem beliebigen Zeitpunkt zu verlassen.
#include <unity.h>
#include "blinker_fsm.h"
#include "config.h"

using namespace logic;

void test_toggle_links_on_off() {
  BlinkerFsm fsm;
  auto on = fsm.update(ButtonEvent::SHORT_LINKS, 0);
  TEST_ASSERT_TRUE(on.left_on);  // t=0 liegt in der "an"-Phase
  TEST_ASSERT_FALSE(on.right_on);

  auto off = fsm.update(ButtonEvent::SHORT_LINKS, 10);
  TEST_ASSERT_FALSE(off.left_on);
  TEST_ASSERT_FALSE(off.right_on);
}

void test_toggle_rechts_on_off() {
  BlinkerFsm fsm;
  auto on = fsm.update(ButtonEvent::SHORT_RECHTS, 0);
  TEST_ASSERT_TRUE(on.right_on);
  TEST_ASSERT_FALSE(on.left_on);

  auto off = fsm.update(ButtonEvent::SHORT_RECHTS, 10);
  TEST_ASSERT_FALSE(off.left_on);
  TEST_ASSERT_FALSE(off.right_on);
}

void test_switch_side_resets_timeout() {
  BlinkerFsm fsm;
  fsm.update(ButtonEvent::SHORT_LINKS, 0);      // Links an, Timer-Anker t=0
  fsm.update(ButtonEvent::SHORT_RECHTS, 1000);   // Wechsel -> Rechts, Timer-Anker neu bei 1000

  // Ohne Reset waere der alte Anker (t=0) bei t=BLINKER_TIMEOUT_MS bereits
  // abgelaufen. Nachweis, dass der Zustand trotzdem noch RECHTS ist: ein
  // Rechts-Druck zu diesem Zeitpunkt muss togglen (-> AUS), nicht neu auf
  // RECHTS schalten (was bei bereits automatisch abgeschaltetem AUS der Fall
  // waere).
  auto toggled = fsm.update(ButtonEvent::SHORT_RECHTS, BLINKER_TIMEOUT_MS);
  TEST_ASSERT_FALSE(toggled.left_on);
  TEST_ASSERT_FALSE(toggled.right_on);
}

void test_timeout_after_60s_switches_off() {
  BlinkerFsm fsm;
  fsm.update(ButtonEvent::SHORT_LINKS, 0);  // Links an, Timer-Anker t=0

  // Kurz vor der Marke: Zustand noch LINKS -> Toggle-Druck schaltet ab.
  auto still_links = fsm.update(ButtonEvent::SHORT_LINKS, BLINKER_TIMEOUT_MS - 1);
  TEST_ASSERT_FALSE(still_links.left_on);
  TEST_ASSERT_FALSE(still_links.right_on);

  // Neuer Fall: Marke exakt erreicht, ohne Tastendruck -> automatisch AUS.
  BlinkerFsm fsm2;
  fsm2.update(ButtonEvent::SHORT_LINKS, 0);
  auto timed_out = fsm2.update(ButtonEvent::NONE, BLINKER_TIMEOUT_MS);
  TEST_ASSERT_FALSE(timed_out.left_on);
  TEST_ASSERT_FALSE(timed_out.right_on);

  // Beweis, dass wirklich automatisch AUS erreicht wurde (nicht nur eine
  // "aus"-Blinkphase): ein Kurzdruck links muss jetzt NEU anschalten (waere
  // der Zustand noch LINKS, wuerde derselbe Druck stattdessen abschalten).
  auto reactivated = fsm2.update(ButtonEvent::SHORT_LINKS, 90 * 667);  // Blinkphase=0 -> "an"
  TEST_ASSERT_TRUE(reactivated.left_on);
}

void test_long_press_enters_warn_from_aus_and_from_direction() {
  BlinkerFsm fsm;
  auto warn = fsm.update(ButtonEvent::LONG, 0);
  TEST_ASSERT_TRUE(warn.left_on);
  TEST_ASSERT_TRUE(warn.right_on);  // FR-BLK-06: beidseitig

  BlinkerFsm fsm2;
  fsm2.update(ButtonEvent::SHORT_LINKS, 0);      // Links an
  auto warn2 = fsm2.update(ButtonEvent::LONG, 10);
  TEST_ASSERT_TRUE(warn2.left_on);
  TEST_ASSERT_TRUE(warn2.right_on);
}

void test_warn_ends_via_short_press_without_direction_blinker() {
  BlinkerFsm fsm;
  fsm.update(ButtonEvent::LONG, 0);  // -> WARN

  auto ended = fsm.update(ButtonEvent::SHORT_LINKS, 5000);  // beliebiger kurzer Druck beendet WARN
  TEST_ASSERT_FALSE(ended.left_on);
  TEST_ASSERT_FALSE(ended.right_on);

  // Kein Richtungsblinker danach (FR-BLK-05): bleibt AUS.
  auto after = fsm.update(ButtonEvent::NONE, 5010);
  TEST_ASSERT_FALSE(after.left_on);
  TEST_ASSERT_FALSE(after.right_on);
}

void test_warn_has_no_timeout() {
  BlinkerFsm fsm;
  fsm.update(ButtonEvent::LONG, 0);  // -> WARN
  fsm.update(ButtonEvent::NONE, BLINKER_TIMEOUT_MS * 10);  // langes Warten, kein Event

  // Nachweis, dass weiterhin WARN aktiv ist: ein kurzer Druck muss jetzt nach
  // AUS fuehren (FR-BLK-05). Waere WARN laengst (fehlerhaft) beendet und der
  // Zustand z. B. wieder AUS, wuerde derselbe Druck stattdessen einen
  // Richtungsblinker starten statt abzuschalten.
  auto after_short = fsm.update(ButtonEvent::SHORT_LINKS, BLINKER_TIMEOUT_MS * 10 + 10);
  TEST_ASSERT_FALSE(after_short.left_on);
  TEST_ASSERT_FALSE(after_short.right_on);
}

void test_blink_on_off_phases() {
  BlinkerFsm fsm;
  fsm.update(ButtonEvent::SHORT_LINKS, 0);
  // Periode bei 1,5 Hz: round(1000/1.5) = 667 ms, 50 % Duty -> an 0..333 ms.
  auto phase_on = fsm.update(ButtonEvent::NONE, 100);
  TEST_ASSERT_TRUE(phase_on.left_on);
  auto phase_off = fsm.update(ButtonEvent::NONE, 400);
  TEST_ASSERT_FALSE(phase_off.left_on);
  auto phase_on_again = fsm.update(ButtonEvent::NONE, 667 + 100);
  TEST_ASSERT_TRUE(phase_on_again.left_on);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_toggle_links_on_off);
  RUN_TEST(test_toggle_rechts_on_off);
  RUN_TEST(test_switch_side_resets_timeout);
  RUN_TEST(test_timeout_after_60s_switches_off);
  RUN_TEST(test_long_press_enters_warn_from_aus_and_from_direction);
  RUN_TEST(test_warn_ends_via_short_press_without_direction_blinker);
  RUN_TEST(test_warn_has_no_timeout);
  RUN_TEST(test_blink_on_off_phases);
  return UNITY_END();
}
