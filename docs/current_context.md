# Current Context

> Von Claude Code eigenständig pflegbar (siehe `CLAUDE.md`).
> **Zwischenstand 07.08.2026** — vor der Wiederholungs-Messfahrt.

## Kurzfassung

Die Überarbeitung des `motion_filter` (Stufe 1, Normbetrags-Gate) ist
**implementiert, host-getestet und auf der Hardware nachgewiesen**. Der
Kernbeleg der Arbeit liegt gemessen vor. Die Telemetrie wurde auf Schema v3
erweitert, um die Feldvalidierung überhaupt auswertbar zu machen. Die
Reparaturrunde vom Abend des 07.08.2026 hat alle Befunde des Ruhetests
aufgelöst. **Die Firmware ist damit fahrbereit.** Offen sind nur noch die
Wiederholungs-Messfahrt und der Dokumentations-Durchgang.

**Build-Stand:** `pio test -e native` **120/120** grün · `pio run -e esp32dev`
grün · `pio run -e esp32dev_bench` grün · Debug-Code entfernt, finale Firmware
geflasht und Boot verifiziert. **Commit `1178017`** (Firmware + Doku +
iOS-Track AP1–AP3), nicht gepusht.

---

## 1. Stufe 1 — Normbetrags-Gate (abgeschlossen, Feldnachweis offen)

`motion_filter` klassifiziert jedes Sample in eines von drei Regimen
(STATIC / DYNAMIC / SHOCK) und unterdrückt die Accelerometer-Korrektur
außerhalb von STATIC. Zusätzlich umgesetzt: dt-parametrierte Blendfaktoren,
Verankerung über `atan2f` nur innerhalb des STATIC-Bands, zweistufige
Gyro-Bias-Schätzung, schwache Nachglättung (3-Punkt-Median + 15-Hz-Tiefpass).

### Kernbeleg (Bench-Experiment D, Hardware, 07.08.2026)

| Filter | Ausgang bei 4 s Dauerbremsung |
|---|---|
| Legacy (α = 0,98) | 3,924 → **0,000** (Signal vollständig gelöscht) |
| Neu (Normbetrags-Gate) | 3,9 → **3,836** (gehalten) |

Das ist der Vorher-Nachher-Nachweis für Fehlermechanismus A
(Scheinneigung). Er ist auf der realen Hardware gemessen, nicht simuliert.
Die analytische Vorhersage lautete 3,83 — Modell und Messung stimmen überein.

### Gemessene Zeiteigenschaften

| Größe | Messwert | Anforderung |
|---|---|---|
| Reaktionszeit Bremsbeginn → LED | ≤ 10 ms (B), 20 ± 10 ms (F) | NFR-RT-01 ≤ 50 ms ✅ |
| Worst-Case-Schleifenzeit, Normalbetrieb | **0,651 ms** | NFR-RT-04 < 10 ms ✅ |
| dt-Takt (Bench-Harness) | 9–11 ms, Mittel exakt 10,000 ms | — |

**Messauflösung:** Der Abtasttakt beträgt 10 ms; feinere Reaktionszeiten sind
mit diesem Verfahren nicht auflösbar. Angaben unter 10 ms sind entsprechend
als „≤ 10 ms" zu führen, nicht als „0 ms".

**Geltungsbereich der Bench-Zeitstatistik:** Die dt-Verteilung beschreibt den
Bench-Harness (absolutes Raster, 1-ms-Auflösung von `millis()`), nicht den
produktiven Scheduler. Aussagen über das Zeitverhalten im Fahrbetrieb sind
daraus nicht ableitbar — dafür dienen die v3-Felder `dt_max_ms` und
`loop_max_us`.

### Bekannte Restdämpfung (quantifiziert)

Bei einer Bremsung von 2,2 m/s² über 300 ms Anstieg beträgt die verbleibende
Dämpfung **5,9 %** (vorher 20,7 %). Daraus folgt die für die Arbeit
maßgebliche Aussage: Die nominelle Ansprechschwelle von 2,0 m/s² entspricht
einer **effektiven Ansprechschwelle von rund 2,13 m/s²**. Diese Zahl ist so zu
dokumentieren; eine Angabe von „2,0 m/s²" wäre nicht belegbar.

Ursache ist die Totzone des STATIC-Bands: Jede Bremsung durchläuft beim
Anstieg den Bereich 0…1,539 m/s² und ist dabei noch als STATIC klassifiziert.
Mit `MOTION_COMPL_TAU_S = 3,0 s` beträgt der dabei aufgenommene Scheinpitch
noch etwa 0,06 m/s² (vorher 0,30). Stufe 2 (GNSS-Bias, τ_b ≈ 10 s) ist
darauf ausgelegt, diesen Rest weiter zu reduzieren.

---

## 2. Telemetrie-Frame Schema v3 (abgeschlossen)

Frame 81 → **113 Byte**, `version` = 3, Offsets 0–80 byte-identisch zu v2.
Verbindlicher Vertrag: `docs/BLE_Frame_v3_Schnittstelle.md` (Repo) bzw.
`claude/BLE_Frame_v3_Schnittstelle.md` (Projekt) — beide Fassungen geprüft
byte-identisch.

**Neu:** `gnss_accel_ms2` + `gnss_accel_valid` (Referenzgröße, E1),
`pitch_rad`, `gyro_bias_rads`, `bias_calibrated` (Innensicht des Filters),
`norm_delta_min/_max`, `jerk_max`, drei Regime-Zähler (Fensteraggregate über
100 ms), `dt_max_ms`, `loop_max_us` (Zeitverhalten im Normalbetrieb).

**Zentraler Entwurfspunkt:** Regime-Entscheidung und Jerk-Kriterium leben auf
dem 100-Hz-Takt, das Frame läuft mit 10 Hz. Statt zu unterabtasten werden
**Aggregate über das 100-ms-Fenster** übertragen. Damit bleibt die
100-Hz-Information erhalten, ohne die Datenrate zu erhöhen.

Neue Logikmodule (host-testbar, ohne Hardwarebezug): `telemetry_window_agg`,
`gnss_speed_ref`. Die GNSS-Referenz **beobachtet nur** und wirkt nirgends auf
`motion_filter` oder `tail_light_fsm` zurück (Entscheidung E5).

**Kreuztest über Toolchain-Grenzen:** `testdata/frame_v3_golden.hex` (113 Byte,
41 unterscheidbare Werte) plus `testdata/frame_v3_golden.md` (Wertetabelle).
Die Firmware erzeugt die Bytefolge, die iOS-App dekodiert genau diese Bytes.
Round-Trip-Tests je Seite prüfen nur die eigene Symmetrie — ein gemeinsamer
Denkfehler bliebe darin unsichtbar. Der Bootstrap-Lauf schlägt bei fehlender
Referenzdatei bewusst fehl, damit ein fehlendes Golden-File nicht als
„bestanden" durchgeht.

---

## 3. Ruhetest und Reparaturrunde (07.08.2026) — abgeschlossen

Gerät auf dem Tisch, `esp32dev`-Normalbuild.

| Größe | Ruhetest (vormittags) | Nach Reparaturrunde | Bewertung |
|---|---|---|---|
| `loop_max_us` | 0,651 ms | unverändert | NFR-RT-04 erfüllt ✅ |
| `bias_calibrated` | bleibt 0 über 80 s | **1 nach ≈ 3 s** | behoben ✅ |
| gemessener Gyro-Offset | nie kalibriert | **−4,61 °/s**, kompensiert | erstmals erfasst ✅ |
| STATIC-Anteil im Ruhezustand | 72,3 % | **90–100 %** | behoben ✅ |
| `brake_decel_ms2` im Ruhezustand | ≈ 3,0–3,2 | **kein Sockel mehr** | geklärt ✅ |

### 3.0 Ursache des Ruhewerts von 3,0 m/s² — geklärt

Der Drehratensensor lieferte im Stillstand einen konstanten Nullpunktfehler
von **−4,61 °/s**, der mangels funktionierender Kalibrierung nie abgezogen
wurde. Über die bereits früher hergeleitete Beziehung ε = b · τ ergibt das bei
der wirksamen Zeitkonstante von 4 s einen stationären Lagefehler von 18,4° und
damit g · sin(18,4°) = **3,1 m/s²** — genau der beobachtete Sockel. Ursache,
analytische Beziehung und Messwert stimmen überein.

Für die Arbeit verwertbar: Ein Befund, der zunächst wie ein Filterdefekt
aussah, ließ sich vollständig auf eine einzelne unkompensierte Sensorgröße
zurückführen, und die zuvor aufgestellte Beziehung sagt den gemessenen Wert
korrekt voraus.

**Neigungstest (Nachweis Fehlermechanismus A behoben):** Bei einer Neigung von
23,5° erreicht der Ausgang lediglich 1,07 m/s² und bleibt damit deutlich unter
der Ansprechschwelle von 2,0 — eine reine Neigung löst kein Bremslicht mehr
aus. Nach dem Ablegen klingt der Ausgang mit rund 3–4 s Zeitkonstante gegen
null ab, wie für `MOTION_COMPL_TAU_S = 3,0 s` zu erwarten.

**Notiert, nicht verfolgt:** `pitch_rad` pendelt sich nach dem Ablegen auf
einen stabilen Restwert von ≈ 4,2° ein. Ob das die tatsächliche Auflagefläche
abbildet oder ein Restfehler ist, wird aus den Fahrdaten beantwortet, nicht am
Schreibtisch (s. `open_issues.md`).

### 3.1 Sensorkonfiguration — Aliasing nachgewiesen und behoben

Aus den Registern zurückgelesen: `DLPF_CFG = 0` (260 Hz Bandbreite),
`SMPLRT_DIV = 0` (interne Rate 8 kHz), `AFS_SEL = 3` (±16 g),
`FS_SEL = 0` (±250 °/s).

Der Sensor aktualisiert seine Register mit 8 kHz, die Firmware greift mit
100 Hz eine Momentaufnahme heraus — ohne Dezimationsfilter und mit einer
Bandbreite weit oberhalb der halben Abtastrate. Das ist **Unterabtastung ohne
Antialiasing**. `Adafruit_MPU6050::begin()` schreibt beide Register nie
explizit; es bleiben die POR-Defaults stehen, die für diesen Anwendungsfall
die ungünstigsten sind.

**Messung mit `DLPF_CFG = 3` (44 Hz):** `norm_delta`-Spanne 0,235 statt
0,34–0,45; STATIC-Anteil **85,7 %** statt 68–73 %. Deutliche Verbesserung der
Klassifikationsstabilität, aber ein Teil des Rauschens bleibt unerklärt.

**Umgesetzt (Reparaturrunde):** `DLPF_CFG = 3` (44 Hz) und `SMPLRT_DIV = 4`
(200 Hz Sensorrate) fest in `imu_driver.cpp`, mit Register-Readback-
Verifikation am Board bestätigt (`CONFIG=0x03 … SMPLRT_DIV=4 … -> OK`).
Zusätzliche Gruppenlaufzeit 4,9 ms gegen NFR-RT-01 ≤ 50 ms unkritisch.

### 3.2 Kalibrierfenster praktisch unerreichbar — behoben

Das Verankerungsfenster verlangt 100 **zusammenhängende** STATIC-Abtastungen.
Bei real 72 % (bzw. 85,7 % nach der Filterkorrektur) STATIC-Anteil reißt die
Kette statistisch praktisch immer ab: 0,857¹⁰⁰ ≈ 2 · 10⁻⁷. Folge:
`bias_calibrated` wird nie 1, das System läuft dauerhaft mit dem
konservativen `MOTION_COMPL_TAU_SLOW_UNCAL_S = 30 s`. **Die auf 90 s
ausgelegte und auf dem Bench nachgewiesene Konfiguration ist im Feld nie
aktiv.**

Ursache ist eine Entwurfsentscheidung, die sich als sachlich falsch erwiesen
hat: Pitch-Verankerung und Bias-Kalibrierung wurden in ein gemeinsames
Fenster gelegt („ein Zustand, zwei Verbraucher"). Die Verankerung braucht
Zusammenhang, weil sie eine kohärente Lagereferenz bildet — die Bias-Mittelung
nicht, denn einem Mittelwert über ruhige Abtastungen ist Nachbarschaft
gleichgültig.

**Umgesetzt (Reparaturrunde):** Verankerung mit 0,3-s-Fenster und Toleranz für
zwei aufeinanderfolgende Ausreißer; Bias-Kalibrierung über 200 kumulierte
STATIC-Abtastungen ohne Zusammenhangsforderung. Am Board erreicht
`bias_calibrated` nach etwa 3 s den Wert 1 — vorher nie innerhalb von 80 s.

### 3.3 EMV-Hypothese nicht auflösbar

Rauschspanne bei 0 / 20 / 100 % LED-Duty: 0,360 / 0,446 / 0,242 — kein
monotoner Zusammenhang. Die Streuung zwischen unabhängigen Wiederholungen
(0,17…0,73) ist **größer als der zu messende Effekt**; der Versuch kann die
Hypothese mit je einem 30-s-Lauf weder stützen noch widerlegen. Elektrische
Einstreuung aus einem Schaltregler wäre stationär; eine Störgröße, die von
Lauf zu Lauf um Faktor vier schwankt, spricht eher für mechanische
Umgebungsanregung. Ein belastbarer EMV-Nachweis bräuchte Wiederholungen je
Stufe und eine Auswertung im Frequenzbereich — **als Ausblick geführt, nicht
Teil dieser Arbeit.**

---

## 4. Arbeitsplan bis zur Abgabe (Umfangsschnitt vom 07.08.2026)

Der Untersuchungsumfang wurde bewusst begrenzt. Es werden **nur noch Punkte
umgesetzt, die die Funktion blockieren**; alles Übrige wird als Grenze der
Arbeit dokumentiert.

1. ~~**Reparaturrunde**~~ — **erledigt 07.08.2026, Commit `1178017`.**
2. **Messfahrt** (~½ Tag) — **nicht mehr blockiert, App-Seite ist fertig.**
   Identisches Protokoll wie 06.08.2026 (sechs Fahrten, gleiche Strecke, App-
   und Strava-Aufzeichnung) für den direkten Vorher-Nachher-Vergleich, ergänzt
   um acht definierte Einzelmanöver (`Messprotokoll_Fahrt.pdf` liegt vor).
   Empfohlen vorab: zweiminütige Testaufzeichnung am Schreibtisch, CSV
   exportieren und die 35 Spalten auf plausible Werte prüfen.
3. **Dokumentation** (~1 Tag): Project Bible, App Bible Kap. 10, Roadmap,
   Lessons Learned, Validierungsunterlagen, Korrektur von
   `measurement_log.md`.

**Ausdrücklich nicht mehr vorgesehen:** weitere Optimierungsschleifen an den
Schwellwerten. `MOTION_NORM_STATIC_BAND`, `_JERK_DELTA` und `_SHOCK_DELTA`
bleiben auf ihren jetzigen Werten. Die Messfahrt dient dazu, ihr Verhalten zu
**dokumentieren**, nicht sie zu iterieren. Ergibt die Fahrt eine
Fehlanpassung, wird das als begründetes Ergebnis mit Empfehlung festgehalten.

---

## 5. iOS-App (paralleler Track, Claude in Xcode) — Schema v3 abgeschlossen

**Stand App Bible v0.22 (07.08.2026): Frame-Schema-v3-Migration AP0–AP8
abgeschlossen, 89 Tests grün** (SmartBikeCore 57, App-Unit 29, UITest 3),
committet. Damit ist die Messfahrt nicht mehr blockiert.

Umgesetzt: Decoder auf 113 Byte mit Mindestversions-/Längenregel (statt
`version == 2` nun `version ≥ 2 & len ≥ 81` für v2-Felder, `≥ 3 & ≥ 113` für
v3-Felder, überzählige Bytes ignorieren, `< 81` oder `version < 2` verwerfen);
reiner Decoder mit Ergebnis-Enum, Zähler im Store; gemeinsamer
`TelemetryFrameEncoder` als einzige Byte-Quelle für Mock und Round-Trip-Test;
Persistenz additiv um die 13 v3-Felder erweitert (leichtgewichtige Migration,
Altdaten laden mit nil); umschaltbarer 10-Hz-Validierungsmodus mit
Batch-Persistenz (~1 Save/s, Skalentest 6000 Samples ohne Verlust);
CSV-Export mit 35 Spalten und eingefrorenem Golden-Header; read-only
Diagnoseansicht unter Settings.

**Golden-Vektor-Kreuztest bestanden.** Der Produktions-Decoder liest den von
der Firmware erzeugten 113-Byte-Golden-Vektor
(`testdata/frame_v3_golden.hex/.md`) und prüft jedes Feld gegen die
Firmware-Wertetabelle — ausdrücklich **nicht** über den eigenen Encoder. Das
ist der geräteunabhängige Nachweis, dass Firmware und App dieselbe
Bytebelegung meinen. Round-Trip-Tests je Seite hätten einen gemeinsamen
Denkfehler nicht aufgedeckt.

**Am realen iPhone verifiziert:** `truncatedV3FrameCount = 0`,
`dt_max_ms = 10`, `loop_max_us = 53`.

**Format-Referenz für jede externe Auswertung:**
`docs/CSV_Format_v3_Validierungsexport.md` bzw.
`claude/CSV_Format_v3_Validierungsexport.md` — 35 Spalten, Semikolon,
deutsches Dezimalkomma, CRLF, UTF-8 mit BOM, Präambel mit Geräte-Frame-Version
und `frame_version_gemischt`.

Entschiedene Sonderfälle: Ein zu kurzes v3-Frame wird gelesen (v2-Ebene), aber
in `truncatedV3FrameCount` geführt — weder verwerfen noch verschweigen. Bei
gemischten Frame-Versionen in einer Fahrt trägt die CSV-Präambel das
**Minimum** plus die Zeile `# frame_version_gemischt;ja`.

**Weiterhin offen (App):** Cockpit-Editor (AR-LIVE-08), finale SF-Symbol-Wahl,
optionale Höhen-Referenzdruck-Kalibrierung — alles nach der Messfahrt bzw.
Future Work.

---

## 6. Historie (verdichtet)

Ältere Arbeitsstände sind kanonisch in der Project Bible geführt. Kurzabriss:
Feldtest 06.08.2026 mit sechs Fahrten falsifizierte die Bremserkennung
(r = −0,132, n = 939) und führte zur Architekturentscheidung V-B (IMU als
schneller Regelpfad, GNSS als langsame Stützreferenz). Davor: BLE-Transport
(NimBLE) nach Board-Tausch validiert, Brownout-Fallstudie dokumentiert;
Serial-Bench-Validierung der Bremslicht-Kennlinie; I²C-Recovery, Fail-Safe
und Task-Watchdog per Fehlerinjektion verifiziert.