// gnss_driver.h — Quectel L86 GNSS-Treiber (UART2, TinyGPSPlus, FR-SNS-01/02)
// HARDWARE-TREIBER: kapselt nur Serial2/TinyGPSPlus, enthaelt keine
// Fix-Bewertung (die liegt in firmware/lib/logic/gnss_fix.h).
// Optionaler Sensor (FR-STA-05): Ausfall/kein Fix beeinflusst Licht/Blinker
// nicht — main.cpp verwendet gnssRead() nur fuer Telemetrie/Debug.
#pragma once
#include <cstdint>

namespace drivers {

struct GnssData {
  double lat, lon;
  float speed_kmph, course_deg, altitude_m, hdop;
  uint8_t sats;
  uint16_t year;
  uint8_t month, day, hour, minute, second;
  bool location_valid;
  uint32_t location_age_ms;
  uint32_t chars_processed;
};

// Serial2.begin(GNSS_BAUD, SERIAL_8N1, PIN_GNSS_RX, PIN_GNSS_TX). Setzt den
// RX-Puffer (GNSS_UART_RX_BUF) VOR begin() (Overflow-Reserve). L86 sendet
// NMEA-Default 1 Hz; keine PMTK-Konfiguration im MVP noetig. Blockierend nur
// insofern Serial2.begin() selbst kurz blockiert; nur fuer setup() gedacht.
bool gnssBegin();

// Draint ALLE aktuell im UART-Puffer verfuegbaren Bytes in TinyGPSPlus.
// Nicht-blockierend (nur Serial2.available()/read(), kein Warten). Muss
// haeufig aufgerufen werden (jeder loop()-Durchlauf), damit der RX-Puffer
// bei 9600 Bd nicht ueberlaeuft (s. main.cpp).
void gnssPump();

// Aktueller geparster Stand von TinyGPSPlus — kein Warten auf neue Daten.
GnssData gnssRead();

}  // namespace drivers
