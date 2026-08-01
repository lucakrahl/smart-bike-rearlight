# Current Context

> Von Claude Code eigenständig pflegbar (siehe `CLAUDE.md`).

**Stand:** Firmware-Kernfunktion (R1–R4) implementiert und mit Host-Tests
abgesichert. SRS/Bible bei Version 0.12. Build-Umgebung: PlatformIO mit
pioarduino, Arduino-ESP32-Core 3.3.11. R1 (`lifecycle_fsm`), R2
(`tail_light_fsm`), R3 (`rf_input`/`button_decoder`/`blinker_fsm`) und R4
(`imu_driver`+`motion_filter`+`imu_health`, `bmp280_driver`,
`gnss_driver`+`gnss_fix`, `ble_telemetry`) sind in `main.cpp` verdrahtet.
BLE läuft im Normalbetrieb mit (kein Feature-Flag mehr, Isolations-Scaffold
entfernt). Baut grün auf PC (`pio test -e native`, 75/75) und ESP32
(`pio run -e esp32dev`).

**Aktueller Fokus:** Firmware-Implementierung nach SRS fortsetzen.

**Nächster Schritt:** GNSS-Freilandtest (echter Fix, freie Himmelssicht,
bisher nur Indoor `NO_FIX` gezeigt). Parallel: iOS-App gegen die reale
BLE-Verbindung (CoreBluetooth statt `MockTelemetrySource`) verifizieren;
danach Rest-Firmware (M6 Konfiguration/NVS, M7 Integration/Messungen)
fortsetzen.

**Zuletzt erledigt:** M5 Teil C2 — BLE-Transport (`ble_telemetry`, NimBLE-
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

**Blocker/offene Klärungen:** BLE-Transport ist nicht mehr blockiert (s.
oben). Derselbe Power-Delivery-Headroom-Mangel vermutlich auch Ursache des
länger bekannten Brown-Out unter LED-Lastspitzen (weiterhin offen, s.
`open_issues.md`) — dort noch kein Board-Tausch-Nachweis erbracht.
Brownout-Detektor bleibt regulär aktiv. Daneben: LED-Kanalzuordnung/
Datenblatt (Bible 11.1); RF-Release-Timeout vorläufig (FR-RF-03);
IMU-Plausibilitäts-/Recovery-Schwellen noch `TODO(offen)` (Feldverifikation,
s. `open_issues.md`); GNSS-Freilandtest (echter Fix) ausstehend; iOS-App
noch nicht gegen reale BLE-Verbindung getestet.

## iOS-App (paralleler Track)
**Stand:** Native SwiftUI-App (Deployment iOS 26) läuft im Simulator gegen eine
simulierte Telemetriequelle (`MockTelemetrySource`, echte 80-Byte-Frames @ 10 Hz) —
kein Gerät nötig. Implementiert & getestet: Live-Cockpit (Start = Tap / Stopp =
Halten mit Fortschrittsring), Fahrtaufzeichnung mit 1-Hz-Verdichtung, SwiftData-
Persistenz im Hintergrund-`ModelActor`, Verlauf-Liste (Wisch-Löschen), Fahrt-Detail
(Statistik + Höhen-/Geschwindigkeitsdiagramm über Distanz via Swift Charts +
Routen-Polyline via MapKit), Liquid-Glass-Chrome (nur Steuerelemente/Statuszeile),
sowie Absturz-Recovery **AR-DATA-04** (beim App-Start: Abschließen / Verwerfen /
Weiter fahren). Tests grün: `SmartBikeCore` (`swift test`) und App-Tests
(SwiftData-Store, Recovery) über den Xcode-Testplan.

**Nächster Schritt (iOS):** echten `BLEConnectionService` (CoreBluetooth) an die
Stelle des Mocks setzen und gegen die reale Firmware verifizieren (sobald Board/BLE
verfügbar); danach Cockpit-Personalisierung (`DashboardLayout`) und Verlaufs-
Gesamtübersicht.
