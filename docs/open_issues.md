# Open Issues

> Von Claude Code pflegbar. Kanonische Fassung: Project Bible Kap. 11.
> **Stand 10.08.2026** — nach dem Firmware-Abschluss (Commit `835c7b3`).

## Firmware — abgeschlossen

**Keine offenen Punkte.** Die Firmware ist mit Commit `835c7b3` vom 10.08.2026
abgeschlossen und eingefroren: 126/126 Host-Tests grün, `esp32dev` und
`esp32dev_bench` bauen fehlerfrei, keine `TODO`- oder `FIXME`-Marker im eigenen
Quellcode, Auslieferungsstand auf das Gerät geflasht und im Normalbetrieb
geprüft. Weitere Änderungen sind nur bei einem funktionsverhindernden Fehler
vorgesehen.

Mit diesem Stand geschlossen:

- [x] **Einbaulage-Transformation nach der 180°-Drehung** — `imu_mount_orientation.h`
  an der Treibergrenze, `IMU_MOUNT_SIGN_X/Y/Z` = −1/−1/+1 auf Beschleunigung und
  Drehrate, Determinante +1, eigener Host-Test. `MOTION_BRAKE_SIGN` bleibt bei +1,
  weil die Transformation die vor dem Umbau verifizierte Konvention wiederherstellt.
- [x] **Mangel M-01 — Mindesthaltezeit unwirksam** — der Haltewert wird nur noch
  oberhalb `BRAKE_ON_MS2` nachgeführt. Regressionstest ergänzt, der einen
  Bremsvorgang monoton durch das Hystereseband fahren lässt. Am Gerät bestätigt.
- [x] **Debug-Ausgaben** — alle drei `TODO(temp debug)`-Prints entfernt,
  `DEBUG_SERIAL = false`. Der ungegatete `[R1/R2]`-Print lief mit 1 Hz im
  100-Hz-Task und lag im Messfenster von `loop_max_us` (s. Befund B7).
- [x] **Versionsfestigkeit der Bibliotheken** — alle sechs `lib_deps` gepinnt.
- [x] **Tote Symbole** — `CONFIG_VERSION`, `COMPL_FILTER_ALPHA`, `bleGetMtu()`
  entfernt.
- [x] **Veraltete Kommentare** — `main.cpp`-Kopf („NOCH GERUEST"),
  `MOTION_ANCHOR_WINDOW_S` (0,3 s statt 1,0 s), `tau_slow` (90 s statt 30 s).
- [x] **MPU6050-Konfiguration** (07.08.2026) — `DLPF_CFG = 3` (44 Hz),
  `SMPLRT_DIV = 4` (200 Hz), mit Register-Readback verifiziert.
- [x] **Ruhewert `brake_decel_ms2` ≈ 3,0 m/s²** (07.08.2026) — Ursache
  unkompensierter Gyro-Nullpunktfehler von −4,61 °/s; über ε = b · τ analytisch
  bestätigt.
- [x] **Verankerungsfenster praktisch unerreichbar** (07.08.2026) — Verankerung
  0,3 s, Bias-Kalibrierung über 200 kumulierte STATIC-Abtastungen ohne
  Zusammenhangsforderung.
- [x] **iOS-Arbeitspakete AP0 bis AP8** (07.08.2026) — App Bible v0.22, 89 Tests
  grün, Golden-Vektor-Kreuztest bestanden, am realen iPhone verifiziert.
- [x] **NFR-RT-04 Schleifenzeit** — Prüfstand 0,651 ms, Fahrbetrieb 6,7 ms,
  Anforderung < 10 ms erfüllt. Zur Ursachenzuordnung s. unten.

**Abgegrenzt statt umgesetzt** (Umfangsschnitt 10.08.2026, Begründung und
Auswirkung je Position in Project Bible Kap. 12.2): FR-CFG-02 (serielles
Kalibrier-Interface), FR-CFG-03 (NVS-Konfiguration), Feldverifikation der
IMU-Plausibilitätsschwellen und der BLE-Parameter, Messung des
RF-Wiederholintervalls, I²C-Recovery für den BMP280, SCL-Release am real
hängenden Bus, Teil A des Messprotokolls, Trennung der beiden Ursachen von
Befund B7, Nachmessung des `pitch_rad`-Restwerts, EMV-Frequenzanalyse, weitere
Iteration der drei Normbetrags-Schwellwerte.

## Hardware und Elektronik

- [ ] **Befund B-1, UART-Pegel** — ESP32 treibt GPIO17 mit 3,3 V, L86-Datenblatt
  nennt für RXD1 V_IHmax = 3,1 V. Serienwiderstand 1 kΩ, Teiler 1 k/10 k oder als
  dokumentierte Abweichung führen. Betrieb seit Wochen unauffällig.
- [ ] **Befund B-6, Nennstrom von SW1** — der Schalter führt seit der Korrektur
  der Schalterposition den Akkustrom, Worst Case 1,18 A. Kein Datenblatt für den
  verbauten 8-mm-Drucktaster. Herstellerangabe beschaffen oder Spannungsabfall
  über dem geschlossenen Kontakt unter Last messen.
- [ ] **Befund B-4, fehlende HF-Entkopplung am L86** — dokumentieren oder
  nachrüsten.
- [ ] **BOM-Korrekturen** — GNSS-Antenne Namvo als nicht verbaut kennzeichnen;
  RF-Empfänger einheitlich als SRX882S (nicht „PT2262"); Positionen 10-kΩ-Pull-Down
  (3×), Drucktaster IP65 8 mm, LP103454, Widerstandsnetze RN1–RN3 ergänzen.
- [ ] **Physische Blinker-L/R-Zuordnung** noch nicht festgelegt.
- [ ] **Datenblatt der 3-W-COB-LED** (Vrabocry) fehlt; V_f = 2,2 V ist der
  Mittelwert der Produktangabe 2,0–2,4 V.
- [ ] **Anordnung der Patch-Antenne im Gehäuse** gegen die Datenblattvorgaben
  prüfen.
- [ ] **Lichtstärke (cd) nach § 67** — photometrische Messung, separate
  Hardware-Eigenschaft.
- [ ] **Wirkungsgrad des MT3608 unter realer Last** (angenommen η = 0,90).
- [ ] **Energiebilanz** — Laufzeitangabe ist gerechnet, nicht gemessen
  (NFR-PWR-02).

## Dokumentation und Thesis-Text

- [ ] **`loop_max_us` eindeutig definieren** — Frame-Feld ist das Maximum je
  100-ms-Fenster (App: 53 µs), Nachweis für NFR-RT-04 ist der Worst Case über den
  Lauf (0,651 ms Prüfstand, 6,7 ms Fahrbetrieb). Sonst stehen zwei Zahlen für
  dieselbe Anforderung.
- [ ] **Effektive Ansprechschwelle 2,13 m/s² ausweisen** — die Restdämpfung von
  5,9 % bedeutet, dass die nominelle Schwelle von 2,0 m/s² real erst bei etwa
  2,13 m/s² erreicht wird.
- [ ] **Firmware-Git-Hash im Golden-Vektor** — `testdata/frame_v3_golden.md` um
  `1178017` ergänzen.
- [ ] **Feldanzahl im Golden-Vektor vereinheitlichen** — 41 („unterscheidbare
  Werte", Firmware) vs. 43 (getroffene Felder, App).
- [ ] **Entscheidungs-Kürzel V3-1 bis V3-4** in App Bible und
  `CSV_Format_v3_Validierungsexport.md` nachziehen (die Feldtest-Kürzel E1–E5
  bleiben unverändert).
- [ ] **`docs/Validierung/measurement_log.md` korrigieren** — nicht belegbarer
  Firmware-Hash (`d8a4e75`); die zu weit gefasste Aussage, die Bench speise
  „denselben Signalpfad" wie der Normalbetrieb (`BENCH_MODE` umgeht
  `motion_filter` und `lifecycle_fsm`); die falsche Behauptung einer
  15-prozentigen Ratenverzerrung der Gyro-Integration (`dt_s` wird real gemessen).
- [ ] **Doppelte App-Bible-Fassung** — `App_Bible_v0.21.md` ist ein veralteter
  Schnappschuss neben dem kanonischen `claude/app_bible.md` (v0.22). Vor der
  Abgabe entfernen oder als überholt kennzeichnen.
- [ ] **`ios-app/SmartBikeRearLight/README.md`** fehlt (App Bible Kap. 9).
- [ ] **`project.pbxproj`** trägt eine ungetrackte Änderung (vier entfernte Zeilen
  mit Verweisen auf ein nicht mehr vorhandenes `Info-Setup.md`). Beim
  Abschluss-Commit mitnehmen.
- [ ] **FR-TL-07 Rechtslage** — blinkendes Rücklicht nach § 67 Abs. 4 unzulässig,
  daher default deaktiviert; als Thesis-Zielkonflikt ausformulieren.
- [ ] **„HSD ESP32 IoT Base" als Web-App-Basis** — historischer Punkt, mit der
  Entscheidung für die native iOS-App gegenstandslos. Vor der Abgabe streichen.

## App (iOS)

- [ ] Verlaufs-Gesamtübersicht (AR-HIST-…) — offen.
- [ ] Finale SF-Symbol-Wahl, optionale Höhen-Referenzdruck-Kalibrierung — Future
  Work.
