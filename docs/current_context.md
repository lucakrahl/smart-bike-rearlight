# Current Context

> Von Claude Code eigenständig pflegbar (siehe `CLAUDE.md`).

**Stand:** Repo-Grundgerüst angelegt (Phase 3). SRS vollständig (Bible v0.7).
Build-Umgebung steht (PlatformIO mit pioarduino, Arduino-ESP32-Core 3.3.11).
R2 Rücklicht (`tail_light_fsm`, M1) und R1 Lebenszyklus (`lifecycle_fsm`, M2)
als reine Logik implementiert; 19/19 Host-Tests grün (`pio test -e native`).

**Aktueller Fokus:** Firmware-Implementierung nach SRS fortsetzen.

**Nächster Schritt:** Integration R1→R2 in `main.cpp` (Lebenszyklus-Ausgabe
`SystemState` an `tail_light_fsm` durchreichen); danach M3 der Roadmap —
Sensorik (R4): `sensors`-Treiber (MPU6050/BMP280/L86), Plausibilitätsprüfung
(FR-SNS-03/04/05).

**Zuletzt erledigt:** M1 Rücklicht-Region (Bremskennlinie FR-TL-06, Hysterese
+ Mindesthaltezeit mit Duty-Freeze, ESS-Blinken FR-TL-07 experimentell) und
M2 Lebenszyklus-Region (INIT→RUN, Init-Timeout, degradierter RUN, FR-STA-01/02/06),
jeweils mit Host-Tests.

**Blocker/offene Klärungen:** LED-Kanalzuordnung/Datenblatt (Bible 11.1);
RF-Release-Timeout vorläufig (FR-RF-03).
