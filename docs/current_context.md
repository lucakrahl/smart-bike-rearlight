# Current Context

> Von Claude Code eigenständig pflegbar (siehe `CLAUDE.md`).

**Stand:** Repo-Grundgerüst angelegt (Phase 3). SRS vollständig (Bible v0.7).
Build-Umgebung steht (PlatformIO mit pioarduino, Arduino-ESP32-Core 3.3.11).
R1 (`lifecycle_fsm`), R2 (`tail_light_fsm`), der IMU-Teilschritt von R4
(`imu_driver` + `motion_filter`) und R3 (`rf_input` + `button_decoder` +
`blinker_fsm`) sind in `main.cpp` verdrahtet. Baut grün auf PC
(`pio test -e native`, 38/38) und ESP32 (`pio run -e esp32dev`).

**Aktueller Fokus:** Firmware-Implementierung nach SRS fortsetzen.

**Nächster Schritt:** M5 der Roadmap — Telemetrie + BLE (R4): versioniertes
Frame (FR-TEL-02/03/06), BLE-Notify (FR-SYS-04), RAM-Ringpuffer (FR-TEL-04,
NFR-RES-01). Zusätzlich aus M3 übernommen: BMP280 + L86/GNSS-Fix-Status
(FR-TEL-05), I²C-Recovery (FR-SNS-04), Plausibilitätsprüfung (FR-SNS-05).

**Zuletzt erledigt:** M4 — RF-Empfang (`rf_input`), Tastenerkennung
(`button_decoder`: Entprellung, Halte-/Release-Erkennung, Kurz-/Langdruck)
und Blinker-FSM (`blinker_fsm`: AUS/LINKS/RECHTS/WARN, 1,5-Hz-Takt), mit
Host-Tests, in `main.cpp` verdrahtet (FR-BLK-09-Gate gegen `SystemState`,
Event genau einmal konsumiert).

**Blocker/offene Klärungen:** LED-Kanalzuordnung/Datenblatt (Bible 11.1);
RF-Release-Timeout vorläufig (FR-RF-03); aus M3 nach M5 verschoben:
I²C-Recovery (FR-SNS-04), Plausibilitätsprüfung (FR-SNS-05), BMP280,
L86/GNSS-Fix-Status (FR-TEL-05).
