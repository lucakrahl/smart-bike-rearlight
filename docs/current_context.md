# Current Context

> Von Claude Code eigenständig pflegbar (siehe `CLAUDE.md`).

**Stand:** Repo-Grundgerüst angelegt (Phase 3). SRS vollständig (Bible v0.7).
Build-Umgebung steht (PlatformIO mit pioarduino, Arduino-ESP32-Core 3.3.11).
R1 (`lifecycle_fsm`) und R2 (`tail_light_fsm`) sind in `main.cpp` verdrahtet
(`taskLifecycleAndTailLight`, 100 Hz); LED-Treiber `led_output` (PWM, CON-02)
gebaut. Baut grün auf PC (`pio test -e native`, 19/19) und ESP32
(`pio run -e esp32dev`, Flash 10,3 % / RAM 7,3 %).

**Aktueller Fokus:** Firmware-Implementierung nach SRS fortsetzen.

**Nächster Schritt:** M3 der Roadmap — Sensorik (R4): `sensors`-Treiber
(MPU6050 Komplementärfilter, BMP280, L86), Plausibilitätsprüfung
(FR-SNS-03/04/05). Damit werden die beiden Platzhalter in `main.cpp`
(`critical_sensors_ready`, `decel_ms2`) durch echte Sensordaten ersetzt.

**Zuletzt erledigt:** M1 Rücklicht-Region und M2 Lebenszyklus-Region (jeweils
mit Host-Tests) sowie deren Integration in `main.cpp` inkl. neuem
`led_output`-PWM-Treiber (erste Lichtausgabe auf echter Hardware).

**Blocker/offene Klärungen:** LED-Kanalzuordnung/Datenblatt (Bible 11.1);
RF-Release-Timeout vorläufig (FR-RF-03).
