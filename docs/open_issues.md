# Open Issues

> Von Claude Code pflegbar. Kanonische Fassung: Project Bible Kap. 11.

## Kritisch
- [ ] **LED-Kanalzuordnung / Datenblatt** (3-W-COB 590–595 nm): welche LED?
  Voraussetzung für Vorwiderstands-Dimensionierung und FR-TL-06-Kalibrierung.
- [ ] **FR-TL-07 Rechtslage:** blinkendes Rücklicht nach § 67 Abs. 4 unzulässig →
  default deaktiviert; als Thesis-Zielkonflikt dokumentieren.

## Wichtig
- [ ] **RF-Verifikationstest:** Wiederhol-Intervall der Fernbedienung messen →
  finaler `RF_RELEASE_TIMEOUT_MS` (FR-RF-03).
- [ ] **Brown-Out unter LED-Lastspitzen:** Pufferkondensator am Vin / MT3608
  (vermutlich derselbe Headroom-Mangel wie der durch Board-Tausch behobene
  BLE-Brownout, s. `docs/ble_brownout_fallstudie.md`).
- [ ] **Schaltplan-Korrekturen:** RF GPIO34→GPIO4; GPIO25↔GPIO26; 3× 10-kΩ-
  Pull-Down; SW1; Entkopplungskondensatoren.
- [ ] **BOM-Ergänzungen:** 10-kΩ-Pull-Down (3×), Drucktaster IP65 8 mm, LP103454.
- [ ] **RF-Empfänger-Bezeichnung:** BOM „PT2262" vs. real SRX882S vereinheitlichen.
- [ ] **IMU-Plausibilitäts-/Recovery-Schwellen (`TODO(offen)` in `config.h`):**
  `IMU_ACCEL_MAX_SLEW_MS2`, `IMU_GYRO_MAX_SLEW_RADS`,
  `IMU_ESCALATION_CONFIRM_CYCLES`, `IMU_FROZEN_LIMIT`,
  `IMU_ACCEL_MIN_MAGNITUDE_MS2`/`IMU_ACCEL_MAX_MAGNITUDE_MS2` — Erstschätzungen,
  Feldverifikation ausstehend.
- [ ] **SCL-Release-Timing real verifizieren** (Bit-Bang-Pulsbreite/-Anzahl am
  tatsächlich hängenden Bus, nicht nur am Kurzschlussfall). Direkter
  `i2c_del_master_bus`/`i2c_new_master_bus`-Fallback (Umgehung von `Wire`)
  nur bei Bedarf implementieren, falls `Wire.end()` nach dem PeriMan-Fix
  noch scheitert.

## Zu verifizieren
- [ ] „HSD ESP32 IoT Base" als Web-App-Basis?
- [x] Messprotokoll Bremslicht-*Logik* — Serial-Bench (`docs/Validierung/measurement_log.md`,
  Firmware `d8a4e75`), s. Bible Kap. 9.3.
- [ ] Nachweise: Lichtstärke (cd) § 67 (photometrische Messung, separate
  Hardware-Eigenschaft) — weiterhin offen.
- [ ] **Feldkalibrierung der Bremsschwellen** (2,0/5,0/1,5 m/s² für reales
  Fahren geeignet?) — weiterhin **getrennt offen**: die Bench validiert nur
  die Logik mit den konfigurierten Schwellen, nicht deren reale Feldeignung
  (Feldtest 30-Zone aussteht, s. `current_context.md`).
- [ ] **GNSS-Fix-Feldtest** im Freien (freie Himmelssicht) — Indoor-Test zeigte
  nur `NO_FIX` (UART/Parsing bestätigt, echter Fix indoor nicht möglich).
- [ ] **BMP280-Temperatur-Retest** im thermisch eingeschwungenen Zustand
  (FORCED-Mode-Umstellung reduzierte Selbsterwärmung; finale Validierung
  gegen Referenzinstrument steht noch aus).
