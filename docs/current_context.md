# Current Context

> Von Claude Code eigenständig pflegbar (siehe `CLAUDE.md`).

**Stand:** Repo-Grundgerüst angelegt (Phase 3). SRS vollständig (Bible v0.7).
Build-Umgebung steht (PlatformIO mit pioarduino, Arduino-ESP32-Core 3.3.11).
R1 (`lifecycle_fsm`), R2 (`tail_light_fsm`) und der IMU-Teilschritt von R4
(`imu_driver` + `motion_filter`) sind in `main.cpp` verdrahtet
(`taskLifecycleAndTailLight`, 100 Hz). Baut grün auf PC (`pio test -e native`,
23/23) und ESP32 (`pio run -e esp32dev`, Flash 10,7 % / RAM 7,3 %).

**Aktueller Fokus:** Firmware-Implementierung nach SRS fortsetzen.

**Nächster Schritt:** M4 der Roadmap — Blinker + RF (R3): `rf_input` +
`button_decoder` (Kurz-/Langdruck FR-RF-03/04), Blinker-State-Machine
(FR-BLK-01..09), 1,5-Hz-Takt.

**Zuletzt erledigt:** M3, Teilschritt IMU — `imu_driver` (MPU6050, I2C-Timeout
FR-SNS-03) + `motion_filter` (Komplementärfilter, Gravitationskompensation,
richtungsabhängige Bremserkennung), mit Host-Tests, in `main.cpp` verdrahtet.

**Blocker/offene Klärungen:** LED-Kanalzuordnung/Datenblatt (Bible 11.1);
RF-Release-Timeout vorläufig (FR-RF-03); Achsen-/Vorzeichenkonvention der IMU
unverifiziert (`TODO(offen)` bei `MOTION_BRAKE_SIGN` in `config.h`); aus M3
zurückgestellt: I²C-Recovery (FR-SNS-04), Plausibilitätsprüfung (FR-SNS-05),
BMP280, L86/GNSS-Fix-Status (FR-TEL-05) — noch keinem Milestone zugeordnet.
