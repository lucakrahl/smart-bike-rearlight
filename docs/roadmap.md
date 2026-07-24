# Roadmap — Firmware-Implementierung

> Von Claude Code pflegbar (Fortschritt abhaken). Reihenfolge: sicherheits-
> kritische Kernfunktion zuerst, dann Komfort/Telemetrie. Jedes Modul: Logik
> vor Hardware, mit Host-Unit-Test.

## M0 — Grundgerüst  ✅
PlatformIO-Projekt, Scheduler-Skelett, CLAUDE.md, Docs, Beispiel-Logik + Test.

## M1 — Rücklicht-Region (R2)  ☐
`led_output` (PWM, CON-02) + Bremslicht-State-Machine (FR-TL-01/03/04/05/06),
Hysterese + Mindesthaltezeit. Notbrems-Blinken (FR-TL-07) als deaktivierbarer
Zustand. Fail-safe-Grundlicht (FR-SAF-01). Tests der Kennlinie/Hysterese.

## M2 — Lebenszyklus (R1)  ☐
INIT→RUN mit Sensor-Init-Timeout (FR-STA-01/02), Init-Diagnose-Blinken (FR-TL-03),
degradierter RUN. Watchdog (FR-SAF-03).

## M3 — Sensorik (R4, Erfassung)  ☐
`sensors`: MPU6050 (Komplementärfilter), BMP280, L86. I²C-Timeout/Recovery
(FR-SNS-03/04), Plausibilitätsprüfung (FR-SNS-05), GNSS-Fix-Status (FR-TEL-05).

## M4 — Blinker + RF (R3)  ☐
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
