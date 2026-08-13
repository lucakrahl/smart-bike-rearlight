# Firmware — Thesis Transfer Package

**Smart Bike Rear Light · Bachelorarbeit Krahl (B.Eng. Maschinenbau & Produktentwicklung, HS Düsseldorf)**
**Erstellt 10.08.2026 · Wissensbasis für den Bachelorarbeits-Schreibchat · noch keine fertigen Kapiteltexte**
**Bezugsstand: Firmware-Abschluss, Commit `835c7b3`, geflasht und geprüft**

> Zweck: verwertbare, belegte Informationsbasis für das Firmware-Kapitel (Kap. 6) und die firmwarebezogenen Anteile der Kapitel 3, 4, 9 und 10, entlang der Kette **Anforderung → Konzeption → Architekturentscheidung → Implementierung → Problem/Iteration → Lösung → Test/Validierung → Ergebnis → Bewertung**. Teil 1 (Dokumenten-Statusübersicht) liegt separat vor (`Thesis_Transfer_Firmware_Dokumentenstatus.md`).
>
> **Belegprinzip:** Als *gesichert* geführt sind Größen, die direkt aus Messdaten oder Quelltext folgen; als *[Annahme]* gekennzeichnete Aussagen sind begründete Vermutungen. Es wurden keine Werte erfunden. Wo eine Zahl fehlt, steht das ausdrücklich.

---

## 1. Dokumentenstatus (Kurzfassung)

Alle firmwarerelevanten Dokumente sind auf den Stand des Abschluss-Commits `835c7b3` gebracht: Project Bible v0.19, Decision Log, Open Issues, Current Context, Lessons Learned und Roadmap wurden in dieser Session aktualisiert; die beiden Validierungsberichte (Feldtest 06.08., Messfahrt 08.08.) haben je einen Nachtrag erhalten und blieben im Kern unverändert, weil sie die historische Beweisgrundlage bilden. Sechs kritische Befunde sind in Teil 1 einzeln aufgeführt; zwei davon (`measurement_log.md`: nicht belegbarer Firmware-Hash, zu weit gefasste Signalpfad-Aussage) müssen vor der Übernahme in Anhang C entschieden werden.

---

## 2. Finaler Stand der Firmware (implementiert / abgegrenzt / verworfen / offen)

**Zielplattform.** Espressif **ESP32-DevKitC-32E** (Modul WROOM-32E, Xtensa LX6 Dual-Core, 240 MHz, 520 kB SRAM, 4 MB Flash) mit onboard AMS1117-3.3. Das ursprünglich verwendete AZ-Delivery NodeMCU Dev Kit C V2 wurde am 01.08.2026 ersetzt (Begründung s. Punkt 12, Problem P2). Toolchain: PlatformIO mit der Plattform **pioarduino** → Arduino-ESP32-Core **3.3.11** (ESP-IDF 5.5.x), Board `esp32dev`, Partitionstabelle `huge_app.csv` (kein OTA). Drei Build-Environments: `esp32dev` (Ziel), `esp32dev_bench` (`-D BENCH_MODE`), `native` (Host-Unit-Tests, Unity).

**Implementiert und verifiziert:**

- **Sensorerfassung.** MPU-6050 (I²C 0x68) mit fest gesetztem Digital-Tiefpass `DLPF_CFG = 3` (44 Hz) und `SMPLRT_DIV = 4` (200 Hz Sensorrate), Messbereich ±16 g, Auslesung 100 Hz, WHO_AM_I-Liveness-Prüfung statt Auswertung des Bibliotheks-Rückgabewerts. BMP280 (I²C 0x76) im FORCED-Mode nach dem Weather-Monitoring-Profil (×1/×1, IIR aus), Trigger und Lesen auf zwei Zyklen entkoppelt (~1 Hz). Zentrale I²C-Bus-Initialisierung auf Anwendungsebene, SDA GPIO21 / SCL GPIO22.
- **Einbaulage-Rückabbildung.** Die 180°-Drehung der Lochrasterplatine in ihrer eigenen Ebene wird an der Treibergrenze zurückgerechnet: `lib/logic/imu_mount_orientation.h`, `IMU_MOUNT_SIGN_X/Y/Z` = −1/−1/+1, angewandt auf Beschleunigung **und** Drehrate. Determinante +1, das Koordinatensystem bleibt rechtshändig.
- **GNSS.** Quectel L86-M33 an UART2 (RX GPIO16 ← TXD1, TX GPIO17 → RXD1), 9600 Bd, TinyGPSPlus, nicht-blockierendes `gnssPump()` in jedem Schleifendurchlauf, Fix-Status-Gating (`NO_DATA` / `NO_FIX` / `FIX_OK`) in `gnss_fix`. Interne Patch-Antenne (EX_ANT offen).
- **433-MHz-Empfang.** SRX882S (Superheterodyn, −114 dBm) an GPIO4, `rc-switch`, zustandsloser Treiber; Tastenerkennung (Entprellung, Halte-/Loslass-Erkennung, Kurz-/Langdruck) in `button_decoder`. **Ein 433-MHz-Sendemodul existiert nicht** — Sender ist ausschließlich die kommerzielle QIACHIP-Fernbedienung. Das ist für Kap. 5.5 und 6.4 relevant: Das System ist funkseitig reiner Empfänger.
- **Aktorik.** Drei Low-Side-Schaltstufen mit IRLZ44N: rote 3-W-COB (Schluss-/Bremslicht) an GPIO26, gelbe COB links an GPIO25, rechts an GPIO27. PWM ausschließlich über `ledcAttach()`/`ledcWrite()` (Core-3.x-API), 5 kHz, 8 Bit Auflösung.
- **Bremserkennung (Stufe 1).** `motion_filter` mit Normbetrags-Gate (drei Regime STATIC / DYNAMIC / SHOCK), Komplementärfilter mit dt-parametrierten Blendfaktoren (τ = 3,0 s; τ_slow = 90 s kalibriert, 30 s Rückfall), Nickwinkel-Verankerung über `atan2f` in einem 0,3-s-STATIC-Fenster, entkoppelte kumulative Gyro-Bias-Kalibrierung über 200 STATIC-Abtastungen, Nachglättung (3-Punkt-Median + 15-Hz-Tiefpass), einseitige Begrenzung des Ausgangs auf positive Werte.
- **Bremslicht.** Zustandsloser Proportionalteil `brake_curve` (20 % unter 2,0 m/s², linear bis 100 % bei 5,0 m/s²) plus Zustandsautomat `tail_light_fsm` (Init-Blinken, Schlusslicht, Bremslicht, ESS-Blinken) mit Ausschalthysterese bei 1,5 m/s² und 300 ms Mindesthaltezeit.
- **Blinker.** `blinker_fsm` mit den Zuständen AUS / LINKS / RECHTS / WARN, 1,5 Hz, gegatet gegen den Systemzustand.
- **Lebenszyklus.** `lifecycle_fsm` INIT → RUN mit Sensor-Init-Timeout und degradiertem RUN; Init-Diagnose-Blinken.
- **Robustheit.** `imu_health` mit Wertebereichs-, Frozen- und Sprung-Plausibilität, gestufter Recovery (Soft-Reinit, SCL-Clock-Release über rohe ESP-IDF-`gpio_*`-Aufrufe) und Eskalations-Vertrauen über drei aufeinanderfolgende plausible Zyklen. Task-Watchdog ~2 s mit automatischem Reset und Boot-Diagnose über `esp_reset_reason()`. Fail-Safe auf Schlusslicht.
- **Telemetrie und BLE.** Versioniertes Frame **Schema v3, 113 Byte**, 10 Hz, unidirektional über NimBLE-Notify; RAM-Ringpuffer für Nachlieferung nach Verbindungsabbruch (~60 s bei 10 Hz); Fensteraggregate des 100-Hz-Takts über 100 ms in `telemetry_window_agg`; GNSS-Referenzbeschleunigung `gnss_speed_ref` rein beobachtend.
- **Auslieferungsstand.** Keine Debug-Ausgaben, `DEBUG_SERIAL = false`, keine `TODO`- oder `FIXME`-Marker im eigenen Quellcode, Bibliotheksversionen gepinnt.

**Kennzahlen des Abschlussstands:** Host-Unit-Tests **126/126** grün · Flash **674 487 B (21,4 %)** · RAM **106 912 B (32,6 %)** · beide Firmware-Builds fehlerfrei.

**Bewusst abgegrenzt (Umfangsschnitt 10.08.2026, Bible Kap. 12.2):** FR-CFG-02 (serielles Kalibrier-Interface über UART0) und FR-CFG-03 (Konfiguration aus NVS mit `config_version`) sind spezifiziert, aber **nicht implementiert**. Alle Kalibrierwerte sind Übersetzungszeit-Konstanten; eine Änderung erfordert Neuübersetzung. Ebenfalls abgegrenzt: Feldverifikation der IMU-Plausibilitätsschwellen und der BLE-Parameter, Messung des RF-Wiederholintervalls, I²C-Recovery für den BMP280, Verifikation des SCL-Release am real hängenden Bus.

**Verworfen (firmwarerelevant):** GNSS als primäre Bremsquelle (Variante V-A, Datenblatt- und Integritätsgründe) · IMU-Signalkette unverändert lassen (V-C, nachgewiesene Funktionsunfähigkeit) · Tiefpass oder Median statt Normbetrags-Gate (Latenzkosten) · Konvergenzkriterium für die Bias-Kalibrierung (Variante B2, zwei weitere ungemessene Parameter) · NaN als Ungültigkeitsmarke im Frame · WiFi statt BLE als Transport · rein betragsbasierte SHOCK-Schwellen · `MOTION_BRAKE_SIGN` als Ort der Einbaulagekorrektur.

**Offen (nicht Firmware):** physische Blinker-L/R-Zuordnung, Befund B-1 (UART-Pegel GPIO17 → L86 RXD1, 3,3 V gegen V_IHmax = 3,1 V), Befund B-6 (Nennstrom SW1 bei 1,18 A Worst Case).

---

## 3. Firmware-Anforderungen (Nachweis-Matrix)

Alle IDs stammen aus der Project Bible Kap. 2 (SRS, Blöcke A–H). Es wurden keine neuen IDs erfunden. Die Tabelle führt nur die firmwarerelevanten Anforderungen; die vollständige SRS gehört nach der Gliederungsentscheidung in **Anhang B**.

| ID | Anforderung | Umsetzung | Nachweis | Thesis-Relevanz |
|---|---|---|---|---|
| FR-TL-01/04 | Rücklicht als Dauerlicht | Schlusslicht-Grundwert 20 % PWM in `tail_light_fsm` | Bench C: konstant 20 % über die gesamte Rampe | mittel |
| FR-TL-03 | Init-Diagnose-Blinken | Zustand `InitBlink`, Frequenz und Duty aus `config.h` | HW-validiert, Boot-Beobachtung | mittel |
| FR-TL-05/06 | Bremslicht mit Kennlinie, Hysterese, Mindesthaltezeit | `brake_curve` (zustandslos) + `tail_light_fsm` (Zustand) | Bench A: Ansprechen 2,004 m/s², `pct = 26,66·decel − 33,32`, **R² = 0,99984**; Feld: 194/194 Zeilen auf ±1 Prozentpunkt | **sehr hoch** |
| FR-TL-06 (Haltezeit) | 300 ms Mindesthaltezeit | Nach Behebung von M-01 friert der zuletzt oberhalb 2,0 m/s² erreichte Wert ein | Bench B: exakt 300 ms bei idealisiertem Sprung; Feld 08.08.: **0 von 14 wirksam** → Mangel; nach Korrektur Regressionstest grün + Beobachtung am Gerät | **sehr hoch** — Mangel und Behebung sind ein vollständiger Nachweiszyklus |
| FR-TL-07 | Notbrems-Blinken (ESS) | Zustand `EssBlink`, per `ESS_ENABLED_DEFAULT = false` deaktiviert | Quelltext; rechtliche Begründung § 67 Abs. 4 | hoch — Zielkonflikt für Kap. 10.2 |
| FR-BLK-01…09 | Blinker links/rechts/Warn, 1,5 Hz, Gating gegen Systemzustand | `blinker_fsm` + `button_decoder` | HW-validiert; Host-Tests | mittel |
| FR-RF-02/03/04 | Entprellung, Loslass-Erkennung, Kurz-/Langdruck | `button_decoder`, `RF_RELEASE_TIMEOUT_MS = 150` | HW-validiert; systematische Messung des Wiederholintervalls **abgegrenzt** | mittel |
| FR-SNS-02 | IMU-Abtastung 100 Hz | Scheduler-Slot `PERIOD_IMU_MS = 10` | dt-Statistik Bench: Mittel 10,000 ms, σ = 0,61…0,97 ms, Min 9 / Max 11 ms; Feld: `dt_max_ms` > 10 ms in 10,15 % (max. 13 ms) | hoch |
| FR-SNS-03 | I²C-Timeout | `Wire`-Timeout, Fehlerzähler `IMU_FAIL_LIMIT = 5` | Fehlerinjektion SDA-Kurzschluss | hoch |
| FR-SNS-04 | I²C-Recovery | Gestuft: Soft-Reinit → SCL-Clock-Release (9 Pulse, 5 µs/5 µs, manuelle STOP-Bedingung) über rohe `gpio_*`-Aufrufe | Fehlerinjektion am Board: Recovery wirksam | **sehr hoch** — Kap. 6.5 |
| FR-SNS-05 | Plausibilitätsprüfung | `imu_health`: Betragsband, Frozen-Zähler, Slew-Grenzen, Eskalations-Vertrauen | Fehlerinjektion: kein Fehl-Bremslicht durch ein einzelnes Müll-Sample | **sehr hoch** |
| FR-STA-01/02/06 | Lebenszyklus INIT → RUN, degradierter Betrieb | `lifecycle_fsm` | HW-validiert; Host-Tests | mittel |
| FR-STA-04 | Sensorausfall gated Bremslicht | Eigenes Signal `imu_health`, getrennt von `lifecycle_fsm.degraded` | Bench C: 20 % konstant bei `imu_health = FAILED` | hoch |
| FR-STA-05 | Optionale Sensoren degradieren nur | BMP280/GNSS-Ausfall setzt Flags, blockiert nicht | Quelltext | mittel |
| FR-SAF-01 | Fail-safe auf Schlusslicht | Prioritätsordnung Schlusslicht > Bremslicht > Blinker > Telemetrie | Bench C | hoch |
| FR-SAF-03 | Watchdog | Task-Watchdog ~2 s, `enableLoopWDT()`, Boot-Diagnose per `esp_reset_reason()` | `'H'`-Hang-Hook am Board: Auto-Reset nach ~2 s, Reset-Grund korrekt erkannt | hoch |
| FR-SYS-01 | Rohdaten liefern, Ableitungen in der App | Frame trägt `pressure_pa`, nicht die Höhe | Frame-Vertrag | hoch — Kap. 4.1 |
| FR-SYS-04 | Unidirektionale Telemetrie | BLE-Notify, kein Write-Characteristic | Quelltext, App-Seite | hoch |
| FR-TEL-02/03/06 | Versioniertes Frame | Schema v3, `version` an Offset 0, Offsets 0–80 byte-identisch zu v2 | `BLE_Frame_v3_Schnittstelle.md`; Golden-Vektor-Kreuztest | **sehr hoch** |
| FR-TEL-04 | Pufferung bei Verbindungsabbruch | RAM-Ringpuffer, Nachlieferung ~60 s @ 10 Hz | App-seitig verifiziert | hoch |
| FR-TEL-05 | GNSS-Fix-Status | `gnss_fix`, Kriterien `isValid` & Alter < 3 s & Sats ≥ 4 | Feldtest Fahrt 6: ~20 s korrekt `NO_FIX`; **Gegenbefund Fahrt 5** | **sehr hoch** |
| FR-CFG-01 | Parametrierbarkeit, keine Magic Numbers | Alle Werte als benannte `constexpr` in `config.h` | Quelltextprüfung | mittel |
| FR-CFG-02/03 | Serielles Interface, NVS | **nicht umgesetzt — abgegrenzt** | Bible Kap. 12.2 | hoch — Abgrenzung ist begründungspflichtig |
| NFR-RT-01 | Reaktionszeit ≤ 50 ms | 100-Hz-Takt, kein `delay()` im Betrieb | Bench B **≤ 10 ms**, D **≤ 10 ms**, F **20 ± 10 ms** | **sehr hoch** |
| NFR-RT-03 | Sicherheitskritisches zuerst | Feste Task-Reihenfolge in `loop()` | Quelltext | mittel |
| NFR-RT-04 | Schleifenzeit < 10 ms | Kooperativer Scheduler, `delay(1)` als CPU-Abgabe | Prüfstand **0,651 ms**; Fahrbetrieb **6,7 ms** Worst Case, 0,00 % der Fenster über 10 ms | **sehr hoch** |
| NFR-PWR-01 | Energieeffizienz | `delay(1)` gibt CPU ab; BMP280 im FORCED-Mode | Energiebilanz gerechnet, **nicht gemessen** | mittel — Lücke |
| NFR-RES-01 | Ressourcenbudget | statische Speicherverwaltung, kein `new`/`malloc` im Betrieb | Flash 21,4 %, RAM 32,6 % | hoch |
| NFR-TST-01 | Trennung Logik ↔ Hardware | `lib/logic` ohne `#include <Arduino.h>`, `lib/drivers` gekapselt | 13 Logikmodule host-getestet | **sehr hoch** |
| NFR-TST-02 | Testdaten-Einspeisung als Hook | `BENCH_MODE` ersetzt `setup()`/`loop()` | Bench-Läufe A–F | hoch |
| NFR-TST-03 | Zwei Testebenen | `native` (Unity, 126 Tests) + On-Target | Testlauf, Fehlerinjektion | **sehr hoch** |
| NFR-EXT-01 | Modularität, Reproduzierbarkeit | klare Modulschnittstellen, `lib_deps` versionsfest gepinnt | `platformio.ini` | hoch |

---

## 4. Softwarearchitektur (tatsächliche Umsetzung)

**Leitgedanke.** Die Architektur folgt zwei Trennlinien, die beide aus Anforderungen abgeleitet sind und nicht aus Geschmack: der Trennung **reine Logik ↔ Hardwaretreiber** (NFR-TST-01, damit die sicherheitsrelevante Logik ohne Zielhardware prüfbar ist) und der Trennung **schneller Regelpfad ↔ langsame Stützgröße** (Architekturentscheidung V-B nach dem Feldtest).

**Ausführungsmodell.** Ein **kooperativer, nicht-blockierender Scheduler** in `loop()`, kein FreeRTOS auf Anwendungsebene. `millis()`-getaktete Aufrufslots in fester Reihenfolge, sicherheitsrelevantes zuerst (NFR-RT-03): IMU und Bremslicht 100 Hz, RF und Blinker jeden Durchlauf, Barometer 10 Hz, GNSS 1 Hz, Telemetrie 10 Hz, abschließend `delay(1)` als bewusste CPU-Abgabe. Der Watchdog wird vom Arduino-Core automatisch vor jedem `loop()` gefüttert.

*Warum kein FreeRTOS.* Der Zeitbedarf pro Durchlauf liegt bei 92 µs im Median (Feld, ohne die 1-Hz-Fenster) gegenüber einem 10-ms-Raster — die CPU ist zu über 99 % unbeschäftigt. Ein Task-Scheduler hätte Nebenläufigkeit eingeführt, ohne ein Problem zu lösen, dafür aber Synchronisation, Prioritätsinversion und schwerer nachvollziehbares Zeitverhalten. Die Project Bible formuliert das als Regel: FreeRTOS nur bei nachweisbarem technischem Vorteil.

*Warum `t_imu = now` und nicht `t_imu += PERIOD_IMU_MS`.* Die rekursive Variante erzeugt nach einer Verzögerung Aufholbursts und verzerrt die Abtastabstände zusätzlich. Der gewählte Weg driftet stattdessen systematisch nach oben — die reale Periode ist 10 ms plus Restlaufzeit, der Nennwert 100 Hz wird strukturbedingt nie exakt erreicht. Zulässig ist das nur, weil der Komplementärfilter mit dem **real gemessenen** `dt_s` arbeitet und nicht mit dem Nennwert. Dieser Zusammenhang ist im Quelltext kommentiert und in der Arbeit erwähnenswert, weil er eine gängige Fehlannahme ausräumt.

| Schicht / Modul | Typ | Verantwortung |
|---|---|---|
| `src/main.cpp` | Anwendung | Scheduler, Task-Slots, Verdrahtung der Module, zentrale I²C-Init, `BENCH_MODE`-Harness |
| `include/config.h`, `pins.h` | Konfiguration | alle Konstanten und GPIO-Zuordnungen; einzige Parameterquelle |
| `lib/logic/motion_filter` | reine Logik | Regime-Klassifikation, Komplementärfilter, Bias-Schätzung, Längsverzögerung |
| `lib/logic/imu_mount_orientation` | reine Logik | Rückabbildung der Einbaulage an der Treibergrenze |
| `lib/logic/brake_curve` | reine Logik | zustandsloser Proportionalteil der Kennlinie |
| `lib/logic/tail_light_fsm` | reine Logik | Rücklicht-/Bremslicht-Zustandsautomat (R2), Hysterese, Mindesthaltezeit |
| `lib/logic/lifecycle_fsm` | reine Logik | Lebenszyklus INIT → RUN, degradiert (R1) |
| `lib/logic/blinker_fsm`, `button_decoder` | reine Logik | Blinker-Zustandsautomat (R3), Tastenerkennung |
| `lib/logic/imu_health` | reine Logik | Plausibilität, Recovery-Stufen, Fail-Safe-Gate |
| `lib/logic/telemetry_frame`, `telemetry_buffer`, `telemetry_window_agg` | reine Logik | Serialisierung, Ringpuffer, Fensteraggregate |
| `lib/logic/gnss_fix`, `gnss_speed_ref` | reine Logik | Fix-Bewertung, GNSS-Referenzbeschleunigung (beobachtend) |
| `lib/drivers/imu_driver`, `bmp280_driver`, `gnss_driver`, `rf_input`, `led_output`, `ble_telemetry` | Treiber | Hardwarezugriff, gekapselt; hier und nur hier liegen Arduino-/ESP-IDF-Abhängigkeiten |

**Vier orthogonale Zustandsregionen (Harel).** R1 Lebenszyklus · R2 Rücklicht/Bremslicht · R3 Blinker · R4 Sensorik/Telemetrie. *Warum orthogonal und nicht als eine kombinierte Zustandsmaschine:* Die Zustandsanzahl wächst dadurch additiv statt multiplikativ, jede Region ist einzeln host-testbar, und die Kopplung beschränkt sich auf zwei explizite Signale (`SystemState` und das getrennte `imu_health`-Gate). Eine flache FSM hätte für dieselbe Funktionalität ein Vielfaches an Zuständen gebraucht.

**Mögliche Abbildungen:** Blockdiagramm der Modulschichten mit der Logik-/Treiber-Trennlinie · Sequenzdiagramm eines `loop()`-Durchlaufs mit den Task-Slots und ihren Perioden · Harel-Diagramm der vier Regionen · Datenflussdiagramm IMU → `motion_filter` → `brake_curve` → `tail_light_fsm` → PWM, mit dem parallelen Zweig in die Telemetrie.

---

## 5. Sensoranbindung und Treiberschicht

**I²C-Bus.** Zentrale Initialisierung mit einem einzigen `Wire.begin()` in `setup()` statt in den Sensortreibern. *Begründung:* Modularität und Unabhängigkeit der optionalen Sensoren; kein Sensor darf die Busverfügbarkeit eines anderen von seiner eigenen Initialisierungsreihenfolge abhängig machen.

**MPU-6050.** Der Befund, der diese Konfiguration erzwang, ist thesisrelevant: Aus den Registern zurückgelesen ergab sich `DLPF_CFG = 0` (260 Hz Bandbreite) bei `SMPLRT_DIV = 0` (8 kHz interne Rate) — die POR-Defaults, die `Adafruit_MPU6050::begin()` **nie explizit schreibt**. Die Firmware liest mit 100 Hz eine Momentaufnahme aus einem mit 8 kHz aktualisierten Register, ohne Dezimationsfilter und mit einer Bandbreite weit oberhalb der halben Abtastrate: **Unterabtastung ohne Antialiasing**. Gesetzt wurden `DLPF_CFG = 3` (44 Hz, unter der halben Auslesefrequenz) und `SMPLRT_DIV = 4` (200 Hz Sensorrate, vermeidet die Schwebung zwischen Sensor- und Auslesetakt, die sonst den scheinbaren Jerk verdoppelt — gerade die Größe, aus der die SHOCK-Schwelle bestimmt wird). Messwirkung: STATIC-Anteil im Ruhezustand von 68–73 % auf **85,7 %**, nach der vollständigen Reparaturrunde 90–100 %. Kosten: 4,9 ms Gruppenlaufzeit gegen NFR-RT-01 ≤ 50 ms. Messbereich ±16 g, weil Fahrbahnstöße bis etwa 20 g auftreten und ein engeres Fenster dort sättigen und die Plausibilitätsprüfung fälschlich ansprechen lassen würde.

**Liveness statt Rückgabewert.** `imuRead()` prüft WHO_AM_I, weil der Adafruit-Wrapper I²C-Fehler nicht zuverlässig über den Rückgabewert von `getEvent()` durchreicht. Das ist ein konkretes Beispiel dafür, dass eine Bibliotheksabstraktion eine sicherheitsrelevante Information verschluckt.

**BMP280.** FORCED-Mode statt NORMAL-Dauerbetrieb: geringeres Rauschen bei 1 Hz, geringere Stromaufnahme, Bosch-Empfehlung für Weather Monitoring, und vor allem weniger Selbsterwärmung — der Sensor sitzt im geschlossenen Gehäuse neben einer Leistungs-LED. Genau daraus folgt die Entscheidung, `temperature_c` **nicht** als Fahrtkennwert zu führen: Der Sensor misst die Eigenerwärmung, nicht die Umgebung. In der Firmware bleibt der Wert erhalten, weil die Druckkompensation ihn braucht.

**GNSS.** Der NMEA-Puffer wird in **jedem** Schleifendurchlauf gedraint (`gnssPump()`), nicht in einem eigenen Zeitslot. Bei 9600 Bd und über 100 Hz Schleifenfrequenz bleibt der Serial2-RX-Puffer damit sicher leer, ohne dass ein Überlauf droht. Die Fix-Verarbeitung selbst läuft im 1-Hz-Slot.

**433 MHz.** `rf_input` ist bewusst zustandslos (`available()` → Code → `resetAvailable()`); die gesamte Zeitlogik liegt in `button_decoder` und ist damit host-testbar. Die Blinker-FSM sieht ausschließlich fertige `ButtonEvent`s. Diese Dreiteilung ist ein sauberes Beispiel für die Logik-/Treiber-Trennung.

---

## 6. Bremserkennung — der Kernbereich der Arbeit

Dies ist der Teil, an dem die Kette Anforderung → Konzept → Falsifikation → Neuentwurf → Nachweis vollständig belegt ist. Er sollte den Schwerpunkt von Kap. 6.3 und Kap. 9.3 bilden.

**Ausgangsentwurf (bis 06.08.2026).** Komplementärfilter mit α = 0,98 bei 100 Hz, Schätzung der Nickstellung als gewichtete Mischung aus integrierter Drehrate und `atan2f(a_y, a_z)`, anschließend Subtraktion von `g·sin(pitch)` von `a_y`. Am Prüfstand mit eingespeistem Verzögerungssignal validiert — die vorgelagerte Kette Sensor → Filter → Schwerkraftkompensation war dabei ausdrücklich **nicht** geprüft.

**Falsifikation (Feldtest 06.08.2026).** Der Versuch war als Falsifikationsversuch angelegt und ist erfolgreich verlaufen. Zwei unabhängige Fehlermechanismen wurden belegt:

*Fehlermechanismus A — Scheinneigung (systematisch).* Eine anhaltende Längsverzögerung ist anhand von `atan2f` allein nicht von einer anhaltenden Neigung unterscheidbar. Mit α = 0,98 bei 100 Hz beträgt die Zeitkonstante τ = α·Δt/(1−α) = **0,49 s**; eine typische Bremsung von 20 km/h bis zum Stillstand mit 3 m/s² dauert **1,85 s**, also deutlich länger. Nach etwa einer Zeitkonstante konvergiert die Nickschätzung auf die falsche Neigung und der Filter rechnet die Bremsbeschleunigung als Schwerkraftkomponente weg. Der negative Überschwinger beim Lösen der Bremse erklärt zusätzlich das negative Vorzeichen der Korrelation.

*Fehlermechanismus B — Stoßdurchgriff (impulsiv).* Zwischen `a_y` und der Kennlinie wirkte keinerlei Stoßunterdrückung. Ein Schlagloch erzeugt Spitzen mehrerer g über 10–30 ms, bei 100 Hz also ein bis drei Abtastwerte — und die 300-ms-Mindesthaltezeit streckt einen 20-ms-Stoß zu einem 300 ms sichtbaren Bremslicht. Das Systemverhalten war damit exakt invers zur Anforderung.

*Direkter experimenteller Nachweis (Fahrt 4).* Im Stand, ohne jede Bewegung, erzeugte reines Neigen bei t = 3,0 s eine Duty von **42 %**. Damit ist Mechanismus A nicht nur hergeleitet, sondern isoliert gemessen.

*Abgrenzung.* Nicht der Sensor war die Ursache. Der MPU-6050 löst mit ±16 g Bereich 0,5 m/s² problemlos auf, liefert 100 Hz ohne nennenswerte Latenz, und kommerzielle Systeme (Busch & Müller, Trelock, Garmin Varia RTL) arbeiten ebenfalls rein beschleunigungsbasiert. Fehlerhaft war ausschließlich die Auswertevorschrift zwischen Rohdaten und Kennlinie.

**Prüfung der Alternative „GNSS primär" (Variante V-A).** Auf Datenblattebene beantwortet, ohne Versuch: Der L86 liefert maximal 10 Hz statt der geforderten 100 Hz; die Gesamtlatenz aus interner Lösungslatenz (100–300 ms) und einer Abtastperiode für die Differentiation beträgt 200–400 ms gegen NFR-RT-01 ≤ 50 ms, also Faktor 4 bis 8; bei 50 km/h eines folgenden Fahrzeugs entsprechen 300 ms **4,2 m** später einsetzender Warnwirkung. Hinzu kommen 20 s bis zum Erstfix nach dem Einschalten und ein Rauschen der differenzierten Geschwindigkeit mit einem 90-%-Quantil bis 1,97 m/s² gegen eine Ansprechschwelle von 2,0 m/s².

**Der entscheidende Integritätsbefund (Fahrt 5).** Bei abgedeckter Antenne meldete das Modul **73 km/h im Stand** — bei `FIX_OK`, 15 Satelliten und HDOP 0,7. Nach dem spezifizierten Gating (`isValid` & Alter < 3 s & Sats ≥ 4) ist das ein exzellenter Fix. Der Umschaltauslöser einer GNSS-primären Architektur („Fix verloren") tritt im gefährlichsten Fehlerfall also gerade nicht ein. Das ist der stärkste einzelne Befund gegen V-A und gehört als solcher in die Arbeit.

**Neuentwurf (Stufe 1, Normbetrags-Gate).** Der physikalische Diskriminator ist der Betrag des Beschleunigungsvektors: Bei reiner Neigung gilt ‖a‖ = g, bei Bremsung ‖a‖ = √(g² + a²) (bei 6 m/s² also 11,5 m/s², das sind g + 1,7), bei Kurvenfahrt mit 20° Schräglage g/cos(20°) = 10,4 m/s², bei einem Schlagloch 20–50 m/s². Daraus die drei Regime STATIC / DYNAMIC / SHOCK, wobei die Accelerometer-Korrektur außerhalb von STATIC unterdrückt wird — ein etabliertes Verfahren (Madgwick 2010; Mahony et al. 2008).

*Warum nicht Tiefpass oder Median.* Es gibt keine Frequenztrennung zwischen Bremsung und Stoß, an der ein Filter ansetzen könnte. Ein Tiefpass erster Ordnung mit f_c = 8 Hz hat τ = 20 ms und lässt einen 25-ms-Impuls immer noch zu **71 %** durch — bei dauerhaften Latenzkosten von 20 ms. Ein Median über fünf Abtastwerte kostet etwa 30 ms Gruppenlaufzeit. Das Normbetrags-Gate kostet **0 ms**, weil es eine Fallunterscheidung und keine Filterung ist.

*Warum die SHOCK-Erkennung über den Jerk läuft.* Die Rückrechnung ursprünglich betragsbasierter Schwellen ergab zwei Lücken: Bremsungen bis 2,64 m/s² blieben STATIC (genau der Bereich der im Feld verpassten Bremsungen), Bremsungen ab 7,44 m/s² wurden als SHOCK eingefroren. Über die Änderungsrate trennen sich die Fälle um zwei Größenordnungen — Bremsung 0,08–0,34 m/s² je 10 ms gegenüber Schlagloch etwa 20.

*Parametrierung der Zeitkonstanten.* `MOTION_COMPL_TAU_S` wurde von 0,49 s auf **3,0 s** angehoben. Der alte Wert stammte aus α = 0,98 bei 10 ms und war für dieses System nie begründet; seit ein explizites Anker-Fenster existiert, muss das laufende Filter die Startkonvergenz nicht mehr leisten und führt nur noch die Fahrbahnneigung nach. Sicherheitsprüfung: Ein Steigungssprung von 10 % entspricht 5,7° und erzeugt g·sin(5,7°) = 0,98 m/s², deutlich unter der Ansprechschwelle. `MOTION_COMPL_TAU_SLOW_S` ging von 30 s auf **90 s**, mit Rückfall auf 30 s solange `bias_calibrated = 0` — zulässig erst durch die Bias-Kalibrierung, denn der stationäre Fehler ε = b·τ beträgt bei 0,005 °/s und 90 s nur 0,45° = 0,077 m/s², bei unkalibrierten 0,5 °/s dagegen bis zu 45°.

**Die Reparatur, die den eigentlichen Unterschied machte.** Das ursprüngliche Verankerungsfenster verlangte 100 **zusammenhängende** STATIC-Abtastungen. Der STATIC-Anteil beträgt auf dem Schreibtisch 90–100 %, auf dem Rad im Stillstand aber nur **39,2 %** und in Fahrt **25,2 %**. Bei p = 0,392 liegt die Wahrscheinlichkeit für 100 zusammenhängende Treffer bei 0,392¹⁰⁰ ≈ 10⁻⁴¹ — die Kalibrierung wäre im Fahrbetrieb **nie** zustande gekommen, und das System hätte dauerhaft mit dem konservativen 30-s-Rückfall gelaufen. Die Entkopplung in ein kurzes Verankerungsfenster (0,3 s, kohärente Lagereferenz nötig) und eine kumulative Bias-Mittelung über 200 STATIC-Abtastungen ohne Zusammenhangsforderung (einem Mittelwert ist Nachbarschaft gleichgültig) war damit keine Optimierung, sondern die Voraussetzung dafür, dass das System überhaupt in seinen ausgelegten Betriebspunkt kommt. **Ein Schreibtischtest allein hätte diesen Nachweis nicht liefern können** — das ist eine der stärksten methodischen Aussagen der Arbeit.

**Nachweis (Messfahrt 08.08.2026, 10 Hz, Schema v3, 177,86 s).**

| Kenngröße | Wert |
|---|---|
| Erkannte Bremsvorgänge | **9 von 9** aus der GNSS-Referenz identifizierten |
| Korrelation, fester Versatz −2,0 s | **r = +0,85** (Vergleichsfahrten 06.08.: Median +0,15) |
| Korrelation, unabhängig geschätzter Versatz −1,6 s | r = +0,808, 95-%-KI +0,748…+0,855 |
| Fehlauslösungen bei Beschleunigung | **0 von 25** Epochen mit ≥ 1,0 m/s² |
| Ruhesockel im Stillstand (118 Abtastungen, 11,8 s) | Median **0,00**, P99 0,08 m/s² (vorher ≈ 3,0) |
| Kennlinientreue im Feld | 194 von 194 Zeilen auf ±1 Prozentpunkt |
| Prüfstandsvergleich (Experiment D) | Legacy 3,924 → **0,000** gegenüber neu 3,9 → **3,836** m/s² |
| Gyro-Nullpunkt (Feld) | −4,08 °/s, Drift +0,18 °/s über drei Minuten |
| Regimeverteilung (Feld) | STATIC 26,2 %, DYNAMIC 69,2 %, SHOCK 4,6 % |

**Bekannte Grenzen, die mit in die Arbeit gehören.** Die effektive Ansprechschwelle liegt bei **2,13 m/s²**, nicht bei den nominellen 2,0 — die verbleibende Filterdämpfung beträgt 5,9 % (vorher 20,7 %), weil jede Bremsung beim Anstieg die Totzone des STATIC-Bands von 0…1,539 m/s² durchläuft. Eine Angabe von „2,0 m/s²" wäre nicht belegbar. Zweitens trägt `brake_decel_ms2` eine **geschwindigkeitsabhängige Grundlinie** (Befund G-01): Korrelation mit der Geschwindigkeit r = +0,80, Partialkorrelation unter Kontrolle der Fahrbahnneigung +0,75; bei 25–35 km/h verbleiben im 99-%-Quantil nur **0,32 m/s²** Reserve zur Ansprechschwelle. Erkennungsschwelle und Störfestigkeit sind damit geschwindigkeitsabhängig, und zwar gegenläufig. Als Ursachen wirken in dieselbe Richtung: die einseitige Begrenzung des Ausgangs auf positive Werte (ein mittelwertfreies Rauschsignal erhält dadurch den Erwartungswert σ/√(2π)), die mit der Geschwindigkeit seltener werdende Verankerung und das um den Faktor drei steigende Anregungsniveau. Dieser Befund liefert zugleich die erste **gemessene** Begründung für Stufe 2 (GNSS-Bias-Term).

---

## 7. Zustandsautomaten und Systemlogik

**R2 Rücklicht/Bremslicht.** Vier Zustände: `InitBlink` → `Taillight` ⇄ `Brakelight` → `EssBlink`. Die Aufteilung in einen **zustandslosen** Proportionalteil (`brake_curve`) und einen **zustandsbehafteten** Automaten (`tail_light_fsm`) ist bewusst: Die Kennlinie ist eine reine Abbildung und als solche trivial testbar; Hysterese, Mindesthaltezeit und ESS-Vorrang gehören in den Automaten. Diese Trennung hat sich bei der Fehleranalyse von M-01 als nützlich erwiesen, weil sie den Defekt eindeutig lokalisierbar machte: Die Kennlinie war nachweislich korrekt (194/194 Feldzeilen auf ±1 Prozentpunkt), der Fehler lag ausschließlich im Automaten.

**R1 Lebenszyklus.** INIT → RUN mit Sensor-Init-Timeout und degradiertem RUN. Die Entscheidung, `lifecycle_fsm.degraded` und das Laufzeit-Sensorsignal `imu_health` **getrennt** zu führen, ist begründungspflichtig: `degraded` bleibt nach FR-STA-06 das Ergebnis der Initialisierung und beschreibt den Systemzustand; ein Sensorausfall im Betrieb ist ein anderer Sachverhalt und gated nur die Region R2. Hätte man `degraded` erweitert, wären zwei semantisch verschiedene Zustände in einer Variablen zusammengefallen.

**R3 Blinker.** AUS / LINKS / RECHTS / WARN, 1,5 Hz, Warnblinker per Langdruck ≥ 5 s. *Warum Langdruck:* Die ASK-Fernbedienung sendet kein Kombisignal; gleichzeitiges Drücken zweier Tasten ist auf der Funkstrecke nicht darstellbar.

**Prioritätsordnung.** Schlusslicht > Bremslicht > Blinker > Telemetrie. Das Schlusslicht ist die einzige nach § 67 StVZO zwingend erforderliche Funktion und darf durch keinen Fehlerzustand ausfallen; die Telemetrie ist rein beobachtend und wird zuerst geopfert.

**Mögliche Abbildungen:** Zustandsdiagramme je Region · Kennlinie `brake_curve` mit Hysteresebändern und den drei Schwellwerten · Ablaufdiagramm der Prioritätsordnung im Fehlerfall.

---

## 8. Telemetrie, Datenformat und BLE

**Frame-Schema v3.** 113 Byte, Little-Endian, gepackt, 10 Hz, `version` an Offset 0. Die Offsets 0–80 sind **byte-identisch** zu Schema v2; die Offsets 81–112 tragen 13 Diagnosefelder: `gnss_accel_ms2` + `gnss_accel_valid`, `pitch_rad`, `gyro_bias_rads`, `bias_calibrated`, `norm_delta_min/_max`, `jerk_max`, drei Regime-Zähler, `dt_max_ms`, `loop_max_us`.

**Der zentrale Entwurfspunkt.** Regime-Entscheidung und Jerk-Kriterium leben auf dem 100-Hz-Takt, das Frame läuft mit 10 Hz. Momentanwerte hätten um den Faktor zehn unterabgetastet — gerade die Stöße, um deren Erkennung es geht, wären verschwunden. Übertragen werden deshalb **Aggregate über das 100-ms-Fenster** (Minima, Maxima, Zähler). Damit bleibt die 100-Hz-Information erhalten, ohne die Datenrate zu erhöhen. Das ist ein gut darstellbares Beispiel für einen Zielkonflikt zwischen Informationsgehalt und Bandbreite und seine Auflösung.

**Vorwärtskompatibilität.** Die Versions- und Längenregel ist der eigentliche Schnittstellenvertrag: `len < 81` oder `version < 2` → verwerfen; `version ≥ 2 & len ≥ 81` → v2-Felder lesen; `version ≥ 3 & len ≥ 113` → zusätzlich v3-Felder; überzählige Bytes ignorieren. Ein zu kurzes v3-Frame wird auf v2-Ebene gelesen und gezählt statt verworfen (V3-1). Zweck ist, dass Firmware und App unabhängig voneinander weiterentwickelt werden können.

**Ungültigkeitsmarkierung per Flag, nicht als NaN.** Eine ungültige GNSS-Referenz wird als `0.0f` plus separates `gnss_accel_valid` übertragen. Der App-Decoder und der CSV-Export müssten einen NaN-Sonderfall sonst über Swift, SwiftData und die deutsche Zahlformatierung durchreichen.

**BLE-Transport.** NimBLE-Arduino 2.5.0, Service `587bb505-9f9d-4ae0-96fd-0b29adfc4b03`, Notify-Characteristic `8c604d09-743f-4850-9109-19604a17f358`, Gerätename `SmartBikeRearLight`, MTU 185, **unidirektional** ESP32 → App ohne Write-Characteristic (FR-SYS-04). RAM-Ringpuffer puffert bei Verbindungsabbruch etwa 60 s bei 10 Hz und liefert nach dem Reconnect nach.

**Golden-Vektor-Kreuztest.** `testdata/frame_v3_golden.hex` (113 Byte) plus Wertetabelle `.md`. Die Firmware erzeugt die Bytefolge, der Produktions-Decoder der iOS-App liest genau diese Bytes gegen die Wertetabelle — ausdrücklich **nicht** gegen den eigenen Encoder. Round-Trip-Tests je Seite prüfen nur die eigene Symmetrie; ein gemeinsamer Denkfehler bliebe darin unsichtbar und fiele erst in den Felddaten auf. Der Bootstrap-Lauf schlägt bei fehlender Referenzdatei bewusst fehl, damit ein fehlendes Golden-File nicht als „bestanden" durchgeht. **Offene Formalie:** Der Firmware-Git-Hash (`1178017`) fehlt in der Wertetabelle, und die frühere Diskrepanz „41 gegenüber 43" bei der Feldanzahl beruhte auf zwei nicht vergleichbaren Zählweisen: Die Firmware-Wertetabelle (`testdata/frame_v3_golden.md`) hat 44 Felder / 113 Byte, darunter 39 **unterscheidbare** Werte (zwei Werte mehrfach: `3` zweimal, `1 (true)` fünfmal); der App-Test `FrameGoldenVectorTests.swift` prüft dagegen **alle** 44 Felder gegen die Tabelle (23 Ganzzahl-/Enum-Felder exakt, 21 Float-Felder mit Toleranz) — beide Seiten decken die volle Bytefolge ab, die vormaligen Zahlen maßen nur nicht dieselbe Größe.

---

## 9. Fehlerbehandlung, Robustheit und Zeitverhalten

**Gestufte I²C-Recovery.** Stufe 1 Soft-Reinit, Stufe 2 SCL-Clock-Release: bis zu neun Taktpulse mit 5 µs High und 5 µs Low (etwa 100 kHz), Abbruch sobald SDA wieder High ist, anschließend eine manuelle STOP-Bedingung. Gesamtdauer maximal etwa 105 µs, gegenüber NFR-RT-04 unkritisch.

*Der eigentliche Befund dahinter.* Die Recovery wirkte zunächst nicht; das Log zeigte wiederholt „Bus already started in Master Mode". Im Core-Quelltext verifiziert: `Wire.end()` ruft `i2cDeinit()` → `i2c_del_master_bus()`; nur bei Erfolg wird der Bus als deinitialisiert markiert und die Pins über `perimanClearPinBus()` freigegeben. Bei elektrisch blockiertem Bus schlägt `i2c_del_master_bus()` fehl — der nächste `Wire.begin()` sieht den Bus fälschlich als bereits gestartet und wird zum wirkungslosen No-op. Zusätzlich bleiben die Pins PeriMan-seitig I²C-besessen, wodurch auch `pinMode()`/`digitalWrite()` in der Release-Routine kommentarlos nichts bewirken. Die Lösung besteht darin, den Bus mit **rohen ESP-IDF-`gpio_*`-Aufrufen** physisch freizumachen — die umgehen PeriMan vollständig — und zwar **vor** dem `Wire.end()`-Versuch. Das ist ein sehr gutes Beispiel für eine Abstraktionsschicht, die im Fehlerfall stillschweigend versagt, und für die Notwendigkeit, in einer solchen Situation im Quelltext der Bibliothek nachzusehen statt zu vermuten.

**Plausibilität und Eskalations-Vertrauen.** Wertebereichsprüfung (3,0…149,0 m/s²), Frozen-Erkennung, Sprung-/Slew-Grenzen. Einer Eskalation der Bremslicht-Duty wird erst nach **drei** aufeinanderfolgenden plausiblen Zyklen vertraut — ein einzelnes „Müll-aber-im-Bereich"-Sample nach einem Fehlerfall darf kein Bremslicht auslösen. Genau dieser Fall trat in der Fehlerinjektion auf.

**Watchdog.** Task-Watchdog mit etwa 2 s, `enableLoopWDT()` registriert den `loopTask`, der Arduino-Core füttert automatisch vor jedem `loop()`. Boot-Diagnose über `esp_reset_reason()`. Per `'H'`-Hang-Hook am Board verifiziert: Auto-Reset nach ~2 s, Reset-Grund korrekt erkannt.

**Zeitverhalten.** NFR-RT-01 (Reaktionszeit ≤ 50 ms) ist mit ≤ 10 ms (Sprung), ≤ 10 ms (nach Verankerung) und 20 ± 10 ms (Gefälle plus Bremsung) erfüllt. Die Messauflösung beträgt 10 ms; Angaben unter 10 ms sind als „≤ 10 ms" zu führen, nicht als „0 ms" — eine frühere Angabe von „0 ms" war scheingenau und wurde korrigiert. NFR-RT-04 (Schleifenzeit < 10 ms) ergibt am Prüfstand 0,651 ms und im Fahrbetrieb **6,7 ms** Worst Case bei 0,00 % der Fenster über 10 ms.

**Methodisch wichtig:** Die Prüfstandszahl unterschätzt den Fahrbetrieb um rund den Faktor zehn. Die Spitzen treten periodisch mit 1,00 s auf. Die ursprüngliche Zuordnung allein zum GNSS-Slot ist **zurückgenommen**, weil im Aufzeichnungsstand drei Debug-Ausgaben mit ebenfalls exakt 1 Hz innerhalb des Messfensters von `loop_max_us` lagen (zusammen etwa 173 Byte, bei 115 200 Bd etwa 87 µs je Byte). Zwei Vorgänge identischer Periode sind aus einer Zeitreihe nicht trennbar. Im Auslieferungsstand ist diese Last entfallen.

---

## 10. Technische Entscheidungen (Firmware)

Vollständige Fassung mit allen Alternativen: `decision_log.md` und Project Bible Kap. 10. Hier die für Kap. 6 tragenden Entscheidungen.

| Entscheidung | Ausgangssituation | Alternativen | Begründung | Ergebnis | Relevanz |
|---|---|---|---|---|---|
| Rechenlast in die App (Variante 2, verteilte Berechnung) | Kennzahlen könnten auf dem ESP32 oder in der App entstehen | Firmware rechnet alle Kennzahlen | ESP32 bleibt deterministisch, geringer RAM-/CPU-Bedarf, Rohdatenintegrität | Frame trägt Rohgrößen; Höhe, Distanz, Statistik in der App | **sehr hoch** — Kap. 4.1 |
| Vier orthogonale Zustandsregionen | Gesamtverhalten aus Lebenszyklus, Licht, Blinker, Sensorik | flache kombinierte FSM | additive statt multiplikative Zustandsanzahl, jede Region einzeln testbar | 4 Regionen, Kopplung über 2 explizite Signale | hoch — Kap. 4.3, 6.2 |
| Kooperativer `millis()`-Scheduler | Echtzeitanforderung 100 Hz | eigene FreeRTOS-Tasks | deterministisch, testbar, dokumentierbar; CPU zu über 99 % frei | Worst Case 6,7 ms bei 10-ms-Raster | hoch — Kap. 6.1 |
| Trennung reine Logik ↔ Treiber | sicherheitsrelevante Logik muss prüfbar sein | Logik an Treiber gekoppelt | ermöglicht 126 Host-Unit-Tests ohne Zielhardware | `lib/logic` frei von Arduino-Includes | **sehr hoch** — Kap. 6.1, 6.6 |
| PlatformIO mit pioarduino statt Arduino IDE | Core 3.x nötig für `ledcAttach` | Arduino IDE, offizielle `espressif32`-Plattform (Core 2.0.17) | nur pioarduino liefert Core 3.3.11; zusätzlich Versionskontrolle und Host-Tests | reproduzierbare Umgebung | hoch — Kap. 6.1 |
| **Architektur V-B: IMU schneller Regelpfad, GNSS langsame Stützreferenz** | Feldtest hatte die IMU-Kette falsifiziert | V-A GNSS primär; V-C IMU unverändert | GNSS erfüllt NFR-RT-01 physikalisch nicht (Latenz 200–400 ms, 10 Hz Grenze) und ist nicht integritätssicher (FIX_OK bei grob falscher Lösung); die IMU liefert 100 Hz mit < 10 ms — sie war nie das Problem, nur ihre Aufbereitung | Zweistufiger Umbau, Stufe 1 verifiziert | **sehr hoch** — Kap. 4.1, 6.3, 10.1 |
| **Normbetrags-Gate statt Tiefpass oder Median** | Bremsung und Stoß müssen getrennt werden | Tiefpass auf `a_y`; Median; α anheben | nutzt den physikalischen Diskriminator ‖a‖ statt einer Frequenztrennung, die es nicht gibt; kostet **0 ms** Latenz, während ein 8-Hz-Tiefpass einen 25-ms-Stoß zu 71 % durchlässt; etabliertes Verfahren (Madgwick, Mahony) | drei Regime, feldverifiziert | **sehr hoch** — Kap. 6.3 |
| SHOCK primär über den Jerk | betragsbasierte Schwellen hatten zwei Lücken | rein betragsbasiert (0,35 / 2,5) | über den Jerk trennen sich die Fälle um zwei Größenordnungen | keine verpassten Bremsungen ≤ 2,64 m/s² mehr | hoch |
| Verankerung und Bias-Kalibrierung entkoppeln | „ein Zustand, zwei Verbraucher" | gemeinsames Fenster beibehalten | funktioniert real nie: 0,392¹⁰⁰ ≈ 10⁻⁴¹; die Verankerung braucht Kohärenz, die Mittelung nicht | `bias_calibrated` ab dem ersten Frame | **sehr hoch** — Kap. 6.3, 10.3 |
| τ 0,49 → 3,0 s, τ_slow 30 → 90 s mit Rückfall | alte Werte nie begründet | bei 0,49 s bleiben; einheitlich 90 s | Rampen-Absorption sinkt von 0,30 auf 0,06 m/s²; 90 s erst durch Bias-Kalibrierung zulässig (ε = b·τ) | effektive Schwelle 2,13 statt 2,4 m/s² | hoch |
| Einbaulage an der Treibergrenze | Platine 180° gedreht verbaut | Vorzeichen in `motion_filter`; `MOTION_BRAKE_SIGN` umkehren | mechanische Eigenschaft gehört an die Hardware-Abstraktionsgrenze; alles Nachgelagerte bleibt validiert; Determinante +1 erhält dθ/dt = ω_x | 126 Tests, Golden-Vektor, App unverändert gültig | hoch — Kap. 6.1, 10.3 |
| MPU6050 DLPF 44 Hz / 200 Hz fest setzen | POR-Defaults 260 Hz bei 8 kHz | Defaults belassen; `SMPLRT_DIV = 9`; Data-Ready-Interrupt | Unterabtastung ohne Antialiasing; 200 Hz vermeidet die Schwebung, die den scheinbaren Jerk verdoppelt | STATIC-Anteil 68–73 % → 85,7 % | hoch — Kap. 6.5, 9.2 |
| I²C-Recovery über rohe `gpio_*`-Aufrufe | `Wire.end()` scheitert bei blockiertem Bus still | Arduino-`pinMode()`/`digitalWrite()` | PeriMan blockiert selbst, im Core-Quelltext verifiziert | Recovery per Fehlerinjektion wirksam | hoch — Kap. 6.5 |
| Getrennte Signale `degraded` und `imu_health` | Sensorausfall im Betrieb vs. Init-Ergebnis | `degraded` erweitern | zwei semantisch verschiedene Sachverhalte dürfen nicht in eine Variable fallen | R2 sauber gegatet | mittel |
| Fensteraggregate statt Momentanwerte im Frame | 100-Hz-Logik, 10-Hz-Frame | Momentanwerte; Frame-Rate anheben | Momentanwerte unterabtasten um Faktor zehn; höhere Rate kostet Bandbreite und Strom | 100-Hz-Information bei gleicher Datenrate | **sehr hoch** — Kap. 7.1 |
| Golden-Vektor als Kreuztest über die Toolchain-Grenze | Firmware und App prüfen sonst nur die eigene Symmetrie | beidseitige Round-Trip-Tests | ein gemeinsamer Denkfehler bliebe unsichtbar und fiele erst in den Felddaten auf | Kreuztest bestanden | **sehr hoch** — Kap. 6.6, 7.1 |
| Board-Tausch auf ESP32-DevKitC-32E | BLE-Brownout-Bootloop | Altboard behalten; WiFi statt BLE | Root Cause: Regler des Altboards liefert die RF-Kalibrierungs-Transiente nicht; WiFi teilt denselben Funk und dieselbe Kalibrierung | BLE stabil, MTU 185, Volllast | hoch — Kap. 5.4, 9.5 |
| **Umfangsschnitt FR-CFG-02/03** | beide spezifiziert, keine umgesetzt | beide umsetzen; nur FR-CFG-02 | NVS ist ein struktureller Eingriff (alle Kalibrier-`constexpr` müssten Laufzeitparameter werden) ohne Nachweisnutzen — parametriert wurde ausschließlich am Entwicklungsrechner | als begründete Abgrenzung geführt | hoch — Kap. 10.1, 10.3 |
| M-01 beheben statt dokumentieren | Mindesthaltezeit im Feld unwirksam | als Anforderungsabweichung führen; Hysterese-Grenzen verschieben | eine spezifizierte, getestete und nachweislich nicht wirkende Funktion ist ein Mangel; die Korrektur ist der kleinstmögliche Eingriff | behoben, Regressionstest, am Gerät bestätigt | **sehr hoch** — Kap. 6.3, 9.3 |

---

## 11. Entwicklungsprobleme und Lösungsfindung

| Problem | Ursache | Analyse | Lösungsansätze | Gewählte Lösung | Validierung | Relevanz |
|---|---|---|---|---|---|---|
| **P1 — Bremserkennung im Feld funktionsunfähig** | Komplementärfilter mit τ = 0,49 s rechnet anhaltende Bremsung als Neigung weg; kein Stoßschutz | Feldtest als Falsifikationsversuch, sechs Fahrten; Duty-Verteilung 93–100 % auf Grundwert; −5,75 m/s² real → 0,18 m/s² gemessen; Neigen im Stand → 42 % Duty | V-A GNSS primär; V-B Umbau der Aufbereitung; V-C unverändert | **V-B + Normbetrags-Gate (Stufe 1)** | Bench D (3,924 → 0,000 gegen 3,9 → 3,836); Messfahrt 08.08. (9/9, r = +0,85) | **sehr hoch** — Kernnarrativ der Arbeit |
| **P2 — BLE-Start löst Brownout-Bootloop aus** | Regler des AZ-Delivery-Altboards liefert die RF-Kalibrierungs-Transiente von `NimBLEDevice::init()` nicht | Zehn systematische Versuche; Firmware per Host-Tests und Bracket-Logging ausgeschlossen; jede Gegenmaßnahme einzeln geprüft; **entscheidend: Deaktivierung des Brownout-Detektors** — das Board hing danach an derselben Stelle statt weiterzulaufen, also echter Spannungskollaps, keine Fehlauslösung | Sendeleistung senken; CPU-Takt senken; Kondensatoren; andere Quelle; Board tauschen; WiFi | **Board-Tausch auf Espressif DevKitC-32E**; Kondensatoren trotz Wirkungslosigkeit gegen den Brownout beibehalten | BLE-Advertising, Verbindung, MTU 185, Volllast stabil | **sehr hoch** — Musterbeispiel systematischer Eingrenzung, Kap. 9.5, 10.3 |
| **P3 — Bias-Kalibrierung kommt real nie zustande** | Verankerungsfenster verlangte 100 zusammenhängende STATIC-Abtastungen bei real 25–39 % STATIC-Anteil | 0,392¹⁰⁰ ≈ 10⁻⁴¹; `bias_calibrated` blieb 0 über 80 s; System lief dauerhaft im 30-s-Rückfall | Fenster verkürzen; Anforderung lockern; Verankerung und Mittelung entkoppeln | **Entkopplung**: Verankerung 0,3 s kohärent, Bias kumulativ über 200 Abtastungen | `bias_calibrated` = 1 nach ≈ 3 s am Board, ab dem ersten Frame im Feld | **sehr hoch** — Kap. 6.3, 10.3 |
| **P4 — Ruhesockel von 3,0 m/s² im Stillstand** | unkompensierter Gyro-Nullpunktfehler von −4,61 °/s | ε = b·τ ergibt bei τ_eff = 4 s einen Lagefehler von 18,4° und damit g·sin(18,4°) = 3,1 m/s² — deckt sich mit dem Messwert | Filter ändern; Bias kompensieren | **Bias-Kalibrierung** (folgt aus P3) | Feld: Median 0,00, P99 0,08 m/s² über 11,8 s Stillstand | hoch — zeigt, dass ein scheinbarer Filterdefekt auf eine einzelne Sensorgröße zurückführbar war |
| **P5 — Aliasing der IMU-Auslesung** | POR-Defaults `DLPF_CFG = 0` (260 Hz) bei 8 kHz interner Rate, von der Bibliothek nie gesetzt | Registerrücklesung; `norm_delta`-Spanne 0,34–0,45 gegenüber 0,235 nach der Korrektur | Defaults belassen; DLPF setzen; Data-Ready-Interrupt | **`DLPF_CFG = 3` (44 Hz), `SMPLRT_DIV = 4` (200 Hz)** | Registerrücklesung am Board bestätigt; STATIC-Anteil 68–73 % → 85,7 % | hoch — Kap. 6.5, 9.2 |
| **P6 — I²C-Recovery wirkungslos** | `Wire.end()` scheitert bei blockiertem Bus still; PeriMan hält die Pins | Core-Quelltext verifiziert (`i2c_del_master_bus()` schlägt fehl → Bus gilt als „schon gestartet") | Arduino-API; rohe `gpio_*`-Aufrufe | **rohe `gpio_*`-Aufrufe vor `Wire.end()`** | SDA-Kurzschluss-Fehlerinjektion am Board | hoch — Kap. 6.5 |
| **P7 — Mindesthaltezeit im Fahrbetrieb unwirksam (M-01/B5)** | `else`-Zweig überschrieb den eingefrorenen Haltewert im Hystereseband 1,5…2,0 mit dem Schlusslichtwert | 0 von 14 Aktivierungen wirksam; FSM-Nachbildung reproduziert die Aufzeichnung zu 98,53 %; Testlücke benannt | beheben; als Anforderungsabweichung führen | **beheben**: Nachführung nur oberhalb 2,0 m/s² | Regressionstest durch das Hystereseband; 126/126; am Gerät bestätigt | **sehr hoch** — vollständiger Nachweiszyklus |
| **P8 — Einbaulage nach mechanischem Umbau** | Platine 180° in ihrer Ebene gedreht; Bremserkennung hätte sich invertiert | Rotation R_z(180°) = diag(−1, −1, +1), Determinante +1; Accel und Gyro müssen gleich transformiert werden, sonst gilt dθ/dt = ω_x nicht mehr | Vorzeichen in der Auswertelogik; Transformation an der Treibergrenze | **Treibergrenze** | Host-Test; am Gerät im Normalbetrieb bestätigt | hoch — Kap. 6.1, 10.3 |
| **P9 — Bench-Harness verfälschte die Messung** | `runFullChainExperiment()` startete `next_tick = t0`, das erste `dt_s` wurde auf 1 ms geklemmt → Verankerungsfenster 9 ms zu klein | `calibrated = 0` in D und E reproduzierbar; im Host-Test unsichtbar, weil dort ein konstantes `dt = 0,01 s` eingespeist wird | Vorlauf verlängern; Harness korrigieren | **beides**: `next_tick = t0 + 10 µs`, Vorlauf 1000 → 2500 ms | Nachlauf A6: `calibrated = 1` in D und E, τ_slow = 90 s, D erreicht 3,836 m/s² gegen Vorhersage 3,83 | hoch — Beispiel dafür, dass das Messmittel selbst validiert werden muss |
| **P10 — 1-Hz-Aufzeichnung verdeckte die Bremsdynamik** | Frame 10 Hz, App-Persistenz 1 Hz; die 300-ms-Haltezeit überlebt einen Peak, der beim Abtastzeitpunkt schon abgefallen ist | Zeilen mit `brake_decel = 0,00` bei erhöhter Duty; kein Logikfehler, ein Messartefakt | Aufzeichnung erhöhen; Artefakt dokumentieren | **10-Hz-Validierungsmodus** (Entscheidung E2) | 1773 von 1776 Frames der Messfahrt vollständig | hoch — Kap. 9.1 Messkonzept |
| **P11 — Debug-Ausgaben im Messfenster** | drei `Serial.printf` mit je 1 Hz innerhalb des `loop_max_us`-Fensters, zusammen ≈ 173 Byte bei 115 200 Bd | zwei Vorgänge identischer Periode sind aus der Zeitreihe nicht trennbar | Ursache messen; Zuordnung zurücknehmen und Instrumentierung entfernen | **Zuordnung zurückgenommen, Ausgaben entfernt** | Auslieferungsstand ohne UART-Last | hoch — Kap. 10.3 Methodenkritik |
| **P12 — Akkubetrieb-Freeze** | USB-Kabel im ESP32, am Host getrennt → floatende VBUS-Leitung → unruhige 3,3-V-Schiene → I²C-Aussetzer | Blinker (nicht IMU-abhängig) lief weiter → regionsspezifisch, nicht global | — | im echten Akkubetrieb nicht reproduzierbar, als Debug-Artefakt dokumentiert | Beobachtung | mittel — „Debug-Setup ≠ Feldbedingung" |

*(Reine App-Probleme — Zeitstempel-Kontinuität nach Neustart, `millis()`-Unterlauf mit 1191:44:03 h — gehören ins iOS-Kapitel; das `millis()`-Thema hat allerdings eine Firmware-Seite und ist unter Punkt 15 als Cross-System-Befund geführt.)*

---

## 12. Struktur des Firmware-Kapitels (Kapitel 6, an die reale Entwicklung angepasst)

Die freigegebene Gliederung sieht für Kapitel 6 sechs Unterkapitel vor. Der Vorschlag behält diese Struktur weitgehend bei, verschiebt aber den Schwerpunkt auf die Bremserkennung, weil dort die belastbarste Nachweiskette liegt, und ergänzt ein Unterkapitel zur Telemetrie-Erzeugung, das derzeit zwischen Kap. 6 und Kap. 7 unklar verortet ist.

- **6.1 Entwicklungsumgebung und Softwarearchitektur** — Toolchain und ihre Begründung (pioarduino wegen Core 3.x); Modultrennung Logik ↔ Treiber als Voraussetzung der Testbarkeit; kooperativer Scheduler mit Begründung gegen FreeRTOS; Task-Raster und Prioritätsordnung; Einbaulage-Rückabbildung als Beispiel der Abstraktionsgrenze. *Tab.:* Modulübersicht mit Verantwortlichkeiten; Task-Slots mit Perioden. *Abb.:* Schichtenbild; Sequenz eines `loop()`-Durchlaufs. *Nachweis:* Quelltext, `platformio.ini`, Bible Kap. 6.
- **6.2 Zustandsautomaten (R1–R4)** — vier orthogonale Regionen mit Begründung gegen die flache FSM; Zustände und Übergänge je Region; getrennte Signale `degraded` und `imu_health`. *Tab.:* Zustands-/Übergangstabelle je Region. *Abb.:* Harel-Diagramm. *Nachweis:* Host-Tests, Bible Kap. 6.6.
- **6.3 Bremserkennung und Bremslicht-Kennlinie** — **Schwerpunktkapitel.** Anforderung; Ausgangsentwurf; Falsifikation mit den zwei Fehlermechanismen; Prüfung und Verwerfen der GNSS-primären Variante; Normbetrags-Gate mit physikalischer Herleitung und Latenzvergleich; Parametrierung der Zeitkonstanten; Entkopplung von Verankerung und Bias-Kalibrierung; Kennlinie und Zustandsautomat; Mangel M-01 und seine Behebung. *Tab.:* Variantenvergleich V-A/B/C; Latenzvergleich der Filteralternativen; Schwellwerte mit Herleitung. *Abb.:* Vektordiagramm der drei Fahrzustände; Kennlinie mit Hysterese; Vorher-Nachher Legacy gegen neu (Bench D); Regime-Klassifikation über einen Bremsvorgang. *Nachweis:* Feldtest 06.08., `stufe1_normgate.md`, `bench_run_notes.md`, Messfahrt 08.08.
- **6.4 433-MHz-Blinkerlogik** — Empfängerauswahl SRX882S; Dreiteilung Treiber → Dekoder → FSM; Entprellung, Loslass-Erkennung, Kurz-/Langdruck; Warnblinker per Langdruck mit Begründung; Gating gegen den Systemzustand. *Tab.:* Ereignistabelle des Dekoders. *Abb.:* Zeitdiagramm eines Tastendrucks. *Nachweis:* Host-Tests, HW-Validierung.
- **6.5 Robustheit und Fehlerbehandlung** — Fail-safe-Prinzip und Prioritätsordnung; Plausibilitätsprüfung und Eskalations-Vertrauen; gestufte I²C-Recovery inklusive des PeriMan-Befunds; Watchdog; Sensorkonfiguration gegen Aliasing. *Tab.:* Fehlerfälle mit Reaktion und Nachweis. *Abb.:* Recovery-Ablaufdiagramm. *Nachweis:* Fehlerinjektion, Core-Quelltextanalyse.
- **6.6 Testkonzept und Verifikation** — zwei Ebenen (Host-Unit-Tests, On-Target); Testdaten-Einspeisung über `BENCH_MODE` und deren **Grenzen**; Golden-Vektor-Kreuztest über die Toolchain-Grenze; die Testlücke, die M-01 verdeckte. *Tab.:* Testabdeckung je Modul; Testebenen mit Aussagekraft und Grenze. *Abb.:* Testpyramide mit Einordnung der drei Ebenen. *Nachweis:* 126/126, Golden-Vektor, `bench_run_notes.md`.
- **6.7 Telemetrie-Erzeugung (neu vorgeschlagen)** — Fensteraggregation des 100-Hz-Takts, Ringpuffer, Serialisierung. Nur der **firmwareseitige** Teil; der Schnittstellenvertrag selbst gehört nach 7.1. Alternativ als 6.6-Unterabschnitt, falls sieben Unterkapitel zu viel sind. *Abb.:* Zeitdiagramm 100 Hz → 100-ms-Fenster → Frame. *Nachweis:* `BLE_Frame_v3_Schnittstelle.md`.
- **6.8 Abgrenzung des Firmware-Umfangs** — FR-CFG-02/03 und die nicht feldverifizierten Parameter mit Begründung und Auswirkung. Kurz halten (eine halbe Seite) und auf Bible Kap. 12.2 stützen; alternativ in 10.3 unterbringen.

**Was ausdrücklich nicht in Kapitel 6 gehört:** die Messergebnisse selbst (Kap. 9), der BLE-Schnittstellenvertrag (Kap. 7.1), die Architekturentscheidung V-B auf Systemebene (Kap. 4.1, in 6.3 nur referenzieren), die vollständige SRS (Anhang B).

---

## 13. Validierung der Firmware (Testübersicht)

| Test | Ziel | Versuchsaufbau | Messgröße | Soll | Ist | Ergebnis | Aussage |
|---|---|---|---|---|---|---|---|
| Bench A — Kennlinie | FR-TL-06 Proportionalteil | `esp32dev_bench`, synthetische Rampe 0 → 6,0 → 0 m/s² über 31 s, Log 100 Hz, 3101 Datenzeilen | `brake_light_pct` über `brake_decel_ms2` | 20 % Grundwert, Ansprechen 2,0, Sättigung 5,0 | Grundwert 20 % konstant; Ansprechen bei **2,004 m/s²**; Sättigung bei 4,98; `pct = 26,66·decel − 33,32`, **R² = 0,99984** (theoretisch 26,67) | ✅ | Die Kennlinie ist spezifikationskonform und praktisch ideal linear |
| Bench B — Zeitverhalten | NFR-RT-01, Haltezeit | Sprung 0 → 6,0 m/s² für 0,5 s, 351 Datenzeilen | Anstiegszeit, Haltedauer | ≤ 50 ms; 300 ms | Anstieg 20 → 100 % **innerhalb eines 10-ms-Schritts**; Duty 100 % von 1500 bis 1800 ms = **300 ms exakt** | ✅ | NFR-RT-01 mit Faktor ≥ 5 Reserve; Haltezeit **für einen idealisierten Sprung** korrekt |
| Bench C — Fail-Safe | FR-SAF-01, FR-STA-04 | `imu_health` auf FAILED erzwungen, identische Rampe | `brake_light_pct` | 20 % | **konstant 20 %** über den gesamten Lauf | ✅ | Fail-Safe-Gate wirkt |
| Bench D — Vorher-Nachher Filter | Wirksamkeit Stufe 1 | volle Kette, 4 s Dauerbremsung, Legacy und neu parallel | Filterausgang | Legacy soll kollabieren, neu halten | Legacy 3,924 → **0,000**; neu 3,9 → **3,836** (analytische Vorhersage 3,83) | ✅ | Der Kernbeleg für Fehlermechanismus A und seine Behebung, auf realer Hardware |
| Bench E — Stoßunterdrückung | SHOCK-Regime | drei Stöße von 20–30 ms | Regimeverteilung | Stöße als SHOCK | STATIC 98,0 % / DYNAMIC 0,0 % / **SHOCK 2,0 %**; kein DYNAMIC-Zwischenzustand | ✅ | Stöße werden zuverlässig erkannt, Konstantfahrt bleibt STATIC |
| Bench F — Neigung + Bremsung | Trennung Neigung/Bremsung | 4 s Neigung, dann 2 s Bremsung | Regimeverteilung, Reaktionszeit | 66,7 / 33,3 % | **66,7 / 33,3 / 0,0 %**, exakt der Profilkonstruktion entsprechend; Reaktionszeit **20 ± 10 ms** | ✅ | Neigung erzeugt kein Bremslicht, Bremsung schon |
| dt-Statistik (A6.4) | Taktstabilität | offline aus `bench_capture4`, Feld `dt_real_ms` | `dt_s` | 10 ms | D: 10,0000 ms, σ = 0,9744; E: 10,0000, σ = 0,6085; F: 10,0017, σ = 0,6173; Min 9 / Max 11; **0 Clamp-Ereignisse** | ✅ | Keine Drift; Streuung auf ±1 ms begrenzt |
| Fehlerinjektion SDA-Kurzschluss | FR-SNS-03/04/05, FR-SAF-01 | physischer Kurzschluss am laufenden Bus | Recovery, Bremslicht | Recovery wirksam, kein Fehl-Bremslicht | Recovery wirksam; kein Fehl-Bremslicht; Fail-Safe fällt sicher auf Schlusslicht | ✅ | qualitativ/funktional — **kein Messprotokoll mit Zahlen vorhanden** |
| Watchdog-Test | FR-SAF-03 | `'H'`-Hang-Hook über Serial | Reset-Zeit, Reset-Grund | ~2 s, korrekt erkannt | Auto-Reset nach ~2 s, `esp_reset_reason()` korrekt | ✅ | qualitativ/funktional — kein protokollierter Zeitmesswert |
| Registerrücklesung MPU6050 | Sensorkonfiguration | Auslesen nach `imuBegin()` | `CONFIG`, `SMPLRT_DIV` | 0x03, 4 | `CONFIG = 0x03 … SMPLRT_DIV = 4 … -> OK` | ✅ | Konfiguration wirksam gesetzt |
| Host-Unit-Tests | NFR-TST-01/03 | `pio test -e native`, Unity | Testergebnis | alle grün | **126/126** (Commit `835c7b3`) | ✅ | Logikschicht vollständig host-verifiziert |
| Golden-Vektor-Kreuztest | Schnittstellenvertrag | Firmware erzeugt 113-Byte-Hex, App-Decoder liest gegen die Wertetabelle | Feldwerte | Übereinstimmung | bestanden | ✅ | Geräteunabhängiger Nachweis über die Toolchain-Grenze |
| Feldtest 06.08.2026 | Falsifikationsversuch | sechs Fahrten, ~5,1 km, App-Aufzeichnung 1 Hz, Referenz Strava, 939 auswertbare Zeilen | Duty-Verteilung, Einzelereignisse | Bremsung soll Bremslicht erzeugen | 93–100 % der Zeilen je Fahrt auf Grundwert; −5,75 m/s² real → 0,18 m/s²; Neigen im Stand → 42 % Duty | ❌ **falsifiziert** (beabsichtigt) | Der wertvollste Ausgang: ein messbar belegter Konstruktionsfehler |
| Messfahrt 08.08.2026 | Wirksamkeitsnachweis Stufe 1 | eine Fahrt, 177,86 s, 10 Hz, Schema v3, 1773/1776 Frames | Erkennungsrate, Korrelation, Fehlauslösungen | alle Bremsungen erkannt, r deutlich > 0,5 | **9/9**; r = **+0,85** bei festem Versatz (06.08.: Median +0,15); **0/25** Fehlauslösungen; Ruhesockel Median 0,00 | ✅ | Kernnachweis der Arbeit |
| Zeitverhalten im Feld | NFR-RT-04 | v3-Feld `loop_max_us` während der Messfahrt | Schleifenzeit | < 10 ms | Median 97 µs; P95 6015 µs; **Max 6713 µs**; 0,00 % der Fenster über 10 ms | ✅ | Erfüllt, aber Prüfstandszahl unterschätzt um Faktor 10 |
| Regressionstest M-01 | FR-TL-06 Haltezeit im realen Signalverlauf | Host-Test, monoton abklingend 4,0 → 2,5 → 1,8 → 1,0 m/s² | Duty während der Haltezeit | > Grundwert bis 300 ms | grün | ✅ | Schließt die Testlücke, die den Mangel verdeckte |

**Nicht durch die Firmware validiert / Grenzen:** Lichtstärke in cd nach § 67 (photometrische Messung, separate Hardware-Eigenschaft) · Energiebilanz und Laufzeit unter realer Last (gerechnet, nicht gemessen) · Brown-Out unter realer LED-Lastspitze · Verlustleistung am AMS1117 · Wirkungsgrad des MT3608 unter Last · das korrigierte Verhalten nach M-01 im Feld (nur am Gerät beobachtet).

---

## 14. Validation Gap List

🔴 **kritisch**

- **Fehlerinjektion und Watchdog ohne Messprotokoll.** Beide Nachweise (SDA-Kurzschluss, `'H'`-Hang-Hook) existieren nur als Beobachtung im Fließtext der Project Bible und im Decision Log. Es gibt kein Protokoll mit Datum, Aufbau, Sollwert und gemessener Zeit — im Gegensatz zu den Bench-Experimenten A/B/C. Für Kap. 6.5 und 9.5 ist das die größte Lücke, weil die Robustheit dort tragend argumentiert wird. **Maßnahme:** entweder nachträglich aus den vorhandenen Logs ein Protokoll erstellen oder beide Versuche einmal kurz mit Protokollbogen wiederholen (Aufwand: unter einer Stunde).
- **`measurement_log.md` behauptet eine Validierungstiefe, die nicht erreicht wurde** (s. Teil 1, Befund 2). Solange die Aussage „`motion_filter` → `brake_curve` → `tail_light_fsm`" dort steht, widerspricht das Protokoll dem Feldtestbericht, der genau diese Lücke als Anlass nennt. Muss vor der Übernahme in Anhang C korrigiert werden.
- **Golden-Vektor ohne Provenienz** (Firmware-Hash fehlt; die frühere Zahl „41 gegen 43" verglich zwei verschiedene Zählweisen — richtig: 39 unterscheidbare Werte auf Firmware-Seite gegenüber allen 44 Feldern geprüft auf App-Seite [23 exakt, 21 mit Toleranz], s. Kap. 8). Der einzige geräteunabhängige Schnittstellennachweis ist ohne Angabe des Firmware-Stands nicht reproduzierbar.

🟠 **wichtig**

- **Korrigierte Firmware nicht im Feld nachgemessen.** M-01 ist behoben, aber die Wirksamkeit ist nur am Gerät beobachtet, nicht quantifiziert. Ein Vorher-Nachher-Diagramm wie Abb. 6 der Messfahrt-Auswertung wäre der saubere Abschluss. Eine dreiminütige Fahrt würde genügen. Bewusst abgegrenzt — aber es ist die billigste verbliebene Verbesserung der Nachweislage.
- **Nur eine Messfahrt nach Stufe 1.** Die Fehlauslösungsrate ist aus 177,86 s nicht belastbar hochrechenbar; der direkte Streckenvergleich zum 06.08. fehlt.
- **Ursache der 6,7 ms nicht getrennt.** Diskriminierungsversuch (Aufzeichnung mit und ohne Debug-Ausgaben) definiert, nicht durchgeführt.
- **Energiebilanz nur gerechnet.** NFR-PWR-01 hat keinen Messnachweis. Betrifft Kap. 9.4 stärker als Kap. 6.
- **Effektive Ansprechschwelle 2,13 m/s² nicht durch eine eigene Messung belegt**, sondern aus der Restdämpfung von 5,9 % gerechnet (Host-Simulation T11). Als gerechnete Größe kennzeichnen.

🟢 **optional**

- Reaktionszeit feiner als 10 ms auflösen (erfordert ein anderes Messverfahren, z. B. Oszilloskop am MOSFET-Gate).
- Bimodale 9/11-ms-Verteilung in Bench D erklären (derzeit als Hypothese geführt: höhere Serial-Last).
- BMP280-Temperatur im thermisch eingeschwungenen Zustand nachmessen.
- Dauerlauftest über mehrere Stunden (Stabilität, Speicherverhalten).

---

## 15. Evidence Map (Firmware-Teil)

| Technische Aussage | Möglicher Nachweis | Quelle/Dokument | Vorhanden? | Fehlt |
|---|---|---|---|---|
| Kennlinie ist linear und spezifikationskonform | Regression `pct = 26,66·decel − 33,32`, R² = 0,99984 | `bench_A_kennlinie_rampe.csv`, `abb_A_kennlinie.png` | ✅ | — |
| Reaktionszeit erfüllt NFR-RT-01 | ≤ 10 ms (B), 20 ± 10 ms (F) | `bench_B_…csv`, `abb_B_sprung.png`, `bench_run_notes.md` A6.3 | ✅ | — |
| Fail-Safe wirkt bei IMU-Ausfall | 20 % konstant bei `imu_health = FAILED` | `bench_C_failsafe.csv`, `abb_C_failsafe.png` | ✅ | — |
| Legacy-Filter löscht das Bremssignal | 3,924 → 0,000 gegen 3,9 → 3,836 | `bench_run_notes.md` (Vorher-Nachher-Tabelle D), Capture 3/4 | ✅ | Diagramm noch zu erstellen |
| Bremserkennung funktioniert im Feld | 9/9 erkannt, r = +0,85, 0/25 Fehlauslösungen | `Messfahrt_2026-08-08_Auswertung.md`, `abb1`–`abb7` | ✅ | — |
| Ursprüngliche Kette war funktionsunfähig | Duty-Verteilung 93–100 %, Einzelereignisse, 42 % Duty beim Neigen im Stand | `Feldtest_2026-08-06_Auswertung.md` | ✅ | Abbildungen F1–F6 noch zu erstellen |
| GNSS ist als Primärquelle nicht tragfähig | Datenblatt L86 (10 Hz, 200–400 ms Latenz) + Fahrt 5 (73 km/h im Stand bei FIX_OK, 15 Sats, HDOP 0,7) | `Feldtest_…`, Quectel L86 Hardware Design Tab. 1 S. 9 | ✅ | — |
| Schleifenzeit erfüllt NFR-RT-04 im Feld | `loop_max_us` Max 6713 µs, 0,00 % > 10 ms | Messfahrt-CSV, `abb5_zeitverhalten` | ✅ | — |
| Bias-Kalibrierung war real unerreichbar | STATIC-Anteil 39,2 % / 25,2 %; 0,392¹⁰⁰ ≈ 10⁻⁴¹ | Messfahrt-Auswertung Kap. 9.5.3 | ✅ | — |
| Sensorkonfiguration wirksam gesetzt | Registerrücklesung `CONFIG = 0x03`, `SMPLRT_DIV = 4` | `bench_run_notes.md`, Serial-Log | ✅ | Log-Auszug als Abbildung |
| Logik ist host-verifiziert | 126/126 Tests, 15 Testdateien | `pio test -e native`, Repo `firmware/test/` | ✅ | Testabdeckungstabelle je Modul |
| Schnittstelle Firmware ↔ App ist verifiziert | Golden-Vektor-Kreuztest | `testdata/frame_v3_golden.hex/.md` | ✅ | **Firmware-Hash und Feldanzahl** |
| I²C-Recovery wirkt | Fehlerinjektion SDA-Kurzschluss | Bible Kap. 9, `lessons_learned.md` | teilweise | **Messprotokoll** |
| Watchdog löst aus und setzt zurück | `'H'`-Hang-Hook, `esp_reset_reason()` | Bible Kap. 9, Roadmap M5 | teilweise | **Messprotokoll mit Zeitmessung** |
| Brownout-Ursache liegt im Altboard | zehn systematische Versuche, BOD-Abschalt-Test als Unterscheidungskriterium | `ble_brownout_fallstudie.md` | ✅ | — |
| Ressourcenbudget eingehalten | Flash 21,4 %, RAM 32,6 % | Build-Ausgabe Commit `835c7b3` | ✅ | — |
| Reproduzierbarkeit der Umgebung | `platformio.ini` mit gepinnten Versionen | Repo | ✅ | — |
| Effektive Ansprechschwelle 2,13 m/s² | Restdämpfung 5,9 %, Host-Simulation T11 | `stufe1_normgate.md` | teilweise | eigene Messung; als gerechnet kennzeichnen |
| Energiebilanz | — | Bible Kap. 5.2 (gerechnet) | ❌ | Messung unter Last |
| Lichtstärke nach § 67 | — | — | ❌ | photometrische Messung |

---

## 16. Informationslücken (Anforderung ↔ Umsetzung ↔ Validierung)

| Thema | Vorhanden | Fehlend | Relevanz | Maßnahme |
|---|---|---|---|---|
| FR-TL-06 Mindesthaltezeit | Anforderung, Umsetzung, Mangelnachweis, Behebung, Regressionstest | quantitativer Feldnachweis **nach** der Korrektur | 🟠 | dreiminütige Fahrt oder als Grenze ausweisen |
| FR-SNS-04/05 Robustheit | Anforderung, Umsetzung, funktionaler Nachweis | Messprotokoll der Fehlerinjektion | 🔴 | Protokoll nachziehen |
| FR-SAF-03 Watchdog | Anforderung, Umsetzung, funktionaler Nachweis | protokollierte Reset-Zeit | 🔴 | Protokoll nachziehen |
| FR-RF-03/04 Timing | Anforderung, Umsetzung, funktionaler Betrieb | gemessenes Wiederholintervall | 🟢 | abgegrenzt, im Ausblick nennen |
| NFR-PWR-01 Energie | Anforderung, Rechnung | Messung | 🟠 | Kap. 9.4, außerhalb der Firmware |
| FR-CFG-02/03 | Anforderung | Umsetzung **und** Validierung | 🟠 | als begründete Abgrenzung führen (Kap. 10.1) |
| NFR-RT-04 Ursache | Anforderung, Messung, Erfüllung | Ursachentrennung der 1-Hz-Spitze | 🟠 | zurückgenommen, Empfehlung im Ausblick |
| Erkennungsrate der Bremserkennung | 9/9 aus einer Fahrt | statistisch belastbare Rate über mehrere Fahrten | 🟠 | als Grenze in Kap. 10.3 |
| Fehlauslösungsrate | 1 Fehlauslösung in 177,86 s | belastbare Rate | 🟠 | als Grenze |
| Golden-Vektor-Provenienz | Vektor, Kreuztest | Firmware-Hash, einheitliche Feldanzahl | 🔴 | eintragen |
| Effektive Schwelle 2,13 m/s² | Rechnung, Host-Simulation | Messung am Board | 🟢 | als gerechnete Größe kennzeichnen |

**Muster-Lücken, die sich durch das Projekt ziehen** (für Kap. 10.3 verwertbar): (1) Funktional geprüft, aber nicht protokolliert — Fehlerinjektion und Watchdog. (2) Am Prüfstand geprüft, im Feld nicht — die Bench-Validierung war das Muster, das der Feldtest als unzureichend entlarvte, und dasselbe Muster wiederholt sich jetzt bei der M-01-Korrektur. (3) Behoben, aber nicht nachgemessen — dieselbe Struktur. Die Arbeit gewinnt an Glaubwürdigkeit, wenn dieses Muster ausdrücklich benannt statt kaschiert wird.

---

## 17. Informationen für andere Thesis-Kapitel

| Information | Zielkapitel | Begründung |
|---|---|---|
| Anforderungen FR-*/NFR-* als Gesamtsatz | **3.1/3.2**, vollständige SRS in **Anhang B** | Die SRS ist eine abgeleitete Spezifikation; Kap. 3 nennt die Systemanforderungen, nicht jede Firmware-ID |
| Architekturentscheidung V-B (IMU schnell, GNSS langsam) | **4.1** | Systemebene, nicht Firmware-Detail; in 6.3 nur referenzieren |
| Zustandsmodell als vier orthogonale Regionen | **4.3** (Konzept) und **6.2** (Umsetzung) | Die Gliederung trennt Konzept- und Umsetzungsebene bereits |
| Verteiltes Berechnungskonzept (Rohdaten in der Firmware, Kennzahlen in der App) | **4.1**, **7.2** | Schnittstellen- und Datenflusskonzept |
| BLE-Frame-Vertrag Schema v3 | **7.1** | Schnittstellenkapitel; Kap. 6.7 liefert nur die firmwareseitige Erzeugung |
| GNSS-Datenblattgrenzen (10 Hz, 200–400 ms, TTFF) | **2.5** (Grundlagen) und **6.3** (Entscheidungsgrundlage) | Grundlagenwissen vs. Anwendung im Entwurf |
| Komplementärfilter-Grundlagen, Madgwick/Mahony | **2.3** | Fachliche Grundlage des Normbetrags-Gates |
| Board-Tausch und Brownout-Fallstudie | **5.4** (Energieversorgung) und **9.5** (Systemrobustheit) | Ursache liegt im Spannungsregler, nicht in der Firmware |
| Pinbelegung, Schaltplan, Pull-Downs, SW1 | **5.5**, **Anhang A** | Hardware |
| LED-Treiberstufe, IRLZ44N, PWM-Frequenz 5 kHz | **5.3**, Grundlagen in **2.4** | Leistungselektronik |
| Alle Messergebnisse (Bench, Feldtest, Messfahrt) | **9.2**, **9.3**, **9.5** | Die Gliederung sieht ein eigenes Validierungskapitel vor |
| Befund G-01 (geschwindigkeitsabhängige Grundlinie) | **10.3** Grenzen, Ausblick Stufe 2 | Quantifizierte Grenze des Verfahrens |
| Mangel M-01 und seine Behebung | **6.3** (Behebung), **9.3** (Nachweis), **10.3** (Methodik) | Der Nachweiszyklus verteilt sich über drei Kapitel |
| Zielkonflikt FR-TL-07 gegen § 67 Abs. 4 | **2.2** (Recht), **10.2** (Zielkonflikte) | rechtlich-technischer Konflikt |
| „Debug-Setup ≠ Feldbedingung", Testlücke bei Hysterese, Beobachtereffekt der Diagnoseausgaben | **1.3** (Methodik) und **10.3** (Methodenkritik) | methodische Erkenntnisse, nicht Firmware-Inhalt |
| Umfangsschnitte vom 07.08. und 10.08.2026 | **10.1** (Soll-Ist), **11** (Ausblick) | Abgrenzung gehört in die Bewertung |
| KI-gestützte Entwicklung (Claude Code, `CLAUDE.md`, Prompts) | **Anhang E** | Pflichtdokumentation nach HSD-Richtlinie |

---

## 18. Cross-System-Schnittstelle zur iOS-App

```
MPU-6050 100 Hz ─► imu_driver ─► imu_mount_orientation ─► motion_filter ─┬─► brake_curve ─► tail_light_fsm ─► PWM GPIO26
                                                                          │
BMP280 1 Hz ─► bmp280_driver ─────────────────────────────┐               │
L86 1 Hz ─► gnss_driver ─► gnss_fix ─► gnss_speed_ref ────┤               │
SRX882S ─► rf_input ─► button_decoder ─► blinker_fsm ─► PWM GPIO25/27     │
                                                          ▼               ▼
                                            telemetry_window_agg (100-ms-Fenster)
                                                          ▼
                                    telemetry_frame (113 B) ─► telemetry_buffer ─► ble_telemetry
                                                          ▼  BLE Notify, 10 Hz, unidirektional
                                              iOS-App ─► SwiftData ─► CSV (35 Spalten) ─► Auswertung
```

- **Gemeinsamer Vertrag:** `BLE_Frame_v3_Schnittstelle.md` — 113 Byte, Little-Endian, gepackt, Offsets 0–80 identisch zu v2. Mehrere Felder sind nicht typ-ausgerichtet (die App muss `loadUnaligned` verwenden); das ist eine direkte Folge des Packings in der Firmware und in Kap. 6.7 oder 7.1 begründungspflichtig.
- **Verantwortungsteilung:** Die Firmware trifft alle sicherheitsrelevanten Entscheidungen (Bremslogik, Fail-Safe, Prioritätsordnung) und liefert Rohgrößen plus Diagnose. Die App **rechnet die Bremslogik nicht nach**; sie ist Datensenke, Persistenz und Auswertung. `brake_decel_ms2` ist der *Eingang* der Bremslogik, `brake_light_pct` der *kommandierte Ausgang* — die Validierung stellt beide synchron gegenüber.
- **Der wichtigste Systemintegrationsbefund:** Mangel M-01 wurde erst durch den 10-Hz-Validierungsmodus der App sichtbar. Die 1-Hz-Aufzeichnung des Feldtests hatte ihn durch Aliasing verdeckt. Ohne die App-seitige Erhöhung der Aufzeichnungsrate wäre der Firmware-Defekt unentdeckt geblieben. Das ist ein starkes Argument dafür, Mess- und Zielsystem gemeinsam zu entwerfen, und gehört in Kap. 9.1 (Validierungskonzept).
- **Zweiter Cross-System-Befund:** Ein Stromausfall setzt `millis()` in der Firmware zurück; die App rechnete daraus über eine vorzeichenlose 32-Bit-Differenz eine Fahrtdauer von 1191:44:03 h und 784 km. Firmwareseitig ist das kein Fehler, systemseitig schon: Die Uhr einer entfernten Quelle darf nicht als monotone Zeitbasis behandelt werden. Behoben in der App (`RecordingClock`).
- **Verifikation der Schnittstelle:** Golden-Vektor-Kreuztest (V3-3) statt beidseitiger Round-Trip-Tests.

---

## 19. Weitere thesisrelevante Erkenntnisse

- **Der Feldtest war ein geplanter Falsifikationsversuch, kein gescheiterter Nachweis.** Diese Rahmung ist wissenschaftlich der stärkere Ausgang und sollte im Text auch so geführt werden: Die Bench-Validierung hatte eine bekannte Lücke, der Feldtest wurde gezielt zu ihrer Schließung angelegt, und er hat einen messbar belegten Konstruktionsfehler offengelegt, dessen Behebung überprüfbar ist.
- **Grüne Tests beweisen nur, was die Testeingabe abdeckt.** M-01 überlebte 120 grüne Host-Tests, weil alle drei Haltezeit-Tests idealisierte Sprünge (5,0 → 0,0) einspeisen und das Hystereseband überspringen. Die Zeilen waren abgedeckt, der Betriebsbereich nicht. Konkrete Konsequenz für Zustandsmaschinen mit Hysterese: mindestens ein Testfall muss das Band **monoton durchlaufen**.
- **Das Messmittel muss selbst validiert werden.** Der Bench-Harness verfälschte durch einen Ein-Zeilen-Fehler (`next_tick = t0`) die Verankerung in zwei von sechs Experimenten — im Host-Test unsichtbar, weil dort ein konstantes `dt` eingespeist wird. Ebenso lagen drei Diagnoseausgaben im Messfenster der Zeitmessung, die sie beobachten sollten.
- **Zwei Vorgänge identischer Periode sind aus einer Zeitreihe nicht trennbar.** Gilt für Befund B7 und ist als methodische Aussage allgemein verwertbar.
- **Abstraktionsschichten können im Fehlerfall stillschweigend versagen.** Zwei Beispiele aus einer Codebasis: `Adafruit_MPU6050::begin()` schreibt DLPF und Sample-Rate nie und reicht I²C-Fehler nicht durch; `Wire.end()` scheitert bei blockiertem Bus ohne Rückmeldung und lässt die Pins PeriMan-besetzt. Beide wurden erst durch Lesen des Bibliotheks-Quelltexts gefunden, nicht durch Beobachtung.
- **Ein scheinbarer Filterdefekt ließ sich vollständig auf eine einzelne unkompensierte Sensorgröße zurückführen**, und die zuvor aufgestellte Beziehung ε = b·τ sagte den gemessenen Wert korrekt voraus (−4,61 °/s → 18,4° → 3,1 m/s² gegenüber gemessenen 3,0–3,2).
- **Ressourcenbild:** Die CPU ist zu über 99 % unbeschäftigt, Flash zu 21,4 % und RAM zu 32,6 % belegt. Der ESP32 ist für diese Aufgabe deutlich überdimensioniert — eine ehrliche Aussage für Kap. 10.1, die zugleich die Wahl rechtfertigt (BLE-Stack, Entwicklungskomfort, Verfügbarkeit).
- **Quellenkritik als Eigenleistung:** In der Literaturliste ist ein Datenblattfehler dokumentiert (AD0-Logik beim GY-521). Solche Befunde sind für die Bewertung wertvoll.

---

## 20. Übergabe-Checkliste

1. **Dokumentenstatus** — Teil 1, mit sechs kritischen Befunden.
2. **Finaler Firmwarestand** — Punkt 2, getrennt nach implementiert / abgegrenzt / verworfen / offen.
3. **Firmware-Anforderungen** — Punkt 3, Nachweis-Matrix mit 30 IDs.
4. **Architektur** — Punkte 4 und 5, mit Begründungen statt Beschreibungen.
5. **Bremserkennung** — Punkt 6, das Kernnarrativ.
6. **Zustandslogik** — Punkt 7.
7. **Telemetrie und BLE** — Punkt 8.
8. **Robustheit und Zeitverhalten** — Punkt 9.
9. **Technische Entscheidungen** — Punkt 10, 20 Einträge mit Alternativen.
10. **Probleme und Lösungsfindung** — Punkt 11, zwölf Fälle.
11. **Kapitelstruktur Kap. 6** — Punkt 12, acht Unterkapitel mit Inhalt, Tabellen, Abbildungen, Nachweisen.
12. **Validierungsübersicht** — Punkt 13, 16 Tests mit Soll- und Istwerten.
13. **Validation Gap List** — Punkt 14, ampelpriorisiert.
14. **Evidence Map** — Punkt 15, 19 Aussagen mit Nachweisstatus.
15. **Informationslücken** — Punkt 16, plus drei Muster-Lücken.
16. **Andere Thesis-Kapitel** — Punkt 17, 17 Zuordnungen.
17. **Cross-System-Schnittstelle** — Punkt 18.
18. **Weitere Erkenntnisse** — Punkt 19.
19. **Relevante Dokumente** — Teil 1, Abschnitte A bis F.
20. **Offene Fragen an den Verfasser:** Wird `measurement_log.md` korrigiert oder mit Vorbehalt in Anhang C übernommen? Werden Fehlerinjektion und Watchdog nachträglich protokolliert? Wird eine kurze Verifikationsfahrt nach der M-01-Korrektur gefahren? Welche Zählweise gilt für den Golden-Vektor (41 oder 43)?
21. **Empfohlene Nachweise**, nach Aufwand geordnet: Golden-Vektor-Hash eintragen (Minuten) · `measurement_log.md` korrigieren (Minuten) · Fehlerinjektion und Watchdog protokollieren (unter einer Stunde) · dreiminütige Verifikationsfahrt nach M-01 (halbe Stunde) · Energiebilanz unter Last messen (halber Tag, Kap. 9.4).

**Wissenschaftlicher Maßstab.** Die spätere Darstellung folgt durchgängig der Kette Problem/Anforderung → technische Entscheidung → Begründung → Umsetzung → Validierung → Ergebnis, nicht der Chronologie der Programmierung. Dieses Paket ist entlang derselben Kette aufgebaut, damit die Umsetzung im Schreibchat ohne Umsortieren möglich ist.

**Es sind keine fertigen Kapiteltexte enthalten.** Auftragsgemäß beschränkt sich dieses Dokument auf Konsolidierung, Bewertung der Thesis-Relevanz, Kapitelstrukturierung, Nachweisidentifikation und Lückenanalyse.
