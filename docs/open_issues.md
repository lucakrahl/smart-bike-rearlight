# Open Issues

> Von Claude Code pflegbar. Kanonische Fassung: Project Bible Kap. 11.

## Kritisch
- [ ] **LED-Kanalzuordnung / Datenblatt** (3-W-COB 590–595 nm): welche LED?
  Voraussetzung für Vorwiderstands-Dimensionierung und FR-TL-06-Kalibrierung.
- [ ] **FR-TL-07 Rechtslage:** blinkendes Rücklicht nach § 67 Abs. 4 unzulässig →
  default deaktiviert; als Thesis-Zielkonflikt dokumentieren.
- [ ] **BLE unverifiziert bis Board-Tausch (M5 Teil C2, FR-TEL-01) —
  blockiert die Telemetrie komplett:** Reproduzierbarer Brownout-Bootloop bei
  `NimBLEDevice::init()`. Vollständige Root-Cause-Analyse (zehn systematische
  Tests: Bracket-Logging, Isolationstest, Nur-USB, TX-Leistung, CPU-Takt,
  Kondensator 3V3, Kondensator Vin, MT3608-Spannung, Multimeter-Verifikation,
  BOD-Abschaltung) in `docs/ble_brownout_fallstudie.md` dokumentiert. Root
  Cause: Spannungsregler des Altboards (AZ-Delivery ESP32 NodeMCU DevKit C V2)
  liefert die BLE-RF-Kalibrierungs-Transiente nicht — Firmware als Ursache
  ausgeschlossen (host-getestet 75/75, Fehler auf `init()` lokalisiert).
  Entscheidung: Board-Tausch auf Espressif ESP32-DevKitC-32E (WROOM-32E,
  **bestellt**, Eintreffen aussteht); Entkopplungskondensatoren (3V3 + Vin)
  bleiben verbaut.
  **Vor dem Auslöten/Umsetzen der Sensorik/Peripherie:** Pin-Zahl (38),
  Bauform und Board-Breite des neuen Boards gegen das Altboard prüfen (reiner
  Board-Tausch nur bei tatsächlicher Pin-Kompatibilität sinnvoll, s.
  ESP32-DevKitC Getting-Started-Guide).
  **Nach dem Board-Tausch weiterhin offen:** MTU-Verhandlung < 83 Byte am
  realen Client (bisher keine BLE-Verbindung zustande gekommen, s.
  `BLE_PREFERRED_MTU` in `config.h`, TODO(offen)); allgemeine BLE-Verifikation
  (Advertising, Verbindung, Reconnect-Backfill) per nRF Connect o. ä.

## Wichtig
- [ ] **RF-Verifikationstest:** Wiederhol-Intervall der Fernbedienung messen →
  finaler `RF_RELEASE_TIMEOUT_MS` (FR-RF-03).
- [ ] **Brown-Out unter LED-Lastspitzen:** Pufferkondensator am Vin / MT3608
  (vermutlich derselbe Headroom-Mangel wie beim BLE-Brownout oben, s. Kritisch).
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
- [ ] Nachweise: Lichtstärke (cd) § 67, Messprotokolle, Feld-Kalibrierdaten.
- [ ] **GNSS-Fix-Feldtest** im Freien (freie Himmelssicht) — Indoor-Test zeigte
  nur `NO_FIX` (UART/Parsing bestätigt, echter Fix indoor nicht möglich).
- [ ] **BMP280-Temperatur-Retest** im thermisch eingeschwungenen Zustand
  (FORCED-Mode-Umstellung reduzierte Selbsterwärmung; finale Validierung
  gegen Referenzinstrument steht noch aus).
