// gnss_speed_ref.h — GNSS-Referenzbeschleunigung fuer das v3-Telemetrie-
// Frame (E1, s. docs/BLE_Frame_v3_Schnittstelle.md Kap. 1a/6). REINE LOGIK,
// hardwarefrei (NFR-TST-01): kein #include <Arduino.h>. Host-testbar (siehe
// firmware/test/test_gnss_speed_ref/).
//
// Leitet aus zwei AUFEINANDERFOLGENDEN gueltigen GNSS-Fixes eine grobe
// Referenzbeschleunigung ab (Stufe 1 der Auswertung; die L86-Umkonfiguration
// auf 5 Hz/115200 Bd fuer eine feinere Ableitung ist Stufe 2 und bleibt
// gesperrt, s. Vertrag Kap. 6). Arbeitet auf den vorhandenen 1-Hz-Fixes.
//
// Guelligkeitskriterien (bewusst schaerfer als das bestehende FR-TEL-05-
// Gating in gnss_fix.h, s. Begruendung bei GNSS_SPEED_REF_* in config.h):
// mindestens zwei UNMITTELBAR AUFEINANDERFOLGENDE Fixes (kein Fix
// dazwischen, der die Kriterien verfehlt hat), sats>=GNSS_SPEED_REF_MIN_SATS,
// hdop<=GNSS_SPEED_REF_MAX_HDOP, Geschwindigkeit>GNSS_SPEED_REF_MIN_SPEED_MPS
// -- jeweils fuer BEIDE beteiligten Samples (ein einzelner Ausreisser bricht
// die Kette, s. .cpp).
//
// WICHTIG (E5): Dieses Modul BEOBACHTET nur. Sein Ausgang darf an keiner
// Stelle in motion_filter, tail_light_fsm oder die Bremslogik zurueckwirken
// -- main.cpp darf den Ausgang ausschliesslich ins Telemetrie-Frame
// schreiben.
#pragma once
#include <cstdint>

namespace logic {

struct GnssSpeedRefInput {
  float speed_mps;      // aktuelle GNSS-Geschwindigkeit (m/s)
  uint8_t sats;
  float hdop;
  bool location_valid;  // TinyGPSPlus location.isValid() bzw. gleichwertig
  float dt_s;            // real gemessene Zeit seit dem letzten Fix
};

struct GnssSpeedRefOutput {
  float accel_ms2 = 0.0f;  // m/s^2, positiv = Verzoegerung; 0.0f wenn !valid
  bool valid = false;
};

class GnssSpeedRef {
 public:
  // Pro eingetroffenem GNSS-Fix genau einmal aufrufen (nominell 1 Hz).
  GnssSpeedRefOutput update(const GnssSpeedRefInput& in);

  // Verwirft den gemerkten letzten Fix (z. B. bei laengerem Fix-Verlust) --
  // erzwingt, dass wieder zwei frische aufeinanderfolgende Fixes noetig
  // sind, bevor erneut eine Beschleunigung ausgegeben wird.
  void reset();

 private:
  bool has_prev_ = false;
  float prev_speed_mps_ = 0.0f;
};

}  // namespace logic
