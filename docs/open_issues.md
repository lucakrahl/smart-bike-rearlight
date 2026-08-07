# Open Issues

> Von Claude Code pflegbar. Kanonische Fassung: Project Bible Kap. 11.
> **Stand 07.08.2026** — Zwischenstand vor der Wiederholungs-Messfahrt.

## Kritisch

- [ ] **Ruhewert `brake_decel_ms2` ≈ 3,0 m/s² ungeklärt (07.08.2026)** — Bei
  ruhendem Gerät müsste der Filterausgang nach etwa 30 s praktisch null sein
  (wirksame Zeitkonstante ~4 s). Zwei Hypothesen: andere Neigungsachse als
  angenommen (die 18° wurden zirkulär aus dem Messwert selbst zurückgerechnet)
  oder die Accelerometer-Korrektur liegt hinter der nie abschließenden
  Verankerung — dann wäre der Filter zu reiner Gyro-Integration degeneriert.
  **Entscheidet der Neigungstest.** Höchste Priorität; blockiert die Messfahrt.

- [ ] **Verankerungsfenster praktisch unerreichbar (07.08.2026)** — Die
  Forderung nach 100 zusammenhängenden STATIC-Abtastungen wird real nie
  erfüllt (0,857¹⁰⁰ ≈ 2 · 10⁻⁷). `bias_calibrated` bleibt 0, das System läuft
  dauerhaft mit `MOTION_COMPL_TAU_SLOW_UNCAL_S = 30 s`; die auf 90 s
  ausgelegte Konfiguration ist im Feld nie aktiv. Behebung beschlossen:
  Fenster 1,0 → 0,3 s mit Toleranzzähler (Abbruch erst nach drei
  aufeinanderfolgenden Nicht-STATIC-Abtastungen); Bias-Kalibrierung getrennt
  auf kumulierte STATIC-Abtastungen (200) ohne Zusammenhangsforderung.

- [ ] **Wiederholung des Feldtests nach Stufe 1** — identisches Protokoll vom
  06.08.2026 (sechs Fahrten, gleiche Strecke, App- und Strava-Aufzeichnung).
  Akzeptanzkriterium: r(a_GNSS, `brake_decel_ms2`) deutlich > 0,5 und keine
  Fehlauslösung > 50 % bei konstanter Geschwindigkeit.

- [ ] **LED-Kanalzuordnung / Datenblatt** (3-W-COB 590–595 nm): welche LED?
  Voraussetzung für Vorwiderstands-Dimensionierung und FR-TL-06-Kalibrierung.

- [ ] **FR-TL-07 Rechtslage:** blinkendes Rücklicht nach § 67 Abs. 4
  unzulässig → default deaktiviert; als Thesis-Zielkonflikt dokumentieren.

## Wichtig

- [ ] **MPU6050-Konfiguration dauerhaft korrigieren** — `DLPF_CFG = 0`
  (260 Hz) und `SMPLRT_DIV = 0` (8 kHz interne Rate) sind POR-Defaults, die
  `Adafruit_MPU6050::begin()` nie überschreibt. Bei 100 Hz Auslesung ist das
  Unterabtastung ohne Antialiasing. Beschlossen: `DLPF_CFG = 3` (44 Hz) und
  `SMPLRT_DIV = 4` (200 Hz Sensorrate, damit kein Schwebungseffekt zwischen
  Sensor- und Auslesetakt). Registerwerte nach dem Setzen zurücklesen und
  verifizieren. Zusätzliche Gruppenlaufzeit 4,9 ms gegen NFR-RT-01 ≤ 50 ms
  bewerten und dokumentieren.

- [ ] **Restrauschen nicht vollständig erklärt** — Nach der Filterkorrektur
  bleibt die gemessene `norm_delta`-Spanne über der Datenblatt-Erwartung. Die
  EMV-Hypothese (MT3608, 5-kHz-PWM) ist mit je einem 30-s-Lauf nicht
  auflösbar, weil die Streuung zwischen Wiederholungen größer ist als der
  Effekt. Ein belastbarer Nachweis bräuchte Wiederholungen je Laststufe und
  eine Auswertung im Frequenzbereich. **Als Grenze der Arbeit dokumentieren,
  nicht mehr untersuchen** (Umfangsschnitt 07.08.2026).

- [ ] **Effektive Ansprechschwelle 2,13 m/s² dokumentieren** — Die
  verbleibende Filterdämpfung von 5,9 % bedeutet, dass die nominelle Schwelle
  von 2,0 m/s² real erst bei etwa 2,13 m/s² erreicht wird. In der Arbeit so
  auszuweisen; eine Angabe von „2,0 m/s²" wäre nicht belegbar.

- [ ] **Drei Schwellwerte bleiben provisorisch** — `MOTION_NORM_STATIC_BAND`
  (0,12), `_JERK_DELTA` (2,0), `_SHOCK_DELTA` (6,0) sind aus physikalischen
  Überlegungen abgeleitet, nicht aus Felddaten parametriert. Beschluss
  07.08.2026: Sie bleiben unverändert; die Messfahrt **dokumentiert** ihr
  Verhalten, sie werden nicht iterativ nachgeführt.

- [ ] **`docs/Validierung/measurement_log.md` korrigieren** — enthält eine
  nicht belegbare Firmware-Hash-Angabe (`d8a4e75`) und die zu weit gefasste
  Aussage, die Bench speise „denselben Signalpfad" wie der Normalbetrieb
  (`BENCH_MODE` umgeht `motion_filter` und `lifecycle_fsm`). **Zusätzlich zu
  streichen:** die Behauptung einer 15-prozentigen Ratenverzerrung der
  Gyro-Integration — sie ist falsch, `dt_s` wird in `main.cpp` real gemessen.

- [ ] **`ios-app/SmartBikeRearLight/README.md` fehlt** — App Bible Kap. 9
  führt sowohl `CLAUDE.md` als auch `README.md` auf. `CLAUDE.md` ist am
  07.08.2026 angelegt worden, `README.md` fehlt weiterhin.

- [ ] **`project.pbxproj` mit ungetrackter Änderung** — Xcode hat beim Öffnen
  vier Zeilen mit Verweisen auf ein nicht mehr vorhandenes `Info-Setup.md`
  entfernt. Unkritisch; beim regulären Abschluss-Commit mitnehmen.

- [ ] **RF-Verifikationstest:** Wiederhol-Intervall der Fernbedienung messen →
  finaler `RF_RELEASE_TIMEOUT_MS` (FR-RF-03).

- [ ] **Brown-Out unter LED-Lastspitzen:** Pufferkondensator am Vin / MT3608
  (vermutlich derselbe Headroom-Mangel wie der durch Board-Tausch behobene
  BLE-Brownout, s. `docs/ble_brownout_fallstudie.md`).

- [ ] **Schaltplan-Korrekturen:** RF GPIO34→GPIO4; GPIO25↔GPIO26; 3× 10-kΩ-
  Pull-Down; SW1; Entkopplungskondensatoren.

- [ ] **BOM-Ergänzungen:** 10-kΩ-Pull-Down (3×), Drucktaster IP65 8 mm,
  LP103454. **RF-Empfänger-Bezeichnung:** BOM „PT2262" vs. real SRX882S
  vereinheitlichen.

- [ ] **IMU-Plausibilitäts-/Recovery-Schwellen (`TODO(offen)` in `config.h`):**
  `IMU_ACCEL_MAX_SLEW_MS2`, `IMU_GYRO_MAX_SLEW_RADS`,
  `IMU_ESCALATION_CONFIRM_CYCLES`, `IMU_FROZEN_LIMIT`,
  `IMU_ACCEL_MIN/MAX_MAGNITUDE_MS2` — Erstschätzungen, Feldverifikation
  ausstehend. **Hinweis:** `IMU_FROZEN_LIMIT` löste in synthetischen
  Testprofilen aus, weil bit-identische Samples auf realer Hardware nicht
  vorkommen (Rauschen ≈ 8 LSB). In Tests wird das über eine
  Sub-LSB-Alternation (±0,001 m/s², ca. 0,21 LSB) umgangen — bewusstes
  Testartefakt, dokumentiert.

- [ ] **SCL-Release-Timing real verifizieren** (Bit-Bang-Pulsbreite/-Anzahl am
  tatsächlich hängenden Bus, nicht nur am Kurzschlussfall).

## Zu verifizieren

- [x] **NFR-RT-04 (Worst-Case-Schleifenzeit im Normalbetrieb)** — erledigt
  07.08.2026: **0,651 ms** gegen eine Anforderung von < 10 ms, gemessen über
  das v3-Feld `loop_max_us` im `esp32dev`-Normalbuild.
- [x] **Direkter Nachweis der internen Nickschätzung (`pitch_rad_`)** —
  erledigt über das erweiterte Diagnose-Log und die Bench-Experimente D/E/F.
- [x] Messprotokoll Bremslicht-*Logik* — Serial-Bench, s. Bible Kap. 9.3
  (Korrekturbedarf s. oben).
- [x] **GNSS-Fix-Feldtest** — bestanden 06.08.2026; neuer Befund:
  Qualitätsflaggen erkennen eine falsche Navigationslösung unter Abschattung
  nicht (Bible Kap. 9.4).
- [ ] „HSD ESP32 IoT Base" als Web-App-Basis?
- [ ] Nachweise: Lichtstärke (cd) § 67 (photometrische Messung, separate
  Hardware-Eigenschaft).
- [ ] **BMP280-Temperatur-Retest** im thermisch eingeschwungenen Zustand.
