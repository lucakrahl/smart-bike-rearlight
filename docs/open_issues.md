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
- [ ] **Brown-Out unter LED-Lastspitzen:** Pufferkondensator am Vin / MT3608.
- [ ] **Schaltplan-Korrekturen:** RF GPIO34→GPIO4; GPIO25↔GPIO26; 3× 10-kΩ-
  Pull-Down; SW1; Entkopplungskondensatoren.
- [ ] **BOM-Ergänzungen:** 10-kΩ-Pull-Down (3×), Drucktaster IP65 8 mm, LP103454.
- [ ] **RF-Empfänger-Bezeichnung:** BOM „PT2262" vs. real SRX882S vereinheitlichen.

## Zu verifizieren
- [ ] „HSD ESP32 IoT Base" als Web-App-Basis?
- [ ] Nachweise: Lichtstärke (cd) § 67, Messprotokolle, Feld-Kalibrierdaten.
