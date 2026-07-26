// gnss_fix.cpp — Umsetzung GNSS-Fix-Bewertung. Rein, hardwarefrei.
#include "gnss_fix.h"

namespace logic {

GnssFixStatus gnssFixStatus(bool location_valid, uint32_t age_ms, uint8_t sats,
                             uint32_t chars_processed) {
  if (chars_processed < GNSS_MIN_CHARS_PROCESSED) {
    return GnssFixStatus::NO_DATA;
  }
  if (location_valid && age_ms < GNSS_MAX_AGE_MS && sats >= GNSS_MIN_SATS) {
    return GnssFixStatus::FIX_OK;
  }
  return GnssFixStatus::NO_FIX;
}

}  // namespace logic
