# Current Context

> Von Claude Code eigenständig pflegbar (siehe `CLAUDE.md`).

**Stand:** Firmware-Kernfunktion (R1–R4) implementiert und mit Host-Tests
abgesichert, Bremslicht-Logik (FR-TL-06/NFR-RT-01/FR-SAF-01) per Serial-Bench
am realen MCU validiert. SRS/Bible bei Version 0.15. Build-Umgebung:
PlatformIO mit pioarduino, Arduino-ESP32-Core 3.3.11. R1 (`lifecycle_fsm`),
R2 (`tail_light_fsm`), R3 (`rf_input`/`button_decoder`/`blinker_fsm`) und R4
(`imu_driver`+`motion_filter`+`imu_health`, `bmp280_driver`,
`gnss_driver`+`gnss_fix`, `ble_telemetry`) sind in `main.cpp` verdrahtet.
BLE läuft im Normalbetrieb mit (kein Feature-Flag mehr, Isolations-Scaffold
entfernt). Baut grün auf PC (`pio test -e native`, 77/77) und ESP32
(`pio run -e esp32dev`). iOS-App-MVP ist funktionsfertig und am realen
iPhone gegen die echte BLE-Verbindung verifiziert (s. iOS-Abschnitt unten).

**Aktueller Fokus:** Feldtest vorbereiten; Rest-Firmware (M6/M7) fortsetzen.

**Nächster Schritt:** Feldtest (30-km/h-Zone: App-Validierungs-Export
`brake_decel_ms2`/`brake_light_pct` @ 1 Hz + Beobachtung/Foto) als realer
Fahrkontext zur präzisen Bench-Kennlinie; GNSS-Freilandtest (echter Fix,
freie Himmelssicht, bisher nur Indoor `NO_FIX` gezeigt). Danach Rest-Firmware
(M6 Konfiguration/NVS, M7 restliche Messungen: Loop-Zykluszeit, Energie,
Brown-Out-Re-Test) fortsetzen.

**Zuletzt erledigt:** Serial-Bench-Validierung der Bremslicht-Logik
(NFR-TST-02-Testdaten-Hook, `BENCH_MODE`-Sonderbuild, Firmware `d8a4e75`,
Board Espressif ESP32-DevKitC-32E): drei Experimente (Kennlinie/Rampe,
Zeitverhalten/Sprung, Fail-Safe) bei 100 Hz aufgezeichnet und ausgewertet.
Ansprechschwelle exakt bei `decel_ms2 > 2,0`, linearer Verlauf bis Sättigung
100 % bei 5,0 (R² = 0,99984), Hysterese-Rückfall < 1,5 m/s² kombiniert mit
exakt 300 ms Mindesthaltezeit; Reaktionszeit ≤ 10 ms (NFR-RT-01-Soll ≤ 50 ms);
Fail-Safe hält bei `imu_health=FAILED` durchgängig 20 % Schlusslicht trotz
0→6,0→0-Rampeneingang. Artefakte unter `docs/Validierung/`: drei CSVs,
`measurement_log.md` (Messprotokoll), drei Diagramme (`abb_A/B/C_*.png`),
`bench_run_notes.md`. Parallel: iOS-App am realen iPhone gegen die echte
BLE-Verbindung verifiziert (Verbinden per Service-UUID, Live-Werte,
Auto-Reconnect) — MVP damit funktionsfertig. Wissensdatenbank nachgezogen:
Bible 0.14→0.15 (Kap. 9 neu 9.3, Kap. 10/11.3), `decision_log.md`
(Bench-Methodik-Entscheidung, freigegeben), `roadmap.md` (M5/M7).
Reine Logik/Firmware-Code dabei nicht angefasst (nur `BENCH_MODE`-Zweig in
`main.cpp` + neue PlatformIO-Env, s. vorheriger Eintrag); Host-Tests
unverändert 77/77.

Davor: Telemetrie-Frame um `brake_light_pct` (Offset 80,
uint8, 0..100) erweitert — die tatsächlich von `tail_light_fsm` kommandierte
Rücklicht-Duty, derselbe Wert wie an `drivers::setDutyPercent()` übergeben.
Ergänzt `brake_decel_ms2` (roher `motion_filter`-Eingang, unverändert bei
Offset 30) um den zugehörigen Ausgang, damit App/Auswertung Eingang vs.
Ausgang der Bremskennlinie vergleichen können (FR-TL-06-Validierung).
Frame 80→81 Byte, `TELEMETRY_SCHEMA_VERSION` 1→2 (FR-TEL-06). Zwei neue
Host-Tests (`test_brake_light_pct_field_round_trip_and_offset`,
`test_brake_light_pct_edge_values`); `pio test -e native` 77/77, `pio run -e
esp32dev` grün (RAM 26,7 %/87.456 B, Flash 21,4 %/672.991 B — Delta nur der
zusätzliche Byte × `RINGBUFFER_FRAMES`). `docs/project_bible.md` (0.13→0.14,
Kap. 7.2/7.6 „80-Byte" → „81-Byte", MTU-Mindestwert 83→84) und
`docs/ble_brownout_fallstudie.md` (dieselbe Zahlenkorrektur) nachgezogen.
`ios-app/SmartBikeRearLight/CLAUDE.md` (BLE-Vertrag, Zeile 17) auf 81 Byte/
Schema v2/`brake_light_pct`@80 aktualisiert — bewusst nur die
Vertragsbeschreibung; `TelemetryFrame.swift`/`TelemetryFrameDecoder.swift`
+ deren Tests unverändert gelassen (separater iOS-Task, liest weiterhin nur
die ersten 80 Byte, ignoriert das neue Feld kommentarlos).

Davor: M5 Teil C2 — BLE-Transport (`ble_telemetry`, NimBLE-
Arduino 2.5.0, FR-TEL-01, FR-SYS-04) implementiert, in `main.cpp` verdrahtet
(Frame + Ringpuffer aus Teil C1 an `taskTelemetry()` angeschlossen,
Reconnect-Backfill), committet (`bd1f3ab`). Ursprünglich Hardware-blockiert
durch einen reproduzierbaren Brownout-Bootloop bei `NimBLEDevice::init()`
auf dem Altboard; vollständige Root-Cause-Analyse (zehn systematische Tests)
in `docs/ble_brownout_fallstudie.md` dokumentiert — Root Cause =
Spannungsregler des Altboards (AZ-Delivery ESP32 NodeMCU DevKit C V2)
liefert die BLE-RF-Kalibrierungs-Transiente nicht. Nach Einlöten des
Ersatzboards (Espressif ESP32-DevKitC-32E, WROOM-32E) **vollständig am
realen System validiert:** Pin-/Logiktest ohne BLE stabil (alle Sensoren,
Schluss-/Bremslicht, Blinker/RF, Watchdog, kein Brownout); BLE-Isolations-
und Vollbetriebstest (alle Sensoren/Aktoren + BLE gleichzeitig) ohne
Brownout, `[BLE] nach init()` erreicht, Advertising läuft parallel stabil;
reale Verbindung über nRF Connect mit MTU=185 (> Mindestwert 83) und
erfolgreichem Notify-Subscribe bestätigt. Dabei zwei von der Brownout-
Ursache unabhängige Firmware-Bugs im Advertising gefunden und behoben
(Gerätename fehlte im Advertising-Paket; Paketgröße durch UUID+Name
überschritten — Name in die Scan-Response ausgelagert). Isolations-Scaffold
(`BLE_ISOLATION_TEST`) entfernt, `BLE_ENABLED`-Flag aufgelöst (BLE ist jetzt
fester Bestandteil des Normalbetriebs). `pio test -e native` 75/75, `pio run
-e esp32dev` grün (Flash 21,4 %/672.083 B, RAM 26,5 %/86.856 B von
327.680 B). Entkopplungskondensatoren (1000 µF an 3V3 UND an Vin) bleiben
trotz Wirkungslosigkeit gegen dieses konkrete Problem als robustes
Stromversorgungsdesign erhalten.
Davor: Härtung Teil 2 — Task-Watchdog + Reset-Reason-Diagnose (FR-SAF-03)
committet (`17d18bc`), per 'H'-Hang-Hook am Board verifiziert (Auto-Reset
nach ~2 s, Reset-Grund korrekt erkannt); M5 Teil A (BMP280, FORCED-Mode) +
Teil B (`gnss_driver`+`gnss_fix`, Fix-Status FR-TEL-05) validiert; Härtung
Teil 1 — I²C-Recovery + Plausibilität + Fail-Safe (`imu_health`,
FR-SNS-04/05, FR-STA-04) committet (`4096d4d`), per SDA-Kurzschluss-
Fehlerinjektion am Board verifiziert.

**Blocker/offene Klärungen:** Keine harten Blocker mehr — BLE-Transport und
iOS-BLE sind verifiziert. Offen bleiben: **Feldkalibrierung der
Bremsschwellen** (2,0/5,0/1,5 m/s² — die Bench validiert nur die Logik mit
den konfigurierten Werten, nicht deren reale Fahreignung, s. Bible Kap. 9.3);
derselbe Power-Delivery-Headroom-Mangel vermutlich auch Ursache des länger
bekannten Brown-Out unter LED-Lastspitzen (weiterhin offen, s.
`open_issues.md`) — dort noch kein Board-Tausch-Nachweis erbracht,
Brownout-Detektor bleibt regulär aktiv. Daneben: LED-Kanalzuordnung/
Datenblatt (Bible 11.1); RF-Release-Timeout vorläufig (FR-RF-03);
IMU-Plausibilitäts-/Recovery-Schwellen noch `TODO(offen)` (Feldverifikation,
s. `open_issues.md`); GNSS-Freilandtest (echter Fix) ausstehend; Feldtest
(30-Zone) ausstehend.

## iOS-App (paralleler Track)
**Stand:** Native SwiftUI-App (Deployment iOS 26), **MVP funktionsfertig und am
realen iPhone gegen die echte BLE-Verbindung verifiziert** (`BLEConnectionService`/
Core Bluetooth löst `MockTelemetrySource` als Standardquelle ab — Mock bleibt für
Simulator-Entwicklung nutzbar): Verbinden per Service-UUID, Live-Werte, Auto-
Reconnect. Implementiert & getestet: Live-Cockpit (Start = Tap / Stopp = Halten mit
Fortschrittsring), Fahrtaufzeichnung mit 1-Hz-Verdichtung, SwiftData-Persistenz im
Hintergrund-`ModelActor`, Verlauf-Liste (Wisch-Löschen), Fahrt-Detail (Statistik +
Höhen-/Geschwindigkeitsdiagramm über Distanz via Swift Charts + Routen-Polyline via
MapKit), Liquid-Glass-Chrome (nur Steuerelemente/Statuszeile), Absturz-Recovery
**AR-DATA-04** (beim App-Start: Abschließen / Verwerfen / Weiter fahren), Stale-/
GNSS-Validitätsanzeige, barometrische Höhe, Bremslicht-Validierungs-Export/-Diagramm
(Gegenstück zur Firmware-Bench, s. oben), System-/Sensorwarnungen sowie
Cockpit-Personalisierung (`DashboardLayout`-Editor). Tests grün: `SmartBikeCore`
(`swift test`) und App-Tests (SwiftData-Store, Recovery) über den Xcode-Testplan.

**Nächster Schritt (iOS):** Verlaufs-Gesamtübersicht; danach gemeinsam mit der
Firmware der Feldtest (30-Zone).