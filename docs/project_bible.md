# Project Bible — Smartes Fahrrad-Rücklicht
**Bachelorarbeit Krahl · Maschinenbau & Produktentwicklung (B.Eng.)**
**Version 0.7 · Stand 21.07.2026 · Status: aktiv gepflegt (Single Source of Truth)**

> Diese Project Bible ist die oberste Wissensinstanz des Projekts. Bei Widersprüchen zwischen Chat-Historie und Project Bible gilt ausschließlich die Project Bible. Chats dienen der Diskussion und Entscheidungsfindung; der offizielle Projektstand steht ausschließlich hier.
>
> **Hinweis:** Diese Datei ist die Repo-Arbeitskopie. Die kanonische Fassung wird im claude.ai-Projekt „Bachelorarbeit" gepflegt. Änderungen an dieser Datei nur nach den Regeln in `CLAUDE.md` (freigabepflichtig).

---

## 0. Meta & Änderungsprotokoll

### 0.1 Geltungsregeln
- Die Project Bible bildet jederzeit den offiziellen Entwicklungsstand des Gesamtsystems ab.
- Widerspricht eine frühere Chat-Aussage der Project Bible, gilt die Project Bible.
- Kapitel 10 (Entwicklungsentscheidungen) wird als lebendes Änderungsprotokoll geführt.
- Trennung der Ebenen: **Project Bible** = offizieller Stand · **Kapitel 10** = Begründung der Entscheidungen · **Chats** = Diskussion.

### 0.2 Kennzeichnungslegende
- **[gesichert]** — durch mehrere Quellen bestätigt oder vom Nutzer freigegeben.
- **[Annahme]** — plausibel, aber messtechnisch/dokumentarisch noch nicht verifiziert.
- **[offen]** — in Klärung, noch nicht entschieden.

### 0.3 Änderungsprotokoll
| Version | Datum | Änderung | Anlass |
|---|---|---|---|
| 0.1 | 21.07.2026 | Erstkonsolidierung als Analysedokument. | Projektaufnahme |
| 0.2 | 21.07.2026 | 12-Kapitel-Zielstruktur. SRS-Block A (Systemgrenzen). | Freigabe Block A |
| 0.3 | 21.07.2026 | SRS-Block B (Zustandsmodell, vier parallele Regionen). | Freigabe Block B |
| 0.4 | 21.07.2026 | SRS-Block C (Detaillogik); Norm-Grundlage § 67/ECE. | Freigabe Block C |
| 0.5 | 21.07.2026 | SRS-Block D (Präzisierungen) + E (Fehler & Sicherheit). | Freigabe Block D+E |
| 0.6 | 21.07.2026 | SRS-Block F (nichtfunktional); Akku LP103454 bestätigt. | Freigabe Block F |
| 0.7 | 21.07.2026 | SRS-Block G (Konfigurierbarkeit: FR-CFG) + H (Testbarkeit/Erweiterbarkeit: NFR-TST/EXT, FR-TEL-06). **SRS (Phase 1) vollständig.** | Freigabe Block G+H |

### 0.4 Datengrundlage
| Quelle | Zeitstempel | Aussagekraft |
|---|---|---|
| Projektübergabe-Dokument | „Stand Juni 2026" | Detaillierteste Einzelquelle |
| `blinker_brake_rf_test.ino` | 22.06.2026 | Neuester Firmware-Stand |
| `rf_led_blink_test.ino` | 22.06.2026 | RF-Validierung |
| `gyrobaro.ino` (sensor_validierung_v3) | 25.05.2026 | Komplementärfilter |
| Schaltplan v2.pdf | 20.05.2026 | Gesamtübersicht, fehlerbehaftet (Kap. 11) |
| `Uebersicht.xlsx` (BOM) | 17.02.2026 | Stückliste mit Preisen |
| Datenblätter (ESP32, BMP280, GY-521, IRLZ44N, MT3608, TP4056, L86) | Herstellerstand | Referenzwerte |
| Nutzer-Lastenheft Firmware | 21.07.2026 | Funktionaler Zielumfang (Kap. 2) |
| § 67 StVZO / ECE R6 / ECE R50 (recherchiert) | 07/2026 | Normative Grundlage (Kap. 2.8) |

---

## 1. Projektübersicht

### 1.1 Ziel
Entwicklung eines funktionsfähigen Prototyps eines *Smart Bike Rear Light*. Das System fungiert als IoT-Gerät, das mittels IMU-gesteuerter Bremslichtfunktion, Funk-Blinkern (433 MHz) und einer Live-Datenschnittstelle (BLE → Web-App) die Verkehrssicherheit und Datenaufzeichnung für Radfahrer verbessert.

### 1.2 Kurzbeschreibung
Ein ESP32 erfasst zyklisch Daten von GNSS (Quectel L86), Barometer (BMP280) und IMU (MPU-6050), steuert ein rotes Schluss-/Bremslicht (PWM) sowie gelbe Funk-Blinker und streamt Live-Telemetrie an eine Web-App (PWA), die Statistik, Sensorfusion und Visualisierung übernimmt.

### 1.3 MVP
Zuverlässige Sensorerfassung und -Streaming, funktionierendes Schluss-/Bremslicht (intensitätsabhängig), Funk-Blinker, BLE-Live-Anzeige der Basis-Kennzahlen. Details s. Kap. 2.4.

### 1.4 Future Work
Erweiterte Fahranalyse, Sensorfusion, Sicherheitsfunktionen, Pseudo-Leistung/Kalorien, automatische Blinker-Rückstellung, RF-Anlernmodus, GNSS-Warnanzeige, OTA-Updates. Details s. Kap. 2.4.

---

## 2. Anforderungen an die Firmware (SRS) — vollständig (Blöcke A–H)

### 2.0 Kennzeichnung & ID-Systematik
Anforderungs-IDs: `FR-<Subsystem>-NN` (funktional), `NFR-<Kategorie>-NN`, `CON-NN` (Randbedingung), `OUT-NN` (ausgeschlossen).
Subsystem-Kürzel: `SYS`, `TL`, `BLK`, `RF`, `SNS`, `TEL`, `STA`, `SAF` (Sicherheit), `CFG` (Konfiguration).
NFR-Kategorien: `RT` (Echtzeit), `RES` (Ressourcen), `PWR` (Energie), `TST` (Testbarkeit), `EXT` (Erweiterbarkeit).

> **Bearbeitungsstand SRS:** **Alle Blöcke A–H freigegeben — SRS vollständig (21.07.2026).** A (2.1), B (2.6), C (2.7), D (Präzisierungen in 2.6/2.7), E (2.9), F (2.10), G (2.11), H (2.12). Normative Grundlage in 2.8.

### 2.1 Systemgrenzen & Kontext — Block A

| ID | Anforderung | Status |
|---|---|---|
| FR-SYS-01 | Firmware = Datenerfassungs- und Echtzeitknoten. Alle kumulativen/fusionierten Kennzahlen werden in der Web-App berechnet (Variante 2). | gesichert |
| FR-SYS-02 | Firmware liest GNSS, BMP280, MPU6050 zyklisch aus und stellt Rohmesswerte als Telemetrie bereit. | gesichert |
| FR-SYS-03 | Lokal nur echtzeit-/sicherheitsrelevante Größen: Bremslichtintensität aus IMU-Verzögerung, Blinkerzustand. | gesichert |
| FR-SYS-04 | Schnittstelle zur App unidirektional (ESP32 → App); keine Steuerbefehle über BLE. | gesichert |
| FR-SYS-05 | Blinkersteuerung ausschließlich über 433-MHz-Fernbedienung. | gesichert |
| FR-TL-01 | Rote LED = kombiniertes Schluss-/Bremslicht; Grundzustand dauerhaft gedimmtes Schlusslicht. | gesichert |
| FR-TL-02 | Bremslicht-Helligkeit steigt mit der Bremsintensität (Kennlinie FR-TL-06). | gesichert |
| CON-01 | Datsenke = Web-App; Firmware speichert nicht dauerhaft, nur flüchtiger RAM-Ringpuffer (FR-TEL-04). | gesichert |
| OUT-01 | Keine Akkustandsanzeige/Batteriespannungsmessung in HW/FW. | gesichert |
| OUT-02 | Nicht im MVP: Auto-Helligkeit, Standlicht, Auto-Blinker-Rückstellung, Flash-Voll-Logging. | gesichert |

### 2.2 Datenkatalog — Rohmesswerte (Firmware liefert)
- **GNSS (L86):** Position, Geschwindigkeit, Heading, UTC-Zeit, Satellitenanzahl, HDOP, Höhe.
- **BMP280:** Luftdruck, Temperatur.
- **MPU-6050:** Beschleunigung (3 Achsen), Drehrate (3 Achsen).

### 2.3 Datenkatalog — abgeleitete Fahrdaten (App berechnet, FR-SYS-01)
- **aus GNSS:** aktuelle/Durchschnitts-/Höchstgeschwindigkeit, Distanz, Route, Fahrzeit, Bewegungsrichtung, Start-/Zielposition, Zwischenzeiten.
- **aus BMP280:** aktuelle Höhe, Höhenmeter bergauf/bergab, max./min. Höhe, Temperatur, durchschnittl. Steigung.
- **aus MPU:** Bremsvorgänge, Sprints, Kurvenwinkel/-anzahl, Lean Angle, Sturzerkennung, Schräglagen L/R.
- **Sensorfusion:** GNSS+MPU, GNSS+BMP, BMP+MPU, alle drei (Pseudo-Power → Kalorien, Trainingsintensität).

### 2.4 MVP- und Future-Work-Umfang (App-Anzeige)
**MVP:** aktuelle/Durchschnitts-/Maximalgeschwindigkeit, Distanz, Fahrzeit, Höhe, Höhenmeter, Satellitenanzahl, Bluetooth-Status. *(kein Akkustand, OUT-01.)*
**Future-Work:** erweiterte Fahranalyse, Sicherheitsfunktionen (Bremslicht-/Sturzerkennung, GNSS-Verlust-Warnanzeige), Nice-to-have (Pseudo-Leistung, Kalorien, Höhenprofil, Bremsereignisse, Schlagloch-/Road-Quality, Fahrstilanalyse, Effizienzindex), RF-Anlernmodus, OTA.

### 2.5 Eingangs-Lastenheft
Vollständig in den SRS-Blöcken A–H formalisiert. Keine offenen Rohanforderungen mehr.

### 2.6 Zustandsmodell — Block B (+ D-Präzisierungen)
Vier parallele (orthogonale) Regionen (Statechart nach Harel; Diagramme in Kap. 6.6). Kritischer Sensor ist ausschließlich die IMU (MPU6050); GNSS und BMP280 optional.

| ID | Anforderung | Status |
|---|---|---|
| FR-STA-01 | Power-On → INIT. Übergang INIT→RUN, sobald kritische Sensoren (IMU) initialisiert sind oder Init-Timeout 5 s abgelaufen ist. | gesichert |
| FR-STA-02 | Bei Init-Timeout → degradierter RUN: rote LED dauerhaft Schlusslicht (§ 67 gewahrt), fehlende Sensoren als Fehler geführt. | gesichert |
| FR-STA-03 | Regionen 2–4 laufen unabhängig; Sensor-/Systemfehler erzwingt kein regionsübergreifendes Sperren. | gesichert |
| FR-TL-03 | Während INIT signalisiert die rote LED per Diagnose-Blinken (~2 Hz, 50 % Duty) die Nicht-Bereitschaft. Transienter Zustand vor Betriebsbeginn. | gesichert |
| FR-TL-04 | In RUN leuchtet die rote LED dauerhaft mindestens als gedimmtes Schlusslicht (~20 %). | gesichert |
| FR-TL-05 | Bremslicht ist temporäre Helligkeitsanhebung; nach Bremsende Rückfall auf Schlusslicht (Kennlinie FR-TL-06). | gesichert |
| FR-BLK-01 | Richtungsblinker per kurzem Tastendruck (T1=links, T2=rechts); erneuter kurzer Druck derselben Taste = aus (Toggle). | gesichert |
| FR-BLK-02 | Umschalten Links↔Rechts durch andere Taste; setzt 60-s-Timeout neu. | gesichert |
| FR-BLK-03 | Richtungsblinker: maximale Blinkdauer 60 s → automatische Selbstabschaltung. Keine Reaktivierungssperre. | gesichert |
| FR-BLK-04 | Warnblinker durch Langdruck (≥ 5 s) einer beliebigen Taste; beide Seiten blinken; kein Timeout. | gesichert |
| FR-BLK-05 | Warnblinker endet durch beliebigen einzelnen kurzen Tastendruck → AUS. Der abschaltende Druck wird verbraucht und startet keinen Richtungsblinker. | gesichert |
| FR-BLK-06 | Links/Rechts als Richtung gegenseitig verriegelt; WARN einziger Zustand mit beidseitigem Blinken. | gesichert |
| FR-BLK-07 | Kurz-/Langdruck-Diskriminierung: Loslassen < 5 s = Kurzdruck; Halten ≥ 5 s = Warnblinker. | gesichert |
| FR-BLK-09 | RF-Blinkerbefehle werden erst ab RUN wirksam; während INIT verworfen. | gesichert |
| FR-SNS-01 | Ab RUN werden GNSS, BMP280, MPU6050 zyklisch gesampelt — unabhängig vom BLE-Zustand. | gesichert |
| FR-TEL-01 | Bei BLE-Verbindung Telemetrie-Stream; ohne Verbindung RAM-Ringpuffer (FR-TEL-04). | gesichert |

### 2.7 Funktionale Detaillogik — Block C

| ID | Anforderung | Status |
|---|---|---|
| FR-TL-06 | Bremslicht-Kennlinie: Schlusslicht-Grundhelligkeit ~20 % PWM. Stetig-linearer Anstieg von 2,0 m/s² bis Sättigung 5,0 m/s² (100 %). Ausschalthysterese: Rückfall unter ~1,5 m/s²; Mindesthaltezeit 300 ms. Anstieg schnell, Rückfall kurzer Fade. Eingang: gravitationskompensierte Verzögerung (Y-Achse, α=0,98). Norm-Anker ECE R50 (§ 67 Abs. 4). Schwellwerte feldzukalibrieren [Annahme]. | gesichert |
| FR-TL-07 | Notbrems-Blinken (ESS-Konzept), Zustand des roten Kanals: aktiviert ab ≥ 5,0 m/s², deaktiviert bei < 3,0 m/s² (Hysterese). ~4 Hz, Modulation 100 % ↔ Schlusslicht-Grundniveau (nie 0 %). **Experimentalfunktion, standardmäßig DEAKTIVIERT — nicht konform mit § 67 Abs. 4 StVZO.** | gesichert (experimentell) |
| FR-BLK-08 | Blinkfrequenz 1,5 Hz (ECE R6: 1,5 Hz ± 0,5), Duty 50 %, Hellzeit > 0,3 s. | gesichert |
| CON-02 | PWM-Trägerfrequenz aller LED-Kanäle 5 kHz. | gesichert |
| FR-RF-01 | Kontinuierliche Überwachung GPIO4 (RCSwitch); nur bekannte Codes (T1=10967538, T2=10967537), sonst ignoriert. | gesichert |
| FR-RF-02 | Druck erst nach ≥ 2 identischen Empfängen gültig (Entprellung/EMV). | gesichert |
| FR-RF-03 | Halte-Erkennung; „losgelassen" nach Empfangslücke > Release-Timeout (~150 ms, final nach Verifikationstest). | gesichert |
| FR-RF-04 | Kurzdruck (< 5 s) → SHORT; Halten ≥ 5 s → LONG (Warnblinker). | gesichert |
| FR-RF-05 | Reaktionszeit erkanntes Ereignis → LED < 100 ms. | gesichert |
| FR-RF-06 | RF-Codes fest im Code; Anlern-/Pairing-Modus → Future-Work. | gesichert |
| FR-SNS-02 | Sampling: IMU 100 Hz, BMP280 10 Hz, GNSS 1 Hz (ab RUN, unabhängig von BLE). | gesichert |
| FR-TEL-02 | Telemetrie-Frame 10 Hz, frischeste Werte + Status. | gesichert |
| FR-TEL-03 | Frame-Inhalt: IMU (6 Achsen + Verzögerung), BMP (Druck, Temp), GNSS (Breite, Länge, Speed, Heading, Höhe, Sats, HDOP, UTC), Status. Kein Akkustand. | gesichert |
| FR-TEL-04 | Ohne BLE Pufferung im RAM-Ringpuffer; Überlauf überschreibt Ältestes (Dimensionierung NFR-RES-01). | gesichert |

### 2.8 Normative Grundlagen der Lichtfunktionen [recherchiert 07/2026]

| Norm | Gegenstand | Bezug im Projekt |
|---|---|---|
| § 67 Abs. 3 StVZO | Scheinwerfer: Blinken unzulässig | – |
| § 67 Abs. 4 StVZO | Rote Schlussleuchte (kein Blinken); Bremslichtfunktion zulässig (ECE R50) | FR-TL-01/04/06 konform; **FR-TL-07 Konflikt** |
| § 67 Abs. 5 StVZO | Fahrtrichtungsanzeiger zulässig (ECE R50/R148, Anbau R74), gelb/amber | FR-BLK-* |
| ECE R6 | Blinkfrequenz 1,5 Hz ± 0,5, Hellzeit > 0,3 s | FR-BLK-08 |
| ECE R50 | Schluss-/Bremslichtfunktion | FR-TL-06 |
| ECE R48 (nur Kfz) | Emergency Stop Signal — für Fahrräder nicht anwendbar | FR-TL-07 (Konzeptanker) |

Hinweis: Sekundärquellen; für die Thesis am Primärtext (§ 67) gegenprüfen. Keine Rechtsberatung.

### 2.9 Fehlerbehandlung & Sicherheit — Block E

| ID | Anforderung | Status |
|---|---|---|
| FR-SAF-01 | Fail-safe-Leitprinzip: Bei jedem erkennbaren Fehler bleibt das rote Schlusslicht an (§ 67, höchste Priorität). Ausnahmen: Watchdog-Reset, experimentelles Notbrems-Blinken. | gesichert |
| FR-SAF-02 | Funktionspriorität: Schlusslicht > Bremslicht > Blinker > Telemetrie. | gesichert |
| FR-SAF-03 | Task-Watchdog (~2 s), Reset bei Hang; Reset-Ursache als Diagnose-Flag. | gesichert |
| FR-SAF-04 | BLE-/Telemetrie-Fehler dürfen Licht/Blinker nicht beeinträchtigen. | gesichert |
| FR-STA-04 | Laufzeit-IMU-Ausfall → sicheres Schlusslicht, Fehler-Flag, Hintergrund-Reinit. | gesichert |
| FR-STA-05 | Ausfall optionaler Sensoren → Weiterbetrieb, Telemetriefelder ungültig markiert. | gesichert |
| FR-STA-06 | Kein harter FAULT im MVP; degradierter RUN mit Fehler-Flags. `S_FAULT` reserviert. | gesichert |
| FR-SNS-03 | I²C-Zugriffe zeitbegrenzt (~25–50 ms), nicht-blockierend. | gesichert |
| FR-SNS-04 | Gestufte, nicht-blockierende I²C-Recovery (Re-Init → SCL-Clock → nach ~3 Versuchen als ausgefallen markieren). | gesichert |
| FR-SNS-05 | Leichte Plausibilitätsprüfung (Wertebereich + Eingefroren-Erkennung). | gesichert |
| FR-TEL-05 | GNSS-Status NO_DATA/NO_FIX/FIX_OK; Fix gültig wenn isValid & Alter < 3 s & Sats ≥ 4. | gesichert |

### 2.10 Nichtfunktionale Anforderungen — Block F

| ID | Anforderung | Status |
|---|---|---|
| NFR-RT-01 | Bremslicht-Reaktionszeit ≤ 50 ms (Ereignis → LED); messtechnisch zu verifizieren. | gesichert |
| NFR-RT-02 | Blinktakt 1,5 Hz timer-basiert; Periodentoleranz ± 5 %. | gesichert |
| NFR-RT-03 | Kooperativer, nicht-blockierender Scheduler (`millis()`-Tasks, feste Reihenfolge); IMU/Bremslicht 100 Hz. Keine eigenen FreeRTOS-Application-Tasks im MVP. | gesichert |
| NFR-RT-04 | Worst-Case-Loop-Durchlauf < 10 ms. | gesichert |
| NFR-RES-01 | RAM-Ringpuffer ~60 s @ 10 Hz (≈ 600 Frames), statisch vorreserviert. | gesichert |
| NFR-RES-02 | Keine dynamische Speicherallokation im Betrieb (Heap-Fragmentierung vermeiden). | gesichert |
| NFR-RES-03 | RAM-/CPU-Auslastung gemessen und dokumentiert. | gesichert |
| CON-03 | Flash-Partition „No OTA / große App"; OTA → Future-Work. Konfig über NVS/`Preferences`; kein Dateisystem im MVP. | gesichert |
| NFR-PWR-01 | WiFi aus (nur BLE); kein Deep/Light-Sleep im MVP; optional `yield`/`delay(1)`. | gesichert |
| NFR-PWR-02 | Zielaufzeit ~13 h (Schätzung), messtechnisch zu verifizieren; Messplan definierter Lastfälle inkl. MT3608-η. Keine harte Mindestlaufzeit gefordert. | gesichert |

### 2.11 Konfigurierbarkeit — Block G

| ID | Anforderung | Status |
|---|---|---|
| FR-CFG-01 | **Parametrierbar (NVS):** Bremsschwellen (2,0/5,0/3,0/1,5 m/s²), Mindesthaltezeit, GNSS-Fix-Kriterien (Alter, Sat-Anzahl), Aktivierungs-Flag FR-TL-07. **Fest im Code (strukturell/sicherheitsrelevant):** Pinbelegung, PWM-Frequenz (5 kHz), Blinkfrequenz (1,5 Hz, normgebunden), Timeouts (60 s, 5 s Init), RF-Codes, Sampling-/Telemetrie-Raten. | gesichert |
| FR-CFG-02 | Serielles Kalibrier-/Konfigurations-Interface über UART0 (Kommandos get/set/list/reset), nicht-blockierend; Werte in NVS persistiert. Entwickler-/Kalibrierwerkzeug, kein Endnutzer-Feature. | gesichert |
| FR-CFG-03 | Bei leerem/fehlendem NVS Compile-Zeit-Defaults; `config_version`-Schlüssel für Schema-Migration bzw. Reset auf Defaults nach Firmware-Update. | gesichert |

### 2.12 Testbarkeit & Erweiterbarkeit — Block H

| ID | Anforderung | Status |
|---|---|---|
| NFR-TST-01 | Strikte Trennung reine Logik ↔ Hardware-Treiber (Hardware-Abstraktion); Logik host-seitig ohne ESP32/Fahrrad testbar. | gesichert |
| NFR-TST-02 | Testdaten-Einspeisung (aufgezeichnete/synthetische Sensordaten) als schlanker Hook für reproduzierbare Logik-Tests; tiefergehende Simulation → Future-Work. | gesichert |
| NFR-TST-03 | Zwei Test-Ebenen: (a) Host-Unit-Tests der Logik (Unity/PlatformIO `native` oder GoogleTest, CI); (b) On-Target-Validierung hardwareabhängiger Teile (Kap. 9). | gesichert |
| NFR-EXT-01 | Modulare Struktur mit klaren Schnittstellen je Subsystem/Region; neue Sensoren/Telemetriefelder ergänzbar ohne Bruch bestehender Module. | gesichert |
| FR-TEL-06 | Telemetrie-Frame trägt eine Schema-/Versionskennung, damit Firmware und App unabhängig weiterentwickelbar sind (App erkennt das Frame-Format). | gesichert |

---

## 3. Gesamtsystem

### 3.1 Architektur (Variante 2 — verteilte Berechnung)
```
[Sensorik]                [ESP32 Firmware]                 [Web-App / PWA]
 GNSS L86  ─UART2─┐        ┌───────────────────────┐       ┌────────────────────┐
 BMP280    ─I²C──┼──────▶ │ Erfassung + Echtzeit-  │──BLE─▶│ Statistik, Fusion, │
 MPU6050   ─I²C──┘        │ logik (Bremse/Blinker) │ (uni) │ Speicherung, UI    │
 433-MHz-RX ─GPIO4──────▶ │ + Telemetrie-Stream    │       └────────────────────┘
                          └─────────┬──────────────┘
                                    │ PWM (GPIO25/26/27)
                            [Aktorik: rote + gelbe LEDs über IRLZ44N]
```

### 3.2 Datenflüsse
Sensor-Rohdaten → Firmware (Sampling FR-SNS-02) → (a) echtzeitkritische Ableitung Bremse/Blinker → LED; (b) Telemetrie-Serialisierung (10 Hz, versioniert) → BLE-Notify → App. Steuerrichtung App→ESP32 existiert nicht (FR-SYS-04).

### 3.3 Schnittstellen
I²C (Sensoren), UART2 (GNSS), UART0 (Debug/Konfig), digitaler GPIO-Eingang (RF), PWM-Ausgänge (LEDs, 5 kHz), BLE (Telemetrie → App).

---

## 4. Hardware

### 4.1 Bauteilübersicht
| Ref. | Bauteil | Hersteller/Typ | Funktion | Schnittstelle | Status |
|---|---|---|---|---|---|
| U3 | ESP32 NodeMCU DevKit C V2 | AZ-Delivery | Hauptrechner | 3,3 V GPIO | in Betrieb |
| IC2 | MPU-6050 (GY-521) | AZ-Delivery | IMU | I²C 0x68 | validiert |
| IC3 | BMP280 | AZ-Delivery | Barometer | I²C 0x76 | validiert |
| IC4 | Quectel L86 | Quectel | GNSS | UART2, 9600 Bd | teilvalidiert (Fix offen) |
| U4 | SRX882S V2.0 | – | 433-MHz-Empfänger | Digital, GPIO4 | validiert |
| – | QIACHIP-Fernbedienung (2 Tasten) | QIACHIP | Blinkerauslösung | 433 MHz ASK | validiert (10967538 / 10967537) |
| Q1–Q3 | IRLZ44N (3×) | Int. Rectifier | LED-PWM-Treiber | Gate an GPIO | GPIO-Pegel validiert, LED-Last offen |
| U1 | TP4056 Typ-C | – | LiPo-Laderegler 1 A (DW01) | USB-C | unter Last unverifiziert |
| U2 | MT3608 Step-Up | AZ-Delivery | 3,7→5 V (η ≤ 97 %) | Trimmer 5,00 V | unter Last nicht abgeglichen |
| BT1 | LiPo-Akku **LP103454**, 3,7 V, 2000 mAh | LP103454 (10,3 × 34 × 54 mm) | Energiespeicher | 3,7–4,2 V | verbaut, Kapazität gesichert |
| SW1 | Rastender Drucktaster IP65, 8 mm | – | Ein/Aus | – | verbaut; fehlt in Schaltplan/BOM |
| D1 | Rote LED | Vrabocry | Schluss- + Bremslicht (GPIO26) | PWM über Q2 | Kanalzuordnung/Datenblatt offen |
| D2/D3 | Gelbe LEDs | Vrabocry | Blinker L/R | PWM über Q1/Q3 | s. Kap. 11 |
| ANT1/ANT2 | GNSS-/433-MHz-Antenne | Namvo / Draht 17,3 cm | Empfang | – | dokumentiert |

**D1 (rote LED):** kombiniertes Schluss- und Bremslicht auf einem PWM-Kanal (GPIO26); Notbrems-Blinken ist ein Zustand *dieses* Kanals.

### 4.2 Pinbelegung [gesichert]
| Signal | GPIO | Zielgerät | Bewertung |
|---|---|---|---|
| SDA | GPIO21 | BMP280 + MPU6050 | gesichert |
| SCL | GPIO22 | BMP280 + MPU6050 | gesichert |
| UART2 RX | GPIO16 | L86 TXD1 | gesichert |
| UART2 TX | GPIO17 | L86 RXD1 | gesichert |
| RF DATA | GPIO4 | SRX882S | gesichert (Schaltplan GPIO34 veraltet) |
| Blinker links | GPIO25 | Q1 Gate | gesichert (Schaltplan fehlerhaft) |
| Bremslicht (PWM) | GPIO26 | Q2 Gate | gesichert (Schaltplan fehlerhaft) |
| Blinker rechts | GPIO27 | Q3 Gate | gesichert |
| Gate-Widerstand | 100 Ω | alle 3 Gates | gesichert |
| Gate-Pull-Down | 10 kΩ → GND | alle 3 Gates | real verbaut; fehlt in Schaltplan/BOM |

Hinweis: GPIO4 durch RF belegt → MPU6050-INT-Pin ungenutzt (Polling statt Interrupt).

---

## 5. Elektronik & Stromversorgung

### 5.1 Topologie
```
USB-C 5V → TP4056 (1A Ladung) → LiPo LP103454, 3,7–4,2V, 2000 mAh
                                   ↓ (über OUT+, NICHT B+)
                            MT3608 Step-Up (Trimmer auf 5,00V)
                                   ↓ (über Einschalter SW1)
                            ESP32 Vin → onboard AMS1117-3.3 → 3,3V Sensorik
```

### 5.2 Energiebilanz [Annahme — keine Messwerte]
Akku: LiPo **LP103454**, 3,7 V nominal, **2000 mAh** (gesichert).

| Verbraucher | Strom (angenommen) |
|---|---|
| ESP32 aktiv (WiFi aus) | ~80 mA |
| MPU6050 | ~3,5 mA |
| BMP280 | ~0,5 mA |
| SRX882S | ~3,5 mA |
| GPS L86 (Tracking) | ~26 mA |
| LED Rücklicht | ~50 mA |
| **Summe** | **~163 mA** |
| Rechnerische Laufzeit @ 2000 mAh | **~13 h** |

Berücksichtigt weder MT3608-Wirkungsgrad noch Ruheströme noch Lastspitzen. Messtechnische Verifikation erforderlich (Messplan NFR-PWR-02).

### 5.3 Hinweis zur Zustandsüberwachung
Keine Batteriespannungsmessung (OUT-01). Ladezustand nur über USB-C-/TP4056-Modul. Konsequenz: keine Low-Battery-Warnung durch die Firmware.

---

## 6. Firmware

### 6.6 Zustandsmodell (vier parallele Regionen) [Block B/C/D]

Statechart mit vier orthogonalen Regionen (nach Harel). Additive statt multiplikative Zustandsanzahl, je Region testbar. Ausführung über kooperativen Scheduler (NFR-RT-03).

| Region | Zustände | Treiber |
|---|---|---|
| R1 Lebenszyklus (STA) | `S_INIT`, `S_RUN`, `S_FAULT` (reserviert) | Power-On, Sensor-Init |
| R2 Rücklicht (TL) | `TL_INIT_BLINK`, `TL_SCHLUSSLICHT`, `TL_BREMSLICHT`, `TL_NOTBREMS_BLINKEN` (experimentell) | IMU-Verzögerung |
| R3 Blinker (BLK) | `BLK_AUS`, `BLK_LINKS`, `BLK_RECHTS`, `BLK_WARN` | 433-MHz-Fernbedienung |
| R4 Erfassung (SNS/TEL) | `SNS_ACQ` (ab RUN) + Telemetrie je nach BLE | zyklischer Timer |

**R2 — zwei Ebenen:** Innerhalb des roten Kanals gegenseitig ausschließend; `TL_NOTBREMS_BLINKEN` hat Vorrang vor `TL_BREMSLICHT` (ab |a| ≥ 5,0). Roter Kanal (R2) und gelbe Blinker (R3) auf getrennten LEDs, unabhängig.

**R2 Übergänge:** INIT → `TL_INIT_BLINK` (2 Hz). RUN → `TL_SCHLUSSLICHT` (~20 %). `→ TL_BREMSLICHT` bei |a| ≥ 2,0; linear bis 100 % bei 5,0. `→ TL_SCHLUSSLICHT` bei |a| < 1,5 (300 ms). `→ TL_NOTBREMS_BLINKEN` bei |a| ≥ 5,0 (nur experimentell); zurück bei |a| < 3,0.

**R3:** s. FR-BLK-01…09; Blinktakt 1,5 Hz, 50 % Duty. RF-Befehle erst ab RUN.

**RF-Tastenerkennung (Sub-FSM vor R3, FR-RF-03/04):**
```
IDLE ──Code (≥2×)──▶ GEDRÜCKT ──(Lücke > ~150 ms)──▶ Loslassen
                        │                                └─ < 5 s: SHORT-Event
                        └──(gehalten ≥ 5 s)──▶ LONG-Event (Warnblinker), warten auf Loslassen
```

### 6.7 Fehlerbehandlung & Sicherheit [Block E]
Leitprinzip (FR-SAF-01/02): Fail-safe auf Schlusslicht; Prioritätsordnung Schlusslicht > Bremslicht > Blinker > Telemetrie. IMU-Ausfall → sicheres Schlusslicht + Flag + Reinit; optionale Sensoren → Weiterbetrieb + Flag. I²C zeitbegrenzt, gestufte Recovery. GNSS-Status-Feld. Watchdog ~2 s. BLE-Isolation.

### 6.8 Ausführungsmodell [Block F]
Kooperativer, nicht-blockierender Scheduler: schnelle `loop()`, `millis()`-getaktete Tasks in fester Reihenfolge (sicherheitsrelevante zuerst). Keine `delay()`-Blockaden; keine eigenen FreeRTOS-App-Tasks im MVP. Worst-Case-Zyklus < 10 ms. Raster: IMU/Bremslicht 100 Hz, BMP 10 Hz, GNSS 1 Hz, Blinker 1,5 Hz, Telemetrie 10 Hz, Watchdog je Durchlauf.

### 6.9 Konfiguration, Test & Erweiterbarkeit [Block G/H]
Konfiguration: Kalibrierwerte in NVS, Struktur-/Sicherheitswerte fest im Code; serielles Kalibrier-Interface (UART0); `config_version`. Testbarkeit: Trennung Logik ↔ Hardware, Host-Unit-Tests + On-Target-Validierung. Erweiterbarkeit: modulare Schnittstellen, versioniertes Telemetrie-Frame.

*(Vollständige Zustandsdiagramme und Detailtabellen s. kanonische Bible im claude.ai-Projekt.)*

---

## 9. Validierung (Auszug offener Punkte)
Offen/Messung: RF-Kurz-/Langdruck-Timing (FR-RF-03/04), Bremskennlinie-Feldkalibrierung, Reaktionszeit ≤ 50 ms, Loop-Zeit/RAM-CPU, Energie/Laufzeit, I²C-Recovery, Watchdog-Reset, Brown-Out, MOSFET mit realer LED-Last, GPS-Fix, Host-Unit-Tests.

---

## 10. Entwicklungsentscheidungen
Siehe `docs/decision_log.md` (Arbeitskopie) bzw. kanonische Bible Kap. 10.

---

## 11. Offene Punkte
Siehe `docs/open_issues.md` (Arbeitskopie) bzw. kanonische Bible Kap. 11.

---

## 12. Risiken
Siehe kanonische Bible Kap. 12.
