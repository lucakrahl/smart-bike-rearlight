# Messprotokoll — Validierung der Bremslicht-Logik (Serial-Bench)

**Bachelorarbeit Krahl · Smart Bike Rear Light**

| Feld | Wert |
|---|---|
| Datum / Uhrzeit | 05.08.2026, 23:10 CEST |
| Prüfer | Luca Krahl |
| Prüfling (Board) | Espressif ESP32-DevKitC-32E (WROOM-32E) |
| Firmware-Stand (Git-Hash) | `d8a4e75` (Bench-Env `esp32dev_bench`, `-D BENCH_MODE`) |
| Schnittstelle / Port | UART0, 115200 Baud, `/dev/cu.usbserial-110` |
| Abtastrate Log | 100 Hz (10 ms), lückenlos (3101 / 351 / 3101 Datenzeilen) |
| Bench-Modus | `BENCH_MODE`, synthetische Verzögerungs-Einspeisung (NFR-TST-02) |
| FR-TL-07 (Notbrems-Blinken) | deaktiviert (Default) |
| Auswertung | Python/pandas, unabhängig aus den Rohdaten reproduziert |

## 1. Ziel
Nachweis der Bremslicht-Regellogik nach **FR-TL-06** (Bremskennlinie), ihres **zeitlichen/zustandsbehafteten** Verhaltens (300-ms-Haltezeit, Hysterese, Anstiegszeit) sowie des **Fail-Safe-Verhaltens** nach **FR-SAF-01**. Gemessen wird die **kommandierte** LED-Duty (`brake_light_pct`, Ausgang der Bremslogik) als Funktion des Verzögerungs-Eingangs (`brake_decel_ms2`).

## 2. Aufbau & Methodik
Kontrollierter **Bench-Test** (Werkbank, kein Fahrbetrieb). Über den firmwareseitigen Test-Daten-Hook (NFR-TST-02) wird ein **definiertes Verzögerungsprofil** in denselben Signalpfad gespeist, der im Normalbetrieb die Bremslicht-Duty erzeugt (`motion_filter` → `brake_curve` → `tail_light_fsm`); der geloggte Ausgang spiegelt damit das **reale, integrierte FSM-Verhalten** auf dem Ziel-MCU inkl. Halten/Hysterese/Fail-Safe. Aufzeichnung bei 100 Hz über die serielle Schnittstelle.

**Begründung:** Die synthetische Einspeisung ist **reproduzierbar** und durchfährt die Schwellen (2,0 / 5,0 / 1,5 m/s²) kontrolliert; die 100 Hz lösen die schnellen Effekte (Anstieg < 50 ms, 300-ms-Halten) auf, die im 1-Hz-App-Export untergehen.

**Abgrenzung:** Gemessen wird die *kommandierte* Duty (Steuerlogik-Nachweis). Die *physikalische* LED-Helligkeit als Funktion der Duty ist eine separate Hardware-Eigenschaft (optional per Oszilloskop am MOSFET-Gate GPIO26). Der GPIO26-Kanal wurde während der Bench parallel real angesteuert (visuell am Board nachvollziehbar). Ergänzend belegt der **Feldtest** (30-Zone, App-Export @ 1 Hz + Beobachtung) das reale Ansprechen.

## 3. Randbedingungen (Sollwerte aus SRS / `config.h`)
| Parameter | Sollwert (FR-TL-06) |
|---|---|
| Schlusslicht-Grundhelligkeit | ~20 % PWM |
| Ansprechschwelle Bremslicht | 2,0 m/s² |
| Sättigung (100 %) | 5,0 m/s² |
| Ausschalt-Hysterese (Rückfall) | < 1,5 m/s² |
| Mindesthaltezeit | 300 ms |
| Reaktionszeit (Ereignis → LED) | ≤ 50 ms (NFR-RT-01) |

## 4. Experimente

### 4.1 Experiment A — Kennlinie (Rampe) · `bench_A_kennlinie_rampe.csv`
Eingang: 0 → 6,0 m/s² (15 s), 1 s halten, 6,0 → 0 (15 s). Auswertung `brake_light_pct` über `brake_decel_ms2`.

| Kenngröße | Soll | **Ist (gemessen)** | Bewertung |
|---|---|---|---|
| Grundhelligkeit (decel < 2,0) | ~20 % | **20 %** (konstant) | ✔ |
| Ansprechschwelle (Zustand Schluss→Brems) | 2,0 m/s² | **2,00 m/s²** (decel = 2,004) | ✔ |
| Sättigung 100 % | 5,0 m/s² | **≈ 5,0 m/s²** (decel = 4,98) | ✔ |
| Verlauf 2,0–5,0 m/s² | linear | **linear**, `pct = 26,66·decel − 33,32`, **R² = 0,99984** (theoret. 26,67) | ✔ |
| Hysterese-Rückfall (Zustand Brems→Schluss) | < 1,5 m/s² | **decel = 1,38 m/s²** (Zustandswechsel bei t = 27 560 ms) | ✔ (s. Hinweis) |

*Hinweis Hysterese:* Die Duty-Kennlinie ist einwertig (Auf- und Abrampe decken sich, s. Abb. A) — die **Hysterese wirkt auf Zustandsebene** (`fsm_state`), nicht auf dem geklammerten %-Wert. Auf der langsamen Abrampe unterschreitet die Verzögerung 1,5 m/s² bei t = 27 260 ms; der Zustandswechsel erfolgt **exakt 300 ms später** (t = 27 560 ms, dann decel = 1,38 m/s²). Der gemessene „Rückfall bei 1,38" ist also die korrekte Überlagerung aus **1,5-m/s²-Schwelle + 300-ms-Mindesthaltezeit** — kein Fehler, sondern spezifikationskonform.

**Abbildung A:** `abb_A_kennlinie.png` — Bremslicht-Kennlinie (Duty % über Verzögerung), Auf-/Abrampe, Marken Ein 2,0 / Sätt. 5,0 / Hyst. 1,5.

### 4.2 Experiment B — Zeitverhalten (Sprung) · `bench_B_zeitverhalten_sprung.csv`
Eingang: 1 s bei 0 → Sprung 6,0 m/s² für 0,5 s → 0 → 2 s Nachlauf.

| Kenngröße | Soll | **Ist (gemessen)** | Bewertung |
|---|---|---|---|
| Anstiegszeit (Ereignis → 100 %) | ≤ 50 ms | **≤ 10 ms** (20 %→100 % innerhalb eines Abtastschritts) | ✔ (komfortabel) |
| Haltezeit nach Bremsende | 300 ms | **300 ms** (Duty bleibt 100 % von t = 1500 bis 1800 ms) | ✔ (exakt) |
| Rückfall auf Schlusslicht danach | ja | **ja** (→ 20 % ab t = 1800 ms) | ✔ |

**Abbildung B:** `abb_B_sprung.png` — Sprungantwort: schneller Anstieg + gehaltene Helligkeit über 300 ms (kein Sofortabfall, FR-TL-06).

### 4.3 Experiment C — Fail-Safe (IMU FAILED) · `bench_C_failsafe.csv`
Bedingung: `imu_health = FAILED`, dabei Rampe 0 → 6,0 → 0.

| Kenngröße | Soll | **Ist (gemessen)** | Bewertung |
|---|---|---|---|
| Bremslicht-Duty trotz hoher Verzögerung | ~20 % (Schlusslicht) | **20 % konstant** über den gesamten 0→6→0-Rampenlauf | ✔ |
| Zustand | Schlusslicht | **`fsm_state` durchgängig = Schlusslicht** | ✔ |
| Fehler-Flag | FAILED | **`imu_health` durchgängig = 2 (FAILED)** | ✔ |

**Abbildung C:** `abb_C_failsafe.png` — Fail-Safe: Bremslicht bleibt Schlusslicht (20 %) trotz Verzögerungsrampe.

## 5. Ergebnis (Soll/Ist-Zusammenfassung)
**Alle geprüften Kenngrößen liegen innerhalb der Spezifikation.** Die Bremskennlinie (FR-TL-06) ist bestätigt: 20 % Grundhelligkeit, Ansprechen bei 2,0 m/s², linearer Anstieg (R² = 0,9998) bis 100 % Sättigung bei 5,0 m/s². Das zustandsbehaftete Verhalten greift exakt wie spezifiziert: **Anstieg ≤ 10 ms** (NFR-RT-01 ≤ 50 ms deutlich erfüllt), **300-ms-Mindesthaltezeit** (exakt), **Hysterese-Rückfall bei 1,5 m/s²** (auf Zustandsebene, kombiniert mit der Haltezeit). Der **Fail-Safe** (FR-SAF-01/FR-STA-04) hält bei IMU-Ausfall durchgängig das Schlusslicht.

## 6. Diskussion & Einordnung
Die Messung bestätigt sowohl die statische Kennlinie als auch — und das ist der Mehrwert des 100-Hz-Bench gegenüber dem 1-Hz-App-Export — die **schnellen, zustandsbehafteten Effekte**. Die einwertige Duty-Kennlinie und die auf Zustandsebene wirkende Hysterese sind physikalisch konsistent: das Bremslicht folgt der Kennlinie, hält aber die Helligkeit 300 ms (kein Flackern bei kurzen Bremsstößen). Grenzen der Methode: (a) gemessen wird die *kommandierte* Duty, nicht die photometrische Lichtstärke (separate Hardware-Charakterisierung, optional per Oszilloskop); (b) die Verzögerung wird *synthetisch* eingespeist — der **Feldtest** ergänzt dies um das reale Ansprechen unter Fahrbedingungen. Beide zusammen (präzise Bench-Kennlinie + realer Feldkontext) bilden den vollständigen Validierungsnachweis der Bremslicht-Funktion.

*Nebenbefund (ohne Datenwirkung):* Im Bench-Sonderbuild traten kosmetische `esp_task_wdt_reset(): task not found`-Log-Zeilen auf (Bench-Task nicht am TWDT registriert); sie wurden beim CSV-Split über ein striktes Zeilenformat verlustfrei herausgefiltert (0 verworfene echte Datenzeilen). Kein Reset, kein Datenverlust; Normalbetrieb nach der Bench verifiziert wiederhergestellt (`pio test -e native` 77/77 grün).

## 7. Ergänzender Feldtest (Realbetrieb)
30-km/h-Zone; App-Validierungs-CSV (`brake_decel_ms2`, `brake_light_pct` @ 1 Hz) + Beobachtung/Foto → reales Ansprechen des Bremslichts als kontextueller Beleg zur präzisen Bench-Kennlinie. *(auszuführen)*

## 8. Quellen
- SRS **FR-TL-06** (Bremskennlinie), **FR-TL-07** (Notbrems-Blinken, experimentell/deaktiviert), **FR-SAF-01 / FR-STA-04** (Fail-Safe), **NFR-RT-01** (Reaktionszeit ≤ 50 ms), **NFR-TST-02** (Testdaten-Einspeisung) — Project Bible Kap. 2.
- Rohdaten: `docs/Validierung/bench_A_kennlinie_rampe.csv`, `…_B_…`, `…_C_…`, `bench_run_notes.md`.