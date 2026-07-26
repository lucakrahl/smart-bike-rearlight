// button_decoder.h — RF-Tastenerkennung (Sub-FSM vor R3, FR-RF-01/02/03/04)
// REINE LOGIK, hardwarefrei (NFR-TST-01): kein #include <Arduino.h>.
// Nimmt das rohe RF-Signal (Code + Vorhanden-Flag) und die Zeit als
// Eingabewerte entgegen (kein millis()); kennt nicht den RF-Treiber.
// Host-testbar (siehe firmware/test/test_button_decoder/).
//
// Ablauf (Bible Kap. 6.6, Sub-FSM):
//   IDLE --Code (>=2x, Entprellung FR-RF-02)--> GEDRUECKT
//   GEDRUECKT --Luecke > Release-Timeout (FR-RF-03)--> Losgelassen
//     -> Haltezeit < LONGPRESS_MS: SHORT-Event
//   GEDRUECKT --gehalten >= LONGPRESS_MS--> LONG-Event, wartet weiter auf
//     Loslassen (FR-BLK-07: danach kein SHORT mehr fuer diesen Druck).
//   Waehrend GEDRUECKT: taucht ein ANDERER bekannter Code auf, wird die
//   laufende Sequenz verworfen (kein Event) und sauber neu begonnen.
#pragma once
#include <cstdint>
#include "config.h"

namespace logic {

enum class ButtonEvent { NONE, SHORT_LINKS, SHORT_RECHTS, LONG };

struct ButtonDecoderParams {
  uint32_t code_left          = RF_CODE_LEFT;          // FR-RF-01
  uint32_t code_right         = RF_CODE_RIGHT;         // FR-RF-01
  uint8_t  debounce_count     = 2;                      // FR-RF-02
  uint32_t release_timeout_ms = RF_RELEASE_TIMEOUT_MS;  // FR-RF-03 (vorlaeufig)
  uint32_t longpress_ms       = LONGPRESS_MS;           // FR-RF-04, FR-BLK-07
};

class ButtonDecoder {
 public:
  explicit ButtonDecoder(const ButtonDecoderParams& params = ButtonDecoderParams());

  // has_code/code: rohes RF-Signal dieses Ticks. now_ms: monotone Zeitbasis.
  ButtonEvent update(bool has_code, uint32_t code, uint32_t now_ms);

 private:
  ButtonDecoderParams params_;

  bool     pressed_confirmed_ = false;  // Entprellung (FR-RF-02) erreicht
  uint32_t pending_code_      = 0;      // Code der aktuellen Sequenz
  uint8_t  repeat_count_      = 0;      // Empfaenge der aktuellen Sequenz
  uint32_t last_seen_ms_      = 0;      // fuer die Release-Luecke (FR-RF-03)
  uint32_t press_start_ms_    = 0;      // Ankerzeit fuer die Haltezeit (FR-RF-04)
  bool     long_fired_        = false;  // LONG hoechstens einmal pro Druck
};

}  // namespace logic
