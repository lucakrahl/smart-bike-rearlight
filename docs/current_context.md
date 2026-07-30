# Current Context

> Von Claude Code eigenständig pflegbar (siehe `CLAUDE.md`).

**Stand:** Firmware-Kernfunktion (R1–R4) implementiert und mit Host-Tests
abgesichert. SRS/Bible bei Version 0.12. Build-Umgebung: PlatformIO mit
pioarduino, Arduino-ESP32-Core 3.3.11. R1 (`lifecycle_fsm`), R2
(`tail_light_fsm`), R3 (`rf_input`/`button_decoder`/`blinker_fsm`) und R4
(`imu_driver`+`motion_filter`+`imu_health`, `bmp280_driver`,
`gnss_driver`+`gnss_fix`, `ble_telemetry`) sind in `main.cpp` verdrahtet
(BLE aktuell per `#define BLE_ENABLED` geparkt, s. Blocker unten). Baut grün
auf PC (`pio test -e native`, 75/75) und ESP32 (`pio run -e esp32dev`).

**Aktueller Fokus:** Firmware-Implementierung nach SRS fortsetzen.

**Nächster Schritt:** Nach Eintreffen des neuen Boards (Espressif
ESP32-DevKitC-32E, WROOM-32E): Bauform/Pin-Zahl (38) gegen das Altboard
prüfen, Board tauschen, BLE verifizieren (nRF Connect: Verbindung,
Advertising, MTU-Verhandlung, Reconnect-Backfill), danach M5 Teil C2
committen. Parallel: iOS-App-Entwicklung gegen die bereits fixierte
Frame-Spezifikation (80-Byte-Frame, GATT-Service) starten (client-agnostisch,
unabhängig vom Board-Tausch möglich); außerdem Rest-Firmware (M6
Konfiguration/NVS, M7 Integration/Messungen) fortsetzen.

**Zuletzt erledigt:** M5 Teil C2 — BLE-Transport (`ble_telemetry`, NimBLE-
Arduino 2.5.0, FR-TEL-01, FR-SYS-04) implementiert, in `main.cpp` verdrahtet
(Frame + Ringpuffer aus Teil C1 an `taskTelemetry()` angeschlossen,
Reconnect-Backfill), `pio test -e native` grün (75/75), `pio run -e
esp32dev` grün. **Hardware-Verifikation blockiert:** reproduzierbarer
Brownout-Bootloop bei `NimBLEDevice::init()`. Vollständige Root-Cause-Analyse
(zehn systematische Tests) durchgeführt und in `docs/ble_brownout_fallstudie.md`
dokumentiert — Firmware als Ursache ausgeschlossen (host-getestet, Fehler auf
`init()` lokalisiert); Root Cause = Spannungsregler des Altboards (AZ-Delivery
ESP32 NodeMCU DevKit C V2) liefert die BLE-RF-Kalibrierungs-Transiente nicht.
Entscheidung: Board-Tausch auf Espressif ESP32-DevKitC-32E (WROOM-32E,
pin-kompatibel, **bestellt**, Eintreffen ausstehend); Entkopplungskondensatoren
(1000 µF an 3V3 UND an Vin, bereits verbaut) bleiben trotz Wirkungslosigkeit
gegen dieses Problem als robustes Stromversorgungsdesign erhalten. Firmware
M5 Teil C2 bleibt inhaltlich unverändert, Commit steht noch aus (BLE am
realen Gerät bisher nicht verifizierbar). Size-Report (NFR-RES-03, inkl.
48-KB-Telemetrie-Ringpuffer + NimBLE): Flash 21,4 % (672.083 / 3.145.728 B),
RAM 26,5 % (86.856 / 327.680 B) — nicht direkt mit dem alten "10,3 %/7,3 %"-
Wert der Bible vergleichbar (sehr frühe Projektphase, vor M1–M5).
Davor: Härtung Teil 2 — Task-Watchdog + Reset-Reason-Diagnose (FR-SAF-03)
committet (`17d18bc`), per 'H'-Hang-Hook am Board verifiziert (Auto-Reset
nach ~2 s, Reset-Grund korrekt erkannt); M5 Teil A (BMP280, FORCED-Mode) +
Teil B (`gnss_driver`+`gnss_fix`, Fix-Status FR-TEL-05) validiert; Härtung
Teil 1 — I²C-Recovery + Plausibilität + Fail-Safe (`imu_health`,
FR-SNS-04/05, FR-STA-04) committet (`4096d4d`), per SDA-Kurzschluss-
Fehlerinjektion am Board verifiziert.

**Blocker/offene Klärungen:** **BLE-Verifikation (kritisch):** hängt am
Eintreffen des neuen Boards (Espressif ESP32-DevKitC-32E), s. oben und
`docs/ble_brownout_fallstudie.md`; MTU-<83-Grenzfall am realen Client bleibt
zusätzlich offen (noch keine Verbindung zustande gekommen). Derselbe
Power-Delivery-Headroom-Mangel vermutlich auch Ursache des länger bekannten
Brown-Out unter LED-Lastspitzen (jetzt als „realisiert" statt „Risiko"
geführt, s. Bible Kap. 12). Brownout-Detektor bleibt aktiv. Daneben:
LED-Kanalzuordnung/Datenblatt (Bible 11.1); RF-Release-Timeout vorläufig
(FR-RF-03); IMU-Plausibilitäts-/Recovery-Schwellen noch `TODO(offen)`
(Feldverifikation, s. `open_issues.md`); GNSS-Freilandtest (echter Fix)
ausstehend.
