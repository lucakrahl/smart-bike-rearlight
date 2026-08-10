# Open Issues

> Von Claude Code pflegbar. Kanonische Fassung: Project Bible Kap. 11.
> **Stand 07.08.2026** — Zwischenstand vor der Wiederholungs-Messfahrt.

## Kritisch

- [x] **Ruhewert `brake_decel_ms2` ≈ 3,0 m/s²** — **geklärt und behoben
  07.08.2026.** Ursache: unkompensierter Gyro-Nullpunktfehler von −4,61 °/s.
  Über ε = b · τ ergibt das bei τ_eff = 4 s einen Lagefehler von 18,4° und
  damit g · sin(18,4°) = 3,1 m/s² — deckt sich mit dem Messwert. Nach der
  Reparaturrunde tritt kein Sockel mehr auf.

- [x] **Verankerungsfenster praktisch unerreichbar** — **behoben 07.08.2026.**
  Verankerung jetzt 0,3 s mit Toleranz für zwei aufeinanderfolgende Ausreißer,
  Bias-Kalibrierung über 200 kumulierte STATIC-Abtastungen ohne
  Zusammenhangsforderung. `bias_calibrated` erreicht am Board nach ≈ 3 s den
  Wert 1 (vorher nie in 80 s).

- [x] **iOS-Arbeitspakete AP0 bis AP8** — **erledigt 07.08.2026** (App Bible
  v0.22, 89 Tests grün, Golden-Vektor-Kreuztest bestanden, am realen iPhone
  verifiziert mit `truncatedV3FrameCount = 0`). Die Messfahrt ist damit nicht
  mehr blockiert. Empfohlen bleibt eine zweiminütige Testaufzeichnung am
  Schreibtisch, bevor gefahren wird.

- [ ] **Entscheidungs-Kürzel E1–E5 kollidieren (Dokumentationsfehler)** — Es
  existieren zwei Nummerierungen mit identischen Labels. Aus dem
  Feldtestbericht 06.08.: E1 = `gnss_accel_ms2` ins Frame, E2 = 10-Hz-
  Aufzeichnung, E3 = NFR-RT-01 bleibt bei 50 ms, E4 = `temperature_c` aus dem
  CSV, E5 = Stufe-2-Fusion deaktiviert. Aus dem iOS-Nachtrag 07.08.:
  E-1 = `truncatedV3FrameCount`, E-2 = Mischversionen in der Präambel,
  E-3 = Golden-Vektor-Kreuztest, E-4 = `t_s` mit zwei Nachkommastellen.
  In App Bible v0.22 stehen beide Bedeutungen von „E-2" im selben Satz.
  **Festlegung:** Die Nachtrags-Entscheidungen werden in **V3-1 bis V3-4**
  umbenannt; die Feldtest-Kürzel E1–E5 bleiben unverändert. App Bible und
  `CSV_Format_v3_Validierungsexport.md` sind entsprechend nachzuziehen.

- [ ] **`frame_v3_golden.md` um den Firmware-Git-Hash ergänzen** — Der
  Golden-Vektor ist der Schnittstellennachweis; ohne Angabe, gegen welchen
  Firmware-Stand kreuzvalidiert wurde, ist er nicht nachvollziehbar. Der Hash
  lautet **`1178017`**. (Die App Bible fordert das in Kap. 13 selbst. Wir
  hatten dieselbe Lücke schon einmal in `measurement_log.md`.)

- [ ] **Feldanzahl im Golden-Vektor vereinheitlichen** — Firmware-Seite nennt
  41 „unterscheidbare Werte", App-Seite 43 getroffene Felder. Vermutlich
  unterschiedliche Zählweise (Booleans). Eine Zahl festlegen, damit in der
  Arbeit nicht zwei stehen.

- [ ] **`loop_max_us` eindeutig definieren** — Das Frame-Feld ist das Maximum
  je 100-ms-Fenster (App zeigt 53 µs), die Bench-Messung war der Worst Case
  über einen längeren Lauf (0,651 ms). Für NFR-RT-04 ist der **Worst Case**
  maßgeblich. In der Doku eindeutig kennzeichnen, sonst stehen zwei Zahlen für
  dieselbe Anforderung.

- [ ] **Doppelte App-Bible-Fassung im Projekt** — Neben `claude/app_bible.md`
  (kanonisch, v0.22) liegt der veraltete Schnappschuss `App_Bible_v0.21.md`.
  Vor der Abgabe entfernen oder eindeutig als überholt kennzeichnen.

- [ ] **Wiederholung des Feldtests nach Stufe 1** — identisches Protokoll vom
  06.08.2026 (sechs Fahrten, gleiche Strecke, App- und Strava-Aufzeichnung).
  Akzeptanzkriterium: r(a_GNSS, `brake_decel_ms2`) deutlich > 0,5 und keine
  Fehlauslösung > 50 % bei konstanter Geschwindigkeit.

- [ ] **LED-Kanalzuordnung / Datenblatt** (3-W-COB 590–595 nm): welche LED?
  Voraussetzung für Vorwiderstands-Dimensionierung und FR-TL-06-Kalibrierung.

- [ ] **FR-TL-07 Rechtslage:** blinkendes Rücklicht nach § 67 Abs. 4
  unzulässig → default deaktiviert; als Thesis-Zielkonflikt dokumentieren.

## Wichtig

- [x] **MPU6050-Konfiguration** — **erledigt 07.08.2026.** `DLPF_CFG = 3`
  (44 Hz) und `SMPLRT_DIV = 4` (200 Hz) fest in `imu_driver.cpp`, mit
  Register-Readback am Board verifiziert. STATIC-Anteil im Ruhezustand
  dadurch von ~70 % auf 90–100 %.

- [ ] **`pitch_rad` behält im Ruhezustand einen Restwert von ≈ 4,2°** — Offen
  ist, ob das die tatsächliche Auflagefläche abbildet (dann korrekt) oder ein
  Restfehler ist. Im zweiten Fall verbirgt die Nullbegrenzung des Ausgangs
  einen Versatz von etwa 0,72 m/s², und die effektive Ansprechschwelle läge
  nicht bei 2,13, sondern bei rund 2,85 m/s². **Aus den Fahrdaten beantworten**
  (Vergleich `pitch_rad` gegen die aus `accel_y_ms2` folgende Lage in ruhigen
  Phasen), nicht in einer weiteren Schreibtischrunde.
  **Prüfkriterium nach der Einbaulage-Korrektur (09.08.2026,
  `IMU_MOUNT_SIGN_*`):** `pitch_rad` muss im Ruhezustand wieder ≈ **+4,2°**
  liefern (gleiches Vorzeichen wie vor der Korrektur) — das stützt die
  Annahme, dass die 4,2° der Aufstellwinkel des Gehäuses sind, nicht ein
  Artefakt der (jetzt behobenen) vertauschten Achsen. Ein Wert von ≈ −4,2°
  zeigt ein falsches Vorzeichen in der Transformation an; ein völlig
  abweichender Wert widerlegt die Aufstellwinkel-Annahme insgesamt.

- [ ] **Bias-Tor `MOTION_GYRO_BIAS_MAX_RATE` = 2 °/s prüfen** — Der gemessene
  Offset von −4,61 °/s liegt darüber, die Kalibrierung lief dennoch. Zu
  klären, ob das Tor auf die rohe Drehrate oder auf den Rest nach Abzug des
  aktuellen Schätzwerts wirkt. Bei der ersten Variante könnte ein Gerät mit
  größerem Offset nie kalibrieren — dieselbe Fehlerklasse wie die am
  07.08.2026 behobene. Praktisch derzeit unkritisch, da die Kalibrierung
  nachweislich funktioniert.

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