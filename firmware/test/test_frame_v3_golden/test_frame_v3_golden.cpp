// Schnittstellen-Kreuztest Firmware <-> App (NFR-TST-03, docs/
// BLE_Frame_v3_Schnittstelle.md). Laeuft ohne ESP32: pio test -e native
//
// ZWECK: test_telemetry_frame.cpp prueft nur die Symmetrie dieses
// Firmware-Encoders gegen sich selbst -- ein gemeinsamer Denkfehler (z. B.
// falscher Offset, falsche Skalierung), der sowohl hier als auch im
// unabhaengigen App-Decoder gleich falsch waere, bliebe dabei unsichtbar.
// Dieser Test serialisiert ein Frame mit fest definierten, gut
// unterscheidbaren Werten (keine Nullen, keine Wiederholungen gleichartiger
// Felder) und vergleicht das Ergebnis gegen eine EINGEFRORENE Referenz-
// Bytefolge (testdata/frame_v3_golden.hex, Wertetabelle in
// testdata/frame_v3_golden.md fuer die App-Seite). Nur diese Bytefolge
// pruefte die Schnittstelle selbst, nicht nur die eigene Symmetrie.
//
// Die Referenzdatei wird NUR angelegt, wenn sie noch fehlt (Erstlauf). Ist
// sie vorhanden, wird ausschliesslich VERGLICHEN -- ein Mismatch ist ein
// Testfehler, die Datei wird NIE automatisch nachgezogen. Aendert sich
// Layout oder Skalierung des Frames absichtlich, muss die Referenzdatei
// bewusst geloescht und neu erzeugt (und im Diff ueberprueft) werden.
#include <unity.h>
#include <cstdio>
#include <cstring>
#include <string>
#include "telemetry_frame.h"

using namespace logic;

namespace {

// Pfad relativ zum Arbeitsverzeichnis, in dem `pio test -e native` die
// Test-Binaries ausfuehrt (Firmware-Projektwurzel, s. platformio.ini) --
// testdata/ liegt eine Ebene hoeher, auf Repo-Root-Ebene.
constexpr const char* kGoldenHexPath = "../testdata/frame_v3_golden.hex";

std::string toHex(const uint8_t* buf, size_t n) {
  static const char* kDigits = "0123456789abcdef";
  std::string out;
  out.reserve(n * 2);
  for (size_t i = 0; i < n; ++i) {
    out.push_back(kDigits[buf[i] >> 4]);
    out.push_back(kDigits[buf[i] & 0x0F]);
  }
  return out;
}

// Fest definierte, gut unterscheidbare Werte -- keine Nullen (soweit vom
// Typ her moeglich; boolesche Felder sind zwangslaeufig auf 1 begrenzt),
// keine Wiederholung gleichartiger benachbarter Felder. Jedes Feld muss an
// seinem eigenen Wert erkennbar sein.
TelemetryFrame buildGoldenFrame() {
  TelemetryFrame f;
  f.timestamp_ms = 305419896u;  // 0x12345678

  f.accel_x_ms2 = 1.1f;
  f.accel_y_ms2 = 2.2f;
  f.accel_z_ms2 = 3.3f;
  f.gyro_x_rads = 4.4f;
  f.gyro_y_rads = 5.5f;
  f.gyro_z_rads = 6.6f;
  f.brake_decel_ms2 = 7.7f;

  f.pressure_pa = 101325.5f;
  f.temperature_c = 23.4f;

  f.lat = 51.2277;
  f.lon = 6.7735;
  f.speed_kmph = 25.5f;
  f.course_deg = 123.4f;
  f.altitude_m = 45.6f;
  f.sats = 11;
  f.hdop = 1.23f;
  f.utc_year = 2026;
  f.utc_month = 8;
  f.utc_day = 7;
  f.utc_hour = 15;
  f.utc_minute = 42;
  f.utc_second = 33;

  f.system_state = 1;
  f.init_degraded = true;
  f.imu_health_state = 2;
  f.baro_valid = true;
  f.gnss_fix_status = 2;
  f.watchdog_recovered = true;
  f.brake_light_pct = 88;

  f.gnss_accel_ms2 = 8.8f;
  f.gnss_accel_valid = true;
  f.pitch_rad = 0.1234f;
  f.gyro_bias_rads = 0.005678f;
  f.norm_delta_min = -1.11f;
  f.norm_delta_max = 9.99f;
  f.jerk_max = 1.357f;
  f.regime_static_n = 3;
  f.regime_dynamic_n = 6;
  f.regime_shock_n = 2;
  f.bias_calibrated = true;
  f.dt_max_ms = 13;
  f.loop_max_us = 4567;

  return f;
}

}  // namespace

void test_v3_frame_matches_frozen_cross_toolchain_reference() {
  TEST_ASSERT_EQUAL_UINT32(113u, (uint32_t)TELEMETRY_FRAME_SIZE);

  const TelemetryFrame frame = buildGoldenFrame();
  uint8_t buf[TELEMETRY_FRAME_SIZE];
  telemetryFrameSerialize(frame, buf);
  const std::string actual_hex = toHex(buf, TELEMETRY_FRAME_SIZE);

  FILE* in = std::fopen(kGoldenHexPath, "r");
  if (in == nullptr) {
    // Erstlauf: Referenzdatei existiert noch nicht -- EINMALIG anlegen.
    // Danach schlaegt dieser Zweig nie wieder zu; jeder Folgelauf vergleicht
    // nur noch (s. Dateikopf-Kommentar).
    FILE* out = std::fopen(kGoldenHexPath, "w");
    TEST_ASSERT_NOT_NULL_MESSAGE(out, "Konnte frame_v3_golden.hex nicht anlegen (Pfad/Rechte pruefen)");
    std::fprintf(out, "%s\n", actual_hex.c_str());
    std::fclose(out);
    TEST_FAIL_MESSAGE(
        "frame_v3_golden.hex existierte nicht und wurde soeben neu angelegt -- "
        "bitte Inhalt gegen frame_v3_golden.md pruefen, dann erneut testen (Test faellt jetzt bewusst, "
        "damit ein fehlendes Golden-File nicht versehentlich als 'bestanden' durchgeht).");
    return;
  }

  char line[512] = {0};
  const char* read_ok = std::fgets(line, sizeof(line), in);
  std::fclose(in);
  TEST_ASSERT_NOT_NULL_MESSAGE(read_ok, "frame_v3_golden.hex ist leer oder unlesbar");

  // Zeilenende (\n, ggf. \r\n) abschneiden.
  size_t len = std::strlen(line);
  while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
    line[--len] = '\0';
  }

  TEST_ASSERT_EQUAL_MESSAGE(
      226, (int)len, "Golden-Datei hat nicht die erwartete Laenge (113 Byte = 226 Hex-Zeichen)");
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
      line, actual_hex.c_str(),
      "Aktuelles v3-Frame weicht von der eingefrorenen Referenz ab -- Layout oder "
      "Skalierung hat sich geaendert. NICHT die Golden-Datei automatisch nachziehen: "
      "erst pruefen, ob die Aenderung beabsichtigt ist (dann frame_v3_golden.hex/.md "
      "bewusst neu erzeugen und im Diff review en), sonst ist das eine echte Regression.");
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_v3_frame_matches_frozen_cross_toolchain_reference);
  return UNITY_END();
}
