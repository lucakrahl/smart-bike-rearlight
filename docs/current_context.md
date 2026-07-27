# Current Context

> Von Claude Code eigenständig pflegbar (siehe `CLAUDE.md`).

**Stand:** Firmware-Kernfunktion (R1–R4) implementiert und mit Host-Tests
abgesichert. SRS/Bible bei Version 0.10. Build-Umgebung: PlatformIO mit
pioarduino, Arduino-ESP32-Core 3.3.11. R1 (`lifecycle_fsm`), R2
(`tail_light_fsm`), R3 (`rf_input`/`button_decoder`/`blinker_fsm`) und R4
(`imu_driver`+`motion_filter`+`imu_health`, `bmp280_driver`,
`gnss_driver`+`gnss_fix`) sind in `main.cpp` verdrahtet. Baut grün auf PC
(`pio test -e native`, 61/61) und ESP32 (`pio run -e esp32dev`).

**Aktueller Fokus:** Firmware-Implementierung nach SRS fortsetzen.

**Nächster Schritt:** Härtung Teil 2 (Roadmap M5) — Watchdog (FR-SAF-03)
aktivieren (~2 s, `main.cpp`/`setup()`/`loop()`). Danach der Telemetrie-
+BLE-Rest von M5 (FR-TEL-02/03/06, FR-SYS-04, FR-TEL-04/NFR-RES-01).

**Zuletzt erledigt:** M5 Teil A (BMP280, FORCED-Mode) + Teil B (`gnss_driver`
+ `gnss_fix`, Fix-Status FR-TEL-05) validiert; Härtung Teil 1 — I²C-Recovery
+ Plausibilität + Fail-Safe (`imu_health`, FR-SNS-04/05, FR-STA-04) committet
(`4096d4d`), per SDA-Kurzschluss-Fehlerinjektion am Board verifiziert
(Recovery wirksam, kein Fehl-Bremslicht, Fail-Safe auf Schlusslicht).

**Blocker/offene Klärungen:** LED-Kanalzuordnung/Datenblatt (Bible 11.1);
RF-Release-Timeout vorläufig (FR-RF-03); IMU-Plausibilitäts-/Recovery-
Schwellen noch `TODO(offen)` (Feldverifikation, s. `open_issues.md`);
GNSS-Freilandtest (echter Fix) ausstehend.
