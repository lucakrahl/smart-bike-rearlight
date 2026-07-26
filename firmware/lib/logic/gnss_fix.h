// gnss_fix.h — GNSS-Fix-Bewertung (FR-TEL-05). REINE LOGIK, hardwarefrei
// (NFR-TST-01): kein #include <Arduino.h>. Nimmt den geparsten TinyGPSPlus-
// Stand als einfache Werte entgegen (kein Zugriff auf gnss_driver/Serial2).
// Host-testbar (siehe firmware/test/test_gnss_fix/).
#pragma once
#include <cstdint>
#include "config.h"

namespace logic {

enum class GnssFixStatus { NO_DATA, NO_FIX, FIX_OK };

// chars_processed: Gesamtzahl von TinyGPSPlus verarbeiteter Zeichen seit
// gnssBegin() — < GNSS_MIN_CHARS_PROCESSED heisst "noch nie sinnvolle NMEA-
// Daten gesehen" (Verkabelung/Baudrate-Verdacht), nicht "aktuell kein Fix".
GnssFixStatus gnssFixStatus(bool location_valid, uint32_t age_ms, uint8_t sats,
                             uint32_t chars_processed);

}  // namespace logic
