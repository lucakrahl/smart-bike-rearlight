// Host-Unit-Test der RF-Tastenerkennung (NFR-TST-03). Laeuft ohne ESP32:
//   pio test -e native
#include <unity.h>
#include "button_decoder.h"
#include "config.h"

using namespace logic;

void test_short_press_left() {
  ButtonDecoder dec;
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(true, RF_CODE_LEFT, 0));
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(true, RF_CODE_LEFT, 50));  // entprellt

  const uint32_t not_yet = 50 + RF_RELEASE_TIMEOUT_MS;
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(false, 0, not_yet));

  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::SHORT_LINKS, (int)dec.update(false, 0, not_yet + 1));
}

void test_short_press_right() {
  ButtonDecoder dec;
  dec.update(true, RF_CODE_RIGHT, 0);
  dec.update(true, RF_CODE_RIGHT, 50);  // entprellt

  const uint32_t not_yet = 50 + RF_RELEASE_TIMEOUT_MS;
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(false, 0, not_yet));
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::SHORT_RECHTS, (int)dec.update(false, 0, not_yet + 1));
}

void test_unknown_code_is_ignored() {
  ButtonDecoder dec;
  constexpr uint32_t UNKNOWN_CODE = 123456;
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(true, UNKNOWN_CODE, 0));
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(true, UNKNOWN_CODE, 50));
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(true, UNKNOWN_CODE, 100));
  // auch lange danach nie ein Ereignis, da nie ein bekannter Code entprellt wurde.
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(false, 0, 1000000));
}

void test_hold_over_5s_yields_long_once_and_no_short_after() {
  ButtonDecoder dec;
  dec.update(true, RF_CODE_RIGHT, 0);
  dec.update(true, RF_CODE_RIGHT, 50);  // entprellt, press_start_ms_ = 0

  ButtonEvent event = ButtonEvent::NONE;
  uint32_t t = 50;
  // Alle 100 ms nachgesendeter Code haelt den Druck (Luecke < Release-Timeout).
  while (t < LONGPRESS_MS) {
    t += 100;
    event = dec.update(true, RF_CODE_RIGHT, t);
    if (event != ButtonEvent::NONE) break;
  }
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::LONG, (int)event);
  TEST_ASSERT_TRUE(t >= LONGPRESS_MS);

  // Weiterhin gehalten: kein zweites LONG.
  t += 100;
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(true, RF_CODE_RIGHT, t));

  // Loslassen nach LONG: kein SHORT mehr (FR-BLK-07).
  const uint32_t not_yet = t + RF_RELEASE_TIMEOUT_MS;
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(false, 0, not_yet));
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(false, 0, not_yet + 1));
}

void test_release_detected_only_after_timeout() {
  ButtonDecoder dec;
  dec.update(true, RF_CODE_LEFT, 0);
  dec.update(true, RF_CODE_LEFT, 50);  // entprellt, letzter Empfang bei t=50

  // Genau am Timeout: noch nicht losgelassen.
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE,
                        (int)dec.update(false, 0, 50 + RF_RELEASE_TIMEOUT_MS));
  // Einen Tick spaeter: losgelassen -> SHORT.
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::SHORT_LINKS,
                        (int)dec.update(false, 0, 50 + RF_RELEASE_TIMEOUT_MS + 1));
}

void test_single_code_below_debounce_yields_no_event() {
  ButtonDecoder dec;
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(true, RF_CODE_LEFT, 0));
  // Nie ein zweiter Empfang -> nie entprellt, nie ein Ereignis.
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(false, 0, 1000000));
}

void test_different_known_code_during_press_starts_new_sequence() {
  ButtonDecoder dec;
  dec.update(true, RF_CODE_LEFT, 0);
  dec.update(true, RF_CODE_LEFT, 50);  // links bestaetigt gedrueckt

  // Rechts-Code taucht mitten im Links-Druck auf: laufende Sequenz wird
  // sauber verworfen (kein Event fuer den Links-Druck), neue Sequenz beginnt.
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(true, RF_CODE_RIGHT, 100));
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(true, RF_CODE_RIGHT, 150));  // entprellt

  const uint32_t not_yet = 150 + RF_RELEASE_TIMEOUT_MS;
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::NONE, (int)dec.update(false, 0, not_yet));
  TEST_ASSERT_EQUAL_INT((int)ButtonEvent::SHORT_RECHTS, (int)dec.update(false, 0, not_yet + 1));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_short_press_left);
  RUN_TEST(test_short_press_right);
  RUN_TEST(test_unknown_code_is_ignored);
  RUN_TEST(test_hold_over_5s_yields_long_once_and_no_short_after);
  RUN_TEST(test_release_detected_only_after_timeout);
  RUN_TEST(test_single_code_below_debounce_yields_no_event);
  RUN_TEST(test_different_known_code_during_press_starts_new_sequence);
  return UNITY_END();
}
