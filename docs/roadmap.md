# Roadmap — Firmware-Implementierung

> Von Claude Code pflegbar (Fortschritt abhaken). Reihenfolge: sicherheits-
> kritische Kernfunktion zuerst, dann Komfort/Telemetrie. Jedes Modul: Logik
> vor Hardware, mit Host-Unit-Test.

## M0 — Grundgerüst  ✅
PlatformIO-Projekt, Scheduler-Skelett, CLAUDE.md, Docs, Beispiel-Logik + Test.

## M1 — Rücklicht-Region (R2)  ✅
`led_output` (PWM, CON-02) + Bremslicht-State-Machine (FR-TL-01/03/04/05/06),
Hysterese + Mindesthaltezeit. Notbrems-Blinken (FR-TL-07) als deaktivierbarer
Zustand. Fail-safe-Grundlicht (FR-SAF-01). Tests der Kennlinie/Hysterese.

## M2 — Lebenszyklus (R1)  ✅
INIT→RUN mit Sensor-Init-Timeout (FR-STA-01/02), Init-Diagnose-Blinken (FR-TL-03),
degradierter RUN. Watchdog (FR-SAF-03).

**R1→R2-Integration (main.cpp):** ✅ — `lifecycle_fsm` treibt `tail_light_fsm`
direkt im 100-Hz-Task; `led_output`-Treiber (PWM) gebaut. Läuft auf PC- und
ESP32-Build. `critical_sensors_ready`/`decel_ms2` sind noch Platzhalter, bis M3.

## M3 — Sensorik (R4, Erfassung) — Teilschritt IMU  ✅
`imu_driver` (MPU6050, I2C-Timeout FR-SNS-03) + `motion_filter` (Komplementär-
filter, Gravitationskompensation, richtungsabhängige Bremserkennung — Sprints
lösen kein Bremslicht aus), jeweils mit Host-Tests. In `main.cpp` verdrahtet,
ersetzt beide `TODO(M3)`-Platzhalter (`critical_sensors_ready`, `decel_ms2`).

**Zurückgestellt (eigene Folgeschritte, noch keiner Milestone-Nummer
zugeordnet):** I²C-Recovery (FR-SNS-04), Plausibilitätsprüfung (FR-SNS-05),
BMP280, L86/GNSS-Fix-Status (FR-TEL-05). Achsen-/Vorzeichenkonvention der IMU
ist als `TODO(offen)` in `config.h` markiert (unverifiziert, s. `MOTION_BRAKE_SIGN`).

## M4 — Blinker + RF (R3)  ☐ ← als Nächstes
`rf_input` + `button_decoder` (Kurz-/Langdruck FR-RF-03/04), Blinker-State-Machine
(FR-BLK-01..09), 1,5-Hz-Takt. Tests der Blinklogik/Tastenerkennung.

## M5 — Telemetrie + BLE (R4)  ☐
Versioniertes Frame (FR-TEL-02/03/06), BLE-Notify unidirektional (FR-SYS-04),
RAM-Ringpuffer (FR-TEL-04, NFR-RES-01).

## M6 — Konfiguration  ☐
NVS/`Preferences` + serielles Kalibrier-Interface (FR-CFG-01/02/03).

## M7 — Integration & Messungen  ☐
Gesamtsystemtest; Messungen: Reaktionszeit ≤ 50 ms, Loop-Zeit, Energie, I²C-
Recovery, Watchdog, Brown-Out (Bible Kap. 9).
