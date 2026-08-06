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

## M5 — Sensorik-Vervollständigung + Härtung + Telemetrie/BLE (R4)  ✅
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

**Telemetrie + BLE  ✅**
Versioniertes Frame (FR-TEL-02/03/06, inkl. `brake_light_pct` seit Schema v2),
BLE-Notify unidirektional (FR-SYS-04), RAM-Ringpuffer (FR-TEL-04, NFR-RES-01).
Am realen System validiert (Espressif ESP32-DevKitC-32E, Board-Tausch behebt
den zuvor beobachteten Brownout, s. `docs/ble_brownout_fallstudie.md`):
Advertising, Verbindung, MTU=185, Volllastbetrieb stabil (Commit `e007cc3`).

## M6 — Konfiguration  ☐
NVS/`Preferences` + serielles Kalibrier-Interface (FR-CFG-01/02/03).

## M7 — Integration & Messungen  ☐
Gesamtsystemtest; Messungen (Bible Kap. 9):
- Reaktionszeit ≤ 50 ms (NFR-RT-01)  ✅ Serial-Bench validiert (≤ 10 ms
  gemessen, s. `docs/Validierung/`)
- Bremskennlinie (FR-TL-06), Logik/Zeitverhalten  ✅ Serial-Bench validiert
  (Ansprechschwelle 2,0, Sättigung 5,0, Hysterese + 300-ms-Haltezeit)
- Fail-Safe bei IMU-Ausfall (FR-SAF-01/FR-STA-04)  ✅ Serial-Bench validiert
- I²C-Recovery  ✅ (Härtung Teil 1) · Watchdog  ✅ (Härtung Teil 2)
- Loop-Zykluszeit & RAM-/CPU-Budget  ☐ offen — Messung
- Energiebilanz/Laufzeit unter realen Lastfällen  ☐ offen — Messung
- Brown-Out unter realer LED-Lastspitze  ☐ offen (Re-Test auf neuem Board)
- Feldkalibrierung der Bremsschwellen (reale Fahrbedingungen)  ☐ offen —
  Bench validiert nur die Logik mit den konfigurierten Schwellen, s. Kap. 9.3
- GNSS-Freiland-Fix  ☐ offen (bisher nur Indoor `NO_FIX`)
- Feldtest (30-Zone, App-Export + Beobachtung)  ☐ offen

---

# Roadmap — iOS-Begleit-App (paralleler Track)

> Eigene AR-IDs (App Bible). Reine Logik in `SmartBikeCore` (host-getestet,
> `swift test`). App-Target gegen simulierte Quelle, bis realer BLE-Transport steht.

- Core (Frame-Decoder, `StatisticsEngine`, `MetricRegistry`, `DashboardLayout`) + Unit-Tests  ✅
- Live-Cockpit gegen simulierte Quelle (Start/Stopp, Live-Kennzahlen, Stale-Handling)  ✅
- Fahrtaufzeichnung (1-Hz-Verdichtung) + SwiftData-Persistenz (`ModelActor`) + Verlauf-Liste  ✅
- Fahrt-Detail: Statistik-Kacheln, Höhen-/Geschwindigkeitsprofil (Swift Charts), Route (MapKit)  ✅
- Liquid Glass (iOS 26) nur für Chrome/Steuerelemente, Inhalte glasfrei  ✅
- Absturz-Recovery **AR-DATA-04** (abschließen / verwerfen / weiter fahren)  ✅
- Echter BLE-Transport `BLEConnectionService` (CoreBluetooth) statt Mock, am
  realen iPhone verifiziert  ✅
- Cockpit-Personalisierung (`DashboardLayout`-Editor)  ✅
- Verlaufs-Gesamtübersicht  ☐
