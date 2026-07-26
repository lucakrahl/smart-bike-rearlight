// button_decoder.cpp — Umsetzung RF-Tastenerkennung. Rein, hardwarefrei.
#include "button_decoder.h"

namespace logic {

ButtonDecoder::ButtonDecoder(const ButtonDecoderParams& params) : params_(params) {}

ButtonEvent ButtonDecoder::update(bool has_code, uint32_t code, uint32_t now_ms) {
  if (has_code) {
    const bool known = (code == params_.code_left || code == params_.code_right);
    if (known) {
      const bool gap_too_large = (now_ms - last_seen_ms_) > params_.release_timeout_ms;
      if (code != pending_code_ || gap_too_large) {
        // Neue Sequenz: anderer (bekannter) Code oder zu grosse Luecke seit
        // dem letzten Empfang -> laufende Sequenz sauber verwerfen, kein
        // Event fuer den verworfenen Druck.
        pending_code_ = code;
        repeat_count_ = 1;
        press_start_ms_ = now_ms;
        pressed_confirmed_ = false;
        long_fired_ = false;
      } else {
        repeat_count_++;
      }
      last_seen_ms_ = now_ms;

      if (!pressed_confirmed_ && repeat_count_ >= params_.debounce_count) {
        pressed_confirmed_ = true;  // FR-RF-02: Entprellung erreicht
      }
    }
    // Unbekannte Codes: FR-RF-01, ignoriert, keine Wirkung auf eine laufende
    // Sequenz.
  }

  if (!pressed_confirmed_) {
    return ButtonEvent::NONE;
  }

  const uint32_t held_ms = now_ms - press_start_ms_;
  if (!long_fired_ && held_ms >= params_.longpress_ms) {
    long_fired_ = true;
    return ButtonEvent::LONG;  // FR-RF-04
  }

  if (now_ms - last_seen_ms_ > params_.release_timeout_ms) {
    // Losgelassen (FR-RF-03).
    const bool was_long = long_fired_;
    const uint32_t released_code = pending_code_;
    pressed_confirmed_ = false;
    repeat_count_ = 0;
    long_fired_ = false;
    if (was_long) {
      return ButtonEvent::NONE;  // FR-BLK-07: nach LONG kein SHORT mehr
    }
    return released_code == params_.code_left ? ButtonEvent::SHORT_LINKS
                                                : ButtonEvent::SHORT_RECHTS;
  }

  return ButtonEvent::NONE;
}

}  // namespace logic
