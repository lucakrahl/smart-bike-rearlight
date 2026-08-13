# Entwicklungsprozess Firmware — Chronologie aus der Commit-Historie

**Zweck:** Lückenlose Chronologie der Firmware-Entwicklung von M0 bis zum
Firmware-Abschluss, aus `git log` rekonstruiert (kein Gedächtnisprotokoll).
Umfasst den vollständigen Commit-Bereich `83f364d..835c7b3` (39 Commits,
24.07.–10.08.2026) — inklusive der eng verzahnten iOS-App- und
Doku-Commits, weil das Repo eine gemeinsame Historie führt und mehrere
Firmware-Probleme (P10, P12) erst im Zusammenspiel mit der App bzw. der
Doku sichtbar werden.

**Quellen je Zeile:** Commit-Hash + `git log`/`git show` (Betreff und
Commit-Body), ergänzend `docs/Thesis_Transfer_Firmware.md` Abschnitt 11
(Problem-Tabelle P1–P12) für die Zuordnungsspalte, wo die Commit-Message
selbst keine Zuordnung hergibt.

**Spalte „Geräteverifikation":** belegt = die Commit-Message selbst nennt
einen Test/Beleg am realen Board (bzw. bei iOS-Zeilen am iPhone); über
Doku belegt = kein Beleg in der Commit-Message, aber in einem referenzierten
Dokument; Host-Test/Build = die Message nennt ausdrücklich nur
`pio test`/Unit-Tests, keinen Gerätebezug; TODO(offen) = kein Beleg
auffindbar (nicht: "nicht verifiziert" — nur nicht belegt).

**Spalte „P-Zuordnung":** Bezug zu den zwölf Entwicklungsproblemen
P1–P12 aus `docs/Thesis_Transfer_Firmware.md` Abschnitt 11, nur wo aus
Commit-Message oder Doku klar herleitbar. Leer = kein belegbarer Bezug
(nicht: "kein Bezug" — nur nicht belegt).

---

| # | Datum | Commit | Bereich | Fachlicher Inhalt | Geräteverifikation | P-Zuordnung |
|---|---|---|---|---|---|---|
| 1 | 24.07.2026 | `83f364d` | Firmware | M0: PlatformIO-Projektgerüst angelegt (Verzeichnisstruktur `firmware/src`, `lib/logic`, `lib/drivers`, `test`). | TODO(offen) — Commit-Message ohne Body, kein Beleg auffindbar. | |
| 2 | 25.07.2026 | `618c93a` | Firmware/Build | Wechsel auf die pioarduino-Plattform (Arduino-ESP32-Core 3.x) für `ledcAttach()`/`ledcWrite()`; Build-Doku zu einem lzma-Problem ergänzt. | TODO(offen) — kein Body, kein Geräte- oder Testbeleg auffindbar. | |
| 3 | 25.07.2026 | `5480ef3` | Firmware | M1: R2-Zustandsmaschine (Schlusslicht/Bremslicht) inkl. Bremskennlinie (FR-TL-06), host-getestet. | Host-Test/Build — Message nennt nur „Host-Tests", kein Gerätebezug. | |
| 4 | 25.07.2026 | `83ba8bf` | Firmware | M2: R1-Lebenszyklus-FSM (Init-Timeout), host-getestet. | Host-Test/Build. | |
| 5 | 25.07.2026 | `6d9bc46` | Dokumentation | Wissensdatenbank (Roadmap/Current-Context) nach Abschluss M1/M2 fortgeschrieben. | — (reine Doku) | |
| 6 | 25.07.2026 | `a1ce3ba` | Firmware | R1+R2 in `main.cpp` verdrahtet, LED-PWM-Treiber ergänzt. | **Ja** — Betreff: „erste Lichtausgabe auf Hardware". | |
| 7 | 25.07.2026 | `c6be4cb` | Firmware | M3: IMU-Treiber + Bewegungsfilter (richtungsabhängige Bremserkennung), host-getestet. | Host-Test/Build — kein Gerätebezug in dieser Message (Board-Kalibrierung folgt unmittelbar in `94f7567`). | |
| 8 | 25.07.2026 | `c3a7681` | Dokumentation | Wissensdatenbank nach Abschluss M3 fortgeschrieben. | — | |
| 9 | 25.07.2026 | `94f7567` | Firmware | `MOTION_BRAKE_SIGN` von −1 auf +1 korrigiert (reale Einbaulage der Y-Achse), Tests angepasst. | **Ja** — Betreff: „am Board kalibriert". | |
| 10 | 25.07.2026 | `634cfd9` | Dokumentation | Wissensdatenbank fortgeschrieben, IMU-Kalibrierung (`94f7567`) vermerkt. | — | |
| 11 | 26.07.2026 | `71d9649` | Firmware | M4: R3-Blinker-FSM + RF-Empfang/Tastenerkennung, host-getestet. | Host-Test/Build. | |
| 12 | 26.07.2026 | `afae9e0` | Dokumentation | Wissensdatenbank nach Abschluss M4 fortgeschrieben; nächste Schritte Baro+GNSS (M5). | — | |
| 13 | 26.07.2026 | `3b127cd` | Dokumentation | Project Bible 0.10 — Stand M1–M5A, I²C-Bus-Init-Entscheidung, BMP280-FORCED-Begründung; darin erstmals der Akkubetrieb-Freeze-Befund dokumentiert. | Über Doku belegt — Abschnitt „Akkubetrieb-Freeze" beschreibt die Beobachtung als solche (kein Fix nötig, s. P12). | **P12** |
| 14 | 26.07.2026 | `3251af0` | Firmware | M5 Teil A: BMP280-Treiber im FORCED-Mode + zentrale `Wire.begin()`-Init in `setup()` (FR-SNS-03). | **Ja** — Body: „Am Board geflasht, Druck/Temperatur gegen Referenz verglichen". | |
| 15 | 26.07.2026 | `47e057e` | Firmware | M5 Teil B: L86-GNSS-Treiber (`gnssPump()`, nicht-blockierend) + Fix-Statuslogik (`gnss_fix`, FR-TEL-05). | **Ja** — Body: „Am Board geflasht — Drinnen-Test zeigt NO_FIX … NMEA-Datenfluss + Parsing sind bewiesen". | |
| 16 | 27.07.2026 | `4096d4d` | Firmware | `imu_health` (Wertebereich-/Frozen-/Sprung-Plausibilität, gestufte Recovery inkl. roher `gpio_*`-SCL-Freigabe) + Fail-Safe (FR-SNS-04/05, FR-STA-04). | **Ja** — Body: „Per SDA-Kurzschluss-Fehlerinjektion am Board verifiziert: Recovery wirksam …". | **P6** |
| 17 | 27.07.2026 | `749bd74` | Dokumentation | Wissensdatenbank nach „Härtung Teil 1" (I²C-Recovery/Plausibilität/Fail-Safe) fortgeschrieben; Root-Cause-Erkenntnis (`Wire.end()`/PeriMan) in `lessons_learned.md`. | — | **P6** (Doku-Beleg) |
| 18 | 27.07.2026 | `17d18bc` | Firmware | Task-Watchdog (FR-SAF-03) + Reset-Reason-Diagnose; temporärer `'H'`-Hang-Hook zur Verifikation. | **Ja** — Body: „Am Board verifiziert: 'H' gesendet → Reset nach ~2 s, Boot-Log zeigt 'recovered from watchdog reset'". | |
| 19 | 27.07.2026 | `d2de507` | Dokumentation | Wissensdatenbank nach „Härtung Teil 2" (Watchdog) fortgeschrieben; nächster Schritt Telemetrie+BLE. | — | |
| 20 | 28.07.2026 | `252ef3e` | Firmware | Telemetrie-Frame v1 (80 Byte, gepackt, memcpy-Serialisierung) + RAM-Ringpuffer (FR-TEL-02/03/04/06). | Host-Test/Build — Body: „Reine Logik, keine `main.cpp`-Verdrahtung (folgt mit BLE)". | |
| 21 | 29.07.2026 | `781f4ed` | Dokumentation | Project Bible 0.11 — Architekturentscheidung native iOS-App (Core Bluetooth) statt PWA; Firmware/BLE-Schnittstelle bleibt unverändert/client-agnostisch. | — (Architekturentscheidung, kein Firmware-Codeeingriff) | |
| 22 | 30.07.2026 | `dde56fb` | Dokumentation | BLE-Brownout-Fallstudie: zehn systematische Tests grenzen die Ursache auf den Spannungsregler des Altboards ein; Entscheidung Board-Tausch auf ESP32-DevKitC-32E. | Über Doku belegt — `docs/ble_brownout_fallstudie.md`, zehn systematische Tests, Root Cause per BOD-Abschalttest von Fehlauslösung unterschieden. | **P2** |
| 23 | 31.07.2026 | `a07d679` | iOS-App | Phase 6: Persistenz + Verlauf (App-seitig). | TODO(offen) — kein Body. | |
| 24 | 31.07.2026 | `e7f9998` | iOS-App/Repo | Xcode-Projekt + Core sauber ins Repo integriert (verschachteltes Git-Repo entfernt). | — (Repo-Housekeeping, kein Feature) | |
| 25 | 31.07.2026 | `8a21de5` | iOS-App/Repo | Doppelte Quellordner auf `ios-app/`-Ebene entfernt. | — (Repo-Housekeeping) | |
| 26 | 01.08.2026 | `7aa2528` | iOS-App | Phase 6: Fahrt-Detail-Ansicht mit Diagrammen und Route. | TODO(offen) — kein Body. | |
| 27 | 01.08.2026 | `bd1f3ab` | Firmware | M5 Teil C2: BLE-Notify-Server an Frame+Ringpuffer angeschlossen (NimBLE, FR-TEL-01/FR-SYS-04); zwei Advertising-Bugs (fehlender Gerätename, Paketgröße) gefunden und behoben. | **Ja** — Body: „hardware-verifiziert: Advertising, Verbindung und MTU-Verhandlung (185) … laufen auf dem Ersatzboard sauber, kein Brownout". | |
| 28 | 01.08.2026 | `e007cc3` | Firmware | Isolations-Scaffold entfernt, BLE fest im Normalbetrieb; nach Einlöten des Ersatzboards stufenweise vollständig am realen System verifiziert (Pin-/Logiktest, BLE-Isolationstest, Vollbetriebstest). | **Ja**, extensiv — Body: „Vollbetriebstest … ohne Brownout, reale Verbindung über nRF Connect mit MTU=185 …". | **P2** (Validierung) |
| 29 | 05.08.2026 | `d8a4e75` | Firmware | `brake_light_pct` ins Frame (Offset 80, Schema v2) — erlaubt Vergleich Eingang/Ausgang der Bremskennlinie. | Host-Test/Build — Body nennt nur `pio test`/`pio run`, kein Gerätebezug. | |
| 30 | 06.08.2026 | `d9e3010` | Dokumentation | Standortbestimmung iOS-App + Firmware (Schema v2) + Doku. | TODO(offen) — kein Body. | |
| 31 | 06.08.2026 | `305a725` | iOS-App/Doku | App Bible angelegt. | — (reine Doku) | |
| 32 | 07.08.2026 | `90029a7` | iOS-App | AP1: `TelemetryFrame` um 13 v3-Felder als Optionals erweitert (reiner Werttyp, keine Verhaltensänderung). | Host-Test/Build — Body: „Tests: SmartBikeCore 47 grün, App 13 grün", kein Gerätebezug. | |
| 33 | 07.08.2026 | `1178017` | Firmware (+ iOS parallel) | Schema v3 (113 Byte) + „Normbetrags-Gate-Reparaturrunde": Drei-Regime-Klassifikation (STATIC/DYNAMIC/SHOCK), MPU6050-Register fest auf `DLPF_CFG=3`/`SMPLRT_DIV=4`, Verankerung von Bias-Kalibrierung entkoppelt, Bench-Harness-Zeitfehler behoben, Golden-Vektor angelegt. | **Ja**, extensiv — Body: Registerrücklesung am Board, „`bias_calibrated` erreicht 1 binnen ~3 s (vorher: nie in 80 s)", Neigungstest „100-ms-Auflösung, am Board". | **P1, P3, P4, P5, P9** |
| 34 | 07.08.2026 | `18e8938` | iOS-App | AP5: `TrackSample`/`TrackPoint` um 13 v3-Felder additiv erweitert (leichtgewichtige SwiftData-Migration). | Host-Test/Build — Body: „Round-Trip Core↔@Model", kein Gerätebezug. | |
| 35 | 07.08.2026 | `1f8a050` | iOS-App | AP6: umschaltbarer Aufzeichnungsmodus 1 Hz / 10 Hz, gepufferte Batch-Persistenz. | Host-Test/Build — Body nennt Unit-/Skalen-Tests, keinen Gerätebezug. | **P10** |
| 36 | 07.08.2026 | `71217f1` | iOS-App | AP7: CSV-Export auf 35 Spalten (v3-Felder angehängt, `temperature_c` entfernt). | Host-Test/Build — Golden-Test + Unit-Tests, kein Gerätebezug. | |
| 37 | 07.08.2026 | `c729fd8` | iOS-App | AP8: read-only Diagnoseansicht (Settings) für `bias_calibrated`/`gnss_accel_valid`/`dt_max_ms`/`loop_max_us`. | Host-Test/Build — Body: „Tests (3): reine Ableitung + Verdrahtung Store→ViewModel", kein Gerätebezug. | |
| 38 | 10.08.2026 | `cc3009f` | Firmware | Einbaulage-Rückabbildung nach der 180°-Drehung der Platine (`IMU_MOUNT_SIGN_X/Y/Z`) an der Treibergrenze; dazu Messfahrt-/Schaltplan-Doku. | Über Doku belegt — `docs/Thesis_Transfer_Firmware.md` Abschnitt 11, Zeile P8: „Host-Test; am Gerät im Normalbetrieb bestätigt" (nicht in der Commit-Message selbst). | **P8** |
| 39 | 10.08.2026 | `835c7b3` | Firmware | Firmware-Abschluss: FR-TL-06-Defekt B5 behoben (Mindesthaltezeit), alle `TODO(temp debug)`-Ausgaben entfernt, FR-CFG-02/03 als Umfangsschnitt dokumentiert, tote Symbole entfernt, `lib_deps` gepinnt. | Über Doku belegt — `docs/open_issues.md` „Firmware — abgeschlossen": „Auslieferungsstand auf das Gerät geflasht und im Normalbetrieb geprüft" (Commit-Message selbst nennt nur `pio test`/`pio run`). | **P7, P11** |

---

## Nicht zugeordnet

Kein Commit in diesem Bereich trägt einen belegbaren Bezug zu **P8** über die
Commit-Message hinaus (nur über die Doku, s. Zeile 38) und keiner zu
**P12** über die reine Erstdokumentation hinaus (s. Zeile 13) — der Befund
wurde als „im echten Akkubetrieb nicht reproduzierbar" eingestuft und
erforderte keinen Code-Fix, entsprechend gibt es keinen Auflösungs-Commit.

## Offene Punkte dieser Chronologie

- **TODO(offen):** Zeilen 1, 2, 23, 26, 30 haben keine auffindbare
  Verifikationsangabe (weder Commit-Message noch eine referenzierbare
  Doku-Stelle) — nicht mit „unverifiziert" zu verwechseln, nur nicht belegt.
- **TODO(offen):** Ob Zeile 7 (`c6be4cb`, IMU-Treiber/Bewegungsfilter) vor
  der Board-Kalibrierung in Zeile 9 (`94f7567`) bereits am Gerät lief oder
  ausschließlich host-seitig, ist aus den Commit-Messages nicht sicher
  rekonstruierbar.
