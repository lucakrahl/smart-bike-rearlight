# Bench-Run-Notizen — Bremslicht-Logik-Validierung

> Von Claude Code erzeugt (NFR-TST-02). Dokumentiert einen konkreten Bench-Lauf;
> siehe `firmware/src/main.cpp` (`#ifdef BENCH_MODE`) für die Implementierung.

## Lauf-Metadaten
- **Datum/Uhrzeit:** 2026-08-05, 23:10 CEST
- **Port:** `/dev/cu.usbserial-110` (CP2102N USB-to-UART, per `pio device list` ermittelt)
- **Board:** ESP32-DevKitC-32E (WROOM-32E)
- **Firmware-Hash:** `d8a4e75` (HEAD zum Zeitpunkt des Bench-Builds; `firmware=d8a4e75` in der
  `# META`-Zeile jeder CSV bestätigt)
- **Build-Env:** `esp32dev_bench` (`platformio.ini`, `extends = env:esp32dev` + `-D BENCH_MODE`),
  Git-Hash per `PLATFORMIO_BUILD_FLAGS="-D FIRMWARE_GIT_HASH=\"d8a4e75\""` injiziert
- **FR-TL-07 (Notbrems-Blinken):** deaktiviert (Default, `ESS_ENABLED_DEFAULT=false`)
- **Gesamtlaufzeit Bench (Boot → `# BENCH DONE`):** 66,0 s

## Erzeugte Dateien
| Datei | Zeilen (Header + Daten) | Beschreibung |
|---|---|---|
| `bench_A_kennlinie_rampe.csv` | 3103 (1 META + 1 Header + 3101 Daten) | Rampe 0→6,0→0 m/s², 31 s |
| `bench_B_zeitverhalten_sprung.csv` | 353 (1 META + 1 Header + 351 Daten) | Sprung 0→6,0→0 m/s², 3,5 s |
| `bench_C_failsafe.csv` | 3103 (1 META + 1 Header + 3101 Daten) | Rampe wie A, `imu_health` auf FAILED erzwungen |

Alle drei Dateien vollständig (Zeilenzahl entspricht exakt der erwarteten Anzahl 10-ms-Schritte
über die jeweilige Experimentdauer, keine Lücken). CSV-Split hat funktioniert — der
Rohdaten-Fallback (`bench_rohdaten.md`) war nicht nötig.

## Validierungsbefund (Kurzfassung, Details s. CSVs)
- **A — Kennlinie:** Ansprechschwelle exakt bei `decel_ms2 > 2,000` (t=5000 ms noch 20 %/Taillight,
  t=5010 ms bereits Brakelight); Sättigung 100 % ab `decel_ms2 = 5,000` (t=12500 ms); Hysterese-
  Rückfall unter 1,5 m/s² mit exakt 300 ms Mindesthaltezeit (`below_off_since_ms_` bei t=27260 ms,
  Rückfall auf Taillight bei t=27560 ms → 300 ms, deckt sich exakt mit `BRAKE_MIN_HOLD_MS`).
- **B — Zeitverhalten:** Anstieg 20 %→100 % innerhalb eines einzelnen 10-ms-Samples (< 50 ms,
  NFR-RT-01 komfortabel erfüllt); nach Rückfall auf 0 m/s² (t=1500 ms) hält die FSM 100 % exakt bis
  t=1800 ms (300 ms Mindesthaltezeit), danach Rückfall auf 20 %.
- **C — Fail-Safe:** `imu_health` zeigt über den gesamten Lauf ausschließlich den Wert `2`
  (FAILED); `brake_light_pct` bleibt trotz identischem 0→6,0→0-Rampeneingang (Spalte `decel_ms2`)
  konstant bei `20` (Schlusslicht) — Fail-Safe-Gate (FR-STA-04) greift durchgehend.

## Auffälligkeiten
- **`esp_task_wdt_reset(): task not found`-Fehlermeldungen im Rohmitschnitt** (ca. 10.478 Zeilen,
  zwischen den CSV-Datenzeilen verstreut): Der Bench-Task ist nie beim Task-Watchdog registriert,
  da `enableLoopWDT()` im `BENCH_MODE`-Zweig bewusst nicht aufgerufen wird (Bench läuft komplett in
  `setup()`, ruft aber vorsorglich `esp_task_wdt_reset()` pro 10-ms-Zyklus, um einem eingeschalteten
  TWDT vorzubeugen). Funktional folgenlos — kein Reset, kein Datenverlust, Lauf erreichte `# BENCH
  DONE` sauber; die Fehlerzeilen wurden beim CSV-Split über ein striktes Zeilenformat
  (`^\d+,-?\d+\.\d{3},\d+,\d+,\d+$`) zuverlässig herausgefiltert (10.478 verworfene Nicht-Daten-
  zeilen inkl. Marker/Header/Rauschen, 0 verworfene echte Datenzeilen — Zeilenzahlen stimmen exakt).
  Kein Fix erforderlich, da rein kosmetisch und nur im `BENCH_MODE`-Sonderbuild auftretend.
- Physischer Bremslicht-Pin (GPIO26) wurde während der Bench parallel live angesteuert
  (`drivers::setDutyPercent`) — visuell am Board nachvollziehbar, aber nicht separat protokolliert.

## Aufräumen
- `pio test -e native`: 77/77 grün (unverändert, reine Logik nicht angefasst).
- `pio run -e esp32dev`: grün, RAM 26,7 % / Flash 21,4 % (identisch zum Stand vor der Bench).
- Board mit Normal-Firmware (`esp32dev`-Env, `BENCH_MODE` aus) zurückgeflasht und Boot verifiziert:
  IMU/BMP280/GNSS ready, BLE-Advertising + Reconnect, `[R1/R2] duty=20%`, kein Brownout — Normal-
  betrieb bestätigt wiederhergestellt.

---

# Schritt A — Bench D/E/F auf der Hardware (2026-08-07)

> Board: Espressif ESP32-DevKitC-32E, Port `/dev/cu.usbserial-140`, Env
> `esp32dev_bench`. Rohdateien (nicht committet, Dateinamen mit Datum):
> `bench_capture1_diaglog_off_20260807.raw.log` (A–F, `MOTION_DIAG_LOG_ENABLED=false`),
> `bench_capture2_diaglog_on_20260807.raw.log` (A–F, `MOTION_DIAG_LOG_ENABLED=true`,
> Vollraten-Diagnose für D/E/F). Beide vollständig bis `# BENCH DONE`
> mitgeschnitten (Reset synchron zum Capture-Start per DTR/RTS-Toggle, damit
> die Aufzeichnung garantiert ab t=0 beginnt).

## A1 — Alternation gegenprüft (vor dem Lauf)

Amplitude ±0,001 m/s² (Peak-Peak 0,002), alterniert jedes Sample:
- accel_norm-Schwankung ≈ ±0,001 m/s² — Faktor **120** unter
  `MOTION_NORM_STATIC_BAND=0,12`.
- Jerk zwischen aufeinanderfolgenden Samples: `0,002/0,01·0,01=0,002 m/s²/10ms`
  — Faktor **1000** unter `MOTION_NORM_JERK_DELTA=2,0`.

Beide Abstände weit über Faktor 10 → kein Stopp nötig, Kunstgriff bestätigt
unschädlich. Als Kommentarblock in `test_motion_filter.cpp` (T16) und
`main.cpp` (`benchAzJitter()`) festgehalten.

## dt_s-Statistik je Experiment (On-Device-`# DT_STATS`, beide Captures deckungsgleich)

| Experiment | min | max | mean | n | Clamp-Treffer (1..50 ms) |
|---|---|---|---|---|---|
| A Kennlinie/Rampe | 9,000 ms | 11,000 ms | 10,000 ms | 3100 | — (Alt-Pfad, nicht mitgezählt) |
| B Zeitverhalten/Sprung | 9,000 ms | 11,000 ms | 10,000 ms | 350 | — |
| C Fail-Safe | 9,000 ms | 11,000 ms | 10,000 ms | 3100 | — |
| D Scheinneigung | 9,000 ms | 11,000 ms | 9,999 ms | 700 | 0 |
| E Stoßunterdrückung | 9,000 ms | 11,000 ms | 10,002 ms | 400 | 1 |
| F Neigung+Bremsung | 9,000 ms | 11,000 ms | 10,000 ms | 600 | 1 |

**Standardabweichung nicht verfügbar:** Das Board loggt nur min/max/mean pro
Experiment, keine Rohserie der einzelnen `dt_s`-Werte — eine echte Std lässt
sich aus den Captures nicht rekonstruieren. Da `min`/`max` in jedem
Experiment exakt bei 9/11 ms liegen (ESP32-`millis()`-Granularität um den
10-ms-Nominaltakt), ist die Streuung nachweislich auf ±1 ms begrenzt; eine
Std-Zahl würde eine Genauigkeit vortäuschen, die die Daten nicht hergeben.
Für eine exakte Std wäre ein weiterer Capture mit Rohserie nötig (außerhalb
A1–A5, nicht durchgeführt).

**Clamping (1..50 ms):** griff nur je 1× in E und F (vermutlich beim
allerersten Sample, s. u.), 0× in D. Kein Hinweis auf wiederholte Ausreißer
im laufenden Betrieb.

## Worst-Case-Schleifenzeit vs. < 10 ms (NFR-RT-04)

**Einschränkung:** `BENCH_MODE` ersetzt `setup()`/`loop()` vollständig durch
eine Busy-Wait-Schleife mit festem 10-ms-Ziel — das ist NICHT der reale
kooperative Scheduler-`loop()` aus dem Normalbetrieb, misst also nicht
direkt NFR-RT-04. Als Näherung: `max_ms=11,0` über alle sechs Experimente
(nie mehr als +1 ms über dem 10-ms-Ziel) zeigt, dass die Verarbeitung pro
Sample (inkl. `motion_filter`+`imu_health`+`tail_light_fsm`+Serial-Ausgabe)
den Takt nirgends spürbar überschreitet. Eine echte NFR-RT-04-Messung am
Normalbetrieb-`loop()` (Env `esp32dev`) steht separat aus.

## Reaktionszeit Bremsbeginn → Bremslicht-PWM (gegen NFR-RT-01 ≤ 50 ms)

| Experiment | Bremsbeginn (Profil) | erster `duty>20%` | Reaktionszeit | Bewertung |
|---|---|---|---|---|
| B (Sprung 6,0 m/s²) | t=1000 ms | t=1000 ms | **0 ms** | ✅ deutlich unter 50 ms |
| F (Gefälle+Bremsung) | t=2000 ms | t=2020 ms | **20 ms** | ✅ unter 50 ms |
| D (Scheinneigung) | t=1000 ms | t=2000 ms | **1000 ms** | ⚠️ kein NFR-RT-01-Verstoß der Bremslogik — Ursache s. u. |

D zeigt scheinbar eine grobe Verletzung von NFR-RT-01. Die Ursache ist
**keine Latenz von `motion_filter`/`tail_light_fsm`**, sondern ein
Bench-Harness-Artefakt in der Verankerungslogik, s. nächster Abschnitt.

## Abweichung Hardware vs. Host-Test — benannte Ursache (D und E)

**Befund:** `# BIAS_CAL` zeigt `calibrated=0` für D und E (in beiden
Captures reproduzierbar), aber `calibrated=1` für F.

**Ursache (nicht nur Differenz, sondern erklärt):** `runFullChainExperiment()`
initialisiert `t_prev_real=t0` und startet `next_tick=t0`; beim allerersten
Schleifendurchlauf wartet die Busy-Wait-Bedingung `millis()-next_tick<0`
nicht (sie ist beim Start bereits erfüllt), wodurch das erste `dt_s` real
nur ≈0..1 ms misst statt der nominellen 10 ms — auf 1 ms geklemmt
(`dt_s<0,001f`). Der erste Beitrag zum Verankerungsfenster
(`anchor_window_elapsed_s_`) ist dadurch um ≈9 ms zu klein.

Das bleibt folgenlos, solange die STATIC-Vorlaufzeit eines Profils
komfortablen Abstand zu `MOTION_ANCHOR_WINDOW_S=1,0 s` hat (F: 2000 ms
Vorlauf vor der ersten Bremsung → `calibrated=1`). D und E haben dagegen
eine STATIC-Vorlaufzeit von **exakt** 1000 ms (D: „1 s Ruhe" vor der
Bremsung; E: erster Stoß exakt bei t=1000 ms) — ohne jede Marge gegen das
1,0-s-Fenster. Die ≈9-ms-Lücke aus dem ersten Sample genügt, damit das
Fenster nicht rechtzeitig schließt; die anschließende Nicht-STATIC-Probe
(Bremsung/Stoß bei genau t=1000 ms) reißt das noch offene Fenster ab, bevor
es fertig wird. Die Verankerung fällt dadurch auf den
`MOTION_ANCHOR_TIMEOUT_S=2,0 s`-Fallback zurück (ebene Lage annehmen, Bias
bleibt 0) — sichtbar an `t_real_ms=0` in `# BIAS_CAL` (kein Fensterabschluss,
nur der Timeout hat gegriffen) und daran, dass `neu` in D bis t=2000 ms
exakt 0 bleibt (Ausgang während der Unverankert-Phase sicherheitshalber
gehalten, s. `motion_filter.cpp`) und erst am Timeout springt.

**Das ist im Host-Test (`pio test -e native`) nicht sichtbar**, weil
`anchorLevel()` dort mit einem exakt konstanten synthetischen `dt=0,01f`
rechnet — ohne das reale `millis()`-Timing des Bench-Harnesses tritt die
9-ms-Lücke dort nie auf.

**Einordnung:** Das ist ein Fehler im **Bench-Harness**
(`main.cpp`/`runFullChainExperiment()`/Profile D und E), nicht in
`motion_filter` selbst — die 1000-ms-Vorlaufzeiten in den Profilen D/E
wurden ohne Marge gegen das Verankerungsfenster gewählt. Nicht behoben
(außerhalb A1–A5); Vorschlag für einen künftigen Schritt: Vorlaufzeiten in
D/E auf z. B. 1200–1500 ms verlängern, oder das erste `dt_s` im Harness
korrekt aus der tatsächlich verstrichenen Zeit seit Task-Start berechnen
statt aus `t0==t_prev_real`.

## Regime-Verteilung (aus Capture 2, Vollraten-Diagnose)

| Experiment | STATIC | DYNAMIC | SHOCK |
|---|---|---|---|
| D Scheinneigung | 42,9 % | 57,1 % | 0,0 % |
| E Stoßunterdrückung | 98,0 % | 0,0 % | 2,0 % |
| F Neigung+Bremsung | 66,7 % | 33,3 % | 0,0 % |

F entspricht exakt der Profilkonstruktion (4000 ms reine Neigung / 6000 ms
gesamt = 66,7 % STATIC, 2000 ms Bremsphase = 33,3 % DYNAMIC, kein Stoß →
0 % SHOCK) — Regime-Klassifikation verhält sich auf der Hardware
deckungsgleich zur Auslegung. E: die drei Stöße (~20–30 ms) erreichen
zuverlässig SHOCK, die Konstantfahrt dazwischen bleibt durchgehend STATIC
(kein DYNAMIC-Zwischenzustand) — E enthält keine moderate Bremsung, daher
0 % DYNAMIC.

## D: neues Filter vs. Legacy-Filter (Vorher-Nachher)

| t seit Start | neu (m/s²) | Legacy (m/s²) |
|---|---|---|
| 500 ms (Ruhe) | 0,000 | 0,000 |
| 1000 ms (Bremsbeginn) | 0,000¹ | 3,924 |
| 1500 ms | 0,000¹ | 1,583 |
| 2000 ms | 3,999 | 0,758 |
| 3000 ms | 3,877 | 0,357 |
| 4000 ms | 3,756 | 0,304 |
| 5000 ms (Bremsende) | 3,640 | 0,000 |
| 6000–7000 ms (Ruhe) | 0,000 | 0,000 |

¹ Zeitraum 1000–2000 ms ist durch den oben beschriebenen
Verankerungs-Timing-Bug verfälscht (Ausgang sicherheitshalber auf 0
gehalten, nicht repräsentativ für das eingeschwungene Verhalten — s.
Host-Test T2 mit sauberem 1-s-Vorlauf: dort liegt der Ausgang bereits ab
Bremsbeginn nahe 4,0). Der eigentliche Befund bleibt aber eindeutig: das
Legacy-Filter kollabiert innerhalb von ~1 s auf einen Bruchteil des realen
Bremswerts und erreicht bei anhaltender Bremsung 0,000 (klassische
Scheinneigung, deckt sich mit dem Feldtest-Befund); das neue Filter hält
sich nach der Verankerung durchgehend zwischen 3,6 und 4,0.

## Zusammenfassung Schritt A

- Kein Schwellwert (`MOTION_NORM_STATIC_BAND`/`_JERK_DELTA`/`_SHOCK_DELTA`)
  wurde in diesem Schritt verändert (A5).
- Regime-Klassifikation, Stoßunterdrückung und die grundsätzliche
  Vorher-Nachher-Wirkung der Filterüberarbeitung sind auf echter Hardware
  bestätigt.
- Ein klar benannter Bench-Harness-Bug (D/E-Vorlaufzeit ohne Marge gegen
  das Verankerungsfenster) verfälscht D's scheinbare Reaktionszeit und den
  Beginn der Vorher-Nachher-Tabelle — nicht die Bremslogik selbst.
- Offen: echte Std der `dt_s`-Verteilung, NFR-RT-04 am Normalbetrieb-`loop()`,
  Korrektur der D/E-Vorlaufzeiten bzw. des Harness-Erstsample-Fehlers.

---

# Schritt A6 — Nachlauf Bench (2026-08-07, nach Harness-Fix)

> Rohdateien: `bench_capture3_A61_diaglog_off_20260807.raw.log` (A–F,
> `MOTION_DIAG_LOG_ENABLED=false`), `bench_capture4_A61_diaglog_on_20260807.raw.log`
> (A–F, `MOTION_DIAG_LOG_ENABLED=true`). Beide vollständig bis `# BENCH DONE`.

## A6.1 — Harness-Fix

a) `runFullChainExperiment()`: `next_tick` startete bislang bei `t0` statt
`t0+10`, wodurch die Busy-Wait-Bedingung beim allerersten Durchlauf sofort
erfüllt war (kein Warten) und das erste `dt_s` real nur ≈0–1 ms maß statt
10 ms — auf 1 ms geklemmt. Behoben: `next_tick = t0 + 10u`. Zusätzlich
wurde jedes Clamp-Ereignis um eine `# DT_CLAMP_EVENT`-Zeile
(Sample-Index, `t_ms`, reales `dt_ms`, Ursache) ergänzt (für A6.5), und
dem `# DIAG`-Format ein `dt_real_ms`-Feld hinzugefügt (für A6.4).

b) Vorlaufzeit D/E von 1000 ms auf 2500 ms erhöht (150 % Reserve gegen
`MOTION_ANCHOR_WINDOW_S=1,0 s`); Experimentdauern entsprechend angepasst
(D: 7000→8500 ms, E: 4000→5500 ms, Stoßabstände in E unverändert bei
1000 ms). F unverändert.

## A6.2 — D/E erneut gefahren: Ergebnis vs. Erwartung

| Erwartung | Ergebnis |
|---|---|
| `bias_calibrated=1` in D und E | ✅ beide `calibrated=1` (Capture 3 **und** 4, reproduzierbar) |
| `tau_slow=90 s` statt 30 s | ✅ `tau_slow_s=90.0` in D, E, F |
| D nach 4 s Dauerbremsung ≈3,83 statt 3,53 | ✅ **3,836** (t=6490 ms, 4 s nach Bremsbeginn t=2500 ms) — deckt sich mit der Vorhersage |

`# BIAS_CAL`-Zeilen (Capture 3):
```
# BIAS_CAL name=bench_D_scheinneigung calibrated=1 t_real_ms=66734 bias_rad_s=0.00000 bias_deg_s=0.0000 tau_slow_s=90.0
# BIAS_CAL name=bench_E_stossunterdrueckung calibrated=1 t_real_ms=75250 bias_rad_s=0.00000 bias_deg_s=0.0000 tau_slow_s=90.0
# BIAS_CAL name=bench_F_neigung_plus_bremsung calibrated=1 t_real_ms=80767 bias_rad_s=0.00000 bias_deg_s=0.0000 tau_slow_s=90.0
```
F war schon in Schritt A `calibrated=1` und blieb es — nichts zu melden.

Die im ersten Lauf (Schritt A) gemessene Konfiguration (`tau_slow=30s`
Rückfall) wurde damit dort **nie mit den ausgelieferten 90 s** gemessen;
dieser Nachlauf schließt genau diese Lücke. Keine Abweichung von der
Erwartung — keine weitere Anpassung nötig.

## A6.3 — Reaktionszeiten neu formuliert (Messauflösung und Quantisierung)

**Verfahren und Grenze:** Die Bench tastet mit einem festen 10-ms-Takt ab;
jede Reaktionszeit ist deshalb nur auf ±1 Abtastintervall genau bestimmbar.
Eine Angabe wie „0 ms" suggeriert eine Präzision, die das Messverfahren
nicht hergibt — der wahre Wert liegt irgendwo im Intervall
`[0, 10) ms` bzw. allgemein `[gemessen−10, gemessen] ms`, da die
Bremsung jederzeit innerhalb eines 10-ms-Fensters nach dem letzten Sample
begonnen haben kann, aber frühestens im nächsten Sample sichtbar wird.

| Experiment | Bremsbeginn | erster `duty>20%` | Reaktionszeit (quantisiert) |
|---|---|---|---|
| B (Sprung 6,0 m/s²) | t=1000 ms | t=1000 ms | **≤ 10 ms** (unterhalb der Messauflösung) |
| D (Scheinneigung, nach Harness-Fix) | t=2500 ms | t=2510 ms | **≤ 10 ms** (unterhalb der Messauflösung) |
| F (Gefälle+Bremsung) | t=2000 ms | t=2020 ms | **20 ms ± 10 ms** |

NFR-RT-01 (≤ 50 ms) gilt weiterhin als erfüllt, alle drei Werte liegen mit
klarem Abstand darunter — die Unsicherheit selbst (±10 ms) ändert daran
nichts, macht die Aussage aber ehrlich statt scheingenau.

## A6.4 — dt_s-Standardabweichung (offline aus Capture 4, `dt_real_ms`-Feld)

Reale Zeitmarken waren in den Captures aus Schritt A nicht einzeln
vorhanden (`t_ms` in der primären CSV ist der **nominelle** Schleifenindex,
kein Ist-Zeitstempel) — deshalb wurde dem `# DIAG`-Format ein reales
`dt_real_ms`-Feld pro Sample hinzugefügt (A6.1a) und in Capture 4
mitprotokolliert. Auswertung (Python, `statistics.mean`/`pstdev`):

| Experiment | n | Mittelwert | Std | Min | Max | Verteilung |
|---|---|---|---|---|---|---|
| D | 851 | 10,0000 ms | 0,9744 ms | 9 ms | 11 ms | 9 ms: 47,5 % · 10 ms: 5,1 % · 11 ms: 47,5 % |
| E | 551 | 10,0000 ms | 0,6085 ms | 9 ms | 11 ms | 9 ms: 18,5 % · 10 ms: 63,0 % · 11 ms: 18,5 % |
| F | 601 | 10,0017 ms | 0,6173 ms | 9 ms | 11 ms | 9 ms: 19,0 % · 10 ms: 61,9 % · 11 ms: 19,1 % |

Mittelwert in allen drei Fällen exakt/nahezu 10,000 ms — die ±1-ms-Abweichungen
mitteln sich sauber heraus, keine Drift. Auffällig: D zeigt ein fast
perfekt bimodales 9/11-Muster (nur 5,1 % exakt bei 10 ms), E/F dagegen
eine Mehrheit exakt bei 10 ms. Ursache nicht abschließend verifiziert;
naheliegende Hypothese: die höhere Serial-Ausgabelast in D (Legacy-Vergleich
zusätzlich zur DIAG-Zeile) verschiebt die Rundung auf `millis()`-Ganzzahl-
grenzen konsistent in eine Richtung — als Hypothese gekennzeichnet, nicht
als gesicherter Befund.

## A6.5 — Clamping-Ereignisse zugeordnet

Nach dem Harness-Fix (A6.1a): **0 Clamp-Ereignisse** in allen sechs
Experimenten, beiden Captures (`# DT_CLAMP_EVENT` kommt kein einziges Mal
vor; `# DT_CLAMP ... count=0` durchgehend). Das bestätigt die Vermutung aus
Schritt A explizit: die dort beobachteten Einzeltreffer in E (1×) und F (1×)
waren **ausschließlich der Harness-Bug aus A6.1a** (jeweils das erste
Sample) — kein echter Scheduler-Jitter. Mit dem Fix verschwinden sie
vollständig, kein „echter" Clamp-Fall wurde beobachtet.

## A6.7 — NFR-RT-04 (Worst-Case-Schleifenzeit Normalbetrieb)

Weiterhin **nicht angegangen** (per Auftrag). Offener Punkt für den
nächsten Arbeitsschritt: Messung gehört in den `esp32dev`-Normal-Build
(reales `loop()`, kooperativer Scheduler), nicht in `BENCH_MODE`
(Busy-Wait-Schleife, misst nur eine Näherung, s. Schritt A oben).

## A6.8 — Abschluss

- `pio test -e native`: 88/88 grün (unverändert, Bench-Harness-Änderungen
  betreffen nur `main.cpp`/`BENCH_MODE`, nicht die native-getestete Logik).
- `pio run -e esp32dev` und `-e esp32dev_bench`: beide grün.
- Rohmitschnitte (`bench_capture3_...`, `bench_capture4_...`) unter
  `docs/Validierung/` abgelegt, nicht committet (`*.log` ohnehin in
  `.gitignore`).
- `MOTION_NORM_STATIC_BAND`/`_JERK_DELTA`/`_SHOCK_DELTA` unverändert.

---

## Geltungsbereich der Zeitstatistik

Die in Schritt A/A6 gemessene `dt`-Verteilung beschreibt den **Bench-Harness**,
nicht den produktiven Scheduler. Der Harness taktet auf ein absolutes Raster
(`next_tick += 10`) bei 1-ms-Auflösung von `millis()`; überschreitet eine
Iteration die 10 ms, fällt das folgende Sample entsprechend kürzer aus.
Mittelwert bleibt daher exakt 10,000 ms, während die Streuung in die Ränder
wandert — das erklärt das 9/11-Bimodal in D (höhere Serial-Last durch den
Legacy-Vergleich) gegenüber E/F (Mehrheit exakt bei 10 ms, geringere
Serial-Last).

Aussagen über das Zeitverhalten im Fahrbetrieb sind daraus **nicht**
ableitbar. Diese Messung erfolgt seit Schritt B-FW über die Felder
`dt_max_ms` und `loop_max_us` des v3-Telemetrie-Frames (s.
`docs/BLE_Frame_v3_Schnittstelle.md` Kap. 3.2) — gemessen im realen
`esp32dev`-Normalbetrieb-`loop()` (kooperativer Scheduler), nicht in
`BENCH_MODE`. Damit ist der in A6.7 offen gelassene Punkt (NFR-RT-04 am
Normalbetrieb) geschlossen: `loop_max_us` wird per `micros()` um den
gesamten `loop()`-Körper (bis kurz vor `delay(1)`) gemessen und je
100-ms-Fenster aggregiert an die App übertragen — eine tatsächliche
Feldmessung steht noch aus (erfordert eine BLE-Verbindung während der
Fahrt), aber der Messpfad selbst ist implementiert und host-getestet.
