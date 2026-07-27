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

**Zurückgestellt, verschoben nach M5:** I²C-Recovery (FR-SNS-04),
Plausibilitätsprüfung (FR-SNS-05), BMP280, L86/GNSS-Fix-Status (FR-TEL-05).
Achsen-/Vorzeichenkonvention der IMU ist am realen Board kalibriert
(`MOTION_BRAKE_SIGN` in `config.h`, kein `TODO(offen)` mehr).

## M4 — Blinker + RF (R3)  ✅
`rf_input` + `button_decoder` (Entprellung FR-RF-02, Halte-/Release-Erkennung
FR-RF-03, Kurz-/Langdruck FR-RF-04, FR-BLK-07), Blinker-State-Machine
AUS/LINKS/RECHTS/WARN (FR-BLK-01..09), 1,5-Hz-Takt (FR-BLK-08). In `main.cpp`
verdrahtet (FR-BLK-09-Gate gegen `SystemState`, Event genau einmal
konsumiert). Tests der Tastenerkennung/Blinklogik.

## M5 — Sensorik-Vervollständigung + Härtung + Telemetrie/BLE (R4)  ☐
**Teil A — BMP280 (Luftdruck)  ✅**
Rohdaten-Treiber (FR-SYS-01: Höhe wird in der App berechnet), FORCED-Mode
(Weather-Monitoring-Profil ×1/×1, IIR aus — Selbsterwärmung reduziert),
zentrale I²C-Bus-Init (`Wire.begin()` einmalig in `main.cpp`/`setup()`). Am
Board validiert.

**Teil B — L86/GNSS  ✅**
`gnss_driver` (UART2, TinyGPSPlus, nicht-blockierendes `gnssPump()`) +
`gnss_fix` (Fix-Status FR-TEL-05: NO_DATA/NO_FIX/FIX_OK), Host-Tests.
UART/Parsing am Board validiert (Indoor korrekt NO_FIX; echter Fix braucht
Freilandtest, s. `open_issues.md`).

**Härtung Teil 1 — I²C-Recovery + Plausibilität + Fail-Safe  ✅**
`imu_health` (FR-SNS-04/05, FR-STA-04): Wertebereich-/Frozen-/Sprung-
Plausibilität, gestufte Recovery (Soft-Reinit, SCL-Clock-Release),
Eskalations-Vertrauen über N konsekutive plausible Zyklen (verhindert
Fehl-Bremslicht durch ein einzelnes Müll-Sample). `imu_driver`:
WHO_AM_I-Liveness-Check statt `getEvent()`, FSR ±16 g, Recovery über rohe
`gpio_*`-Calls (PeriMan-Umgehung bei hängendem I²C-Bus). Per
SDA-Kurzschluss-Fehlerinjektion am Board verifiziert (Recovery wirksam,
kein Fehl-Bremslicht, Fail-Safe fällt sicher auf Schlusslicht zurück).

**Härtung Teil 2 — Watchdog (FR-SAF-03)  ✅**
`esp_task_wdt_reconfigure()` (Fallback `esp_task_wdt_init()`) auf
`WATCHDOG_TIMEOUT_MS`, `enableLoopWDT()` registriert den `loopTask` beim
TWDT (Arduino-Core fuettert automatisch vor jedem `loop()`). Boot-Diagnose
per `esp_reset_reason()` (WDT-/Panic-Reset-Flag, hinter `DEBUG_SERIAL`,
vorgehalten für Telemetrie). Per 'H'-Hang-Hook am Board verifiziert:
Auto-Reset nach ~2 s, Reset-Grund korrekt erkannt (Commit `17d18bc`).

**Telemetrie + BLE  ☐ ← als Nächstes**
Versioniertes Frame (FR-TEL-02/03/06), BLE-Notify unidirektional (FR-SYS-04),
RAM-Ringpuffer (FR-TEL-04, NFR-RES-01).

## M6 — Konfiguration  ☐
NVS/`Preferences` + serielles Kalibrier-Interface (FR-CFG-01/02/03).

## M7 — Integration & Messungen  ☐
Gesamtsystemtest; Messungen: Reaktionszeit ≤ 50 ms, Loop-Zeit, Energie, I²C-
Recovery, Watchdog, Brown-Out (Bible Kap. 9).
