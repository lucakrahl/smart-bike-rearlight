# Current Context

> Von Claude Code eigenständig pflegbar (siehe `CLAUDE.md`).
> **Zwischenstand 07.08.2026** — vor der Wiederholungs-Messfahrt.

## Kurzfassung

Die Überarbeitung des `motion_filter` (Stufe 1, Normbetrags-Gate) ist
**implementiert, host-getestet und auf der Hardware nachgewiesen**. Der
Kernbeleg der Arbeit liegt gemessen vor. Die Telemetrie wurde auf Schema v3
erweitert, um die Feldvalidierung überhaupt auswertbar zu machen. Offen sind:
ein ungeklärter Ruhewert des Filterausgangs, zwei kleine Konfigurations-
korrekturen, die Wiederholungs-Messfahrt und der Dokumentations-Durchgang.

**Build-Stand:** `pio test -e native` 117/117 grün · `pio run -e esp32dev`
grün (RAM 32,6 % / 106 904 B, Flash 21,5 % / 675 283 B) · `pio run -e
esp32dev_bench` grün (RAM 7,8 %, Flash 12,0 %). Nichts committet.

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

## 3. Befunde des Ruhetests (07.08.2026) — teilweise offen

Gerät auf dem Tisch, `esp32dev`-Normalbuild, 80 s.

| Größe | Messwert | Erwartung / Bewertung |
|---|---|---|
| `gyro_bias_rads` (eingeschwungen) | −0,0014 °/s | unkritisch ✅ |
| `loop_max_us` | 0,651 ms | NFR-RT-04 erfüllt ✅ |
| `bias_calibrated` | bleibt 0 über 80 s | ❌ Konstruktionsfehler, s. u. |
| `norm_delta`-Spanne | −0,73 … +0,61 m/s² | ~3× über der Datenblatt-Erwartung |
| STATIC-Anteil | 72,3 % | zu niedrig für ein ruhendes Gerät |
| `brake_decel_ms2` im Ruhezustand | wiederholt ≈ 3,0–3,2 | ❌ **ungeklärt** |

### 3.1 Sensorkonfiguration — Aliasing nachgewiesen

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

### 3.2 Kalibrierfenster praktisch unerreichbar

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

### 3.3 Ungeklärt: Ruhewert 3,0 m/s²

Ein statisch geneigtes Gerät hat ‖a‖ = g; der Komplementärfilter muss die
Neigung schätzen und den Schwerkraftanteil abziehen. Bei einer wirksamen
Zeitkonstante von rund 4 s müsste der Ausgang nach 30 s Ruhe praktisch null
sein. Ein dauerhafter Wert von 3,0 m/s² ist damit unvereinbar.

Zwei Hypothesen: (a) die Neigungsachse ist eine andere als angenommen — die
18° wurden aus dem Messwert selbst zurückgerechnet, also zirkulär; (b) die
Accelerometer-Korrektur wird erst nach abgeschlossener Verankerung angewendet,
und die schließt nie ab — dann wäre der Filter zu reiner Gyro-Integration
degeneriert. **Der Neigungstest (kippen, halten, ablegen, 30 s nachlaufen)
entscheidet das experimentell.**

### 3.4 EMV-Hypothese nicht auflösbar

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

1. **Reparaturrunde** (~½ Tag): Neigungstest zur Klärung von 3.3;
   `DLPF_CFG = 3` und `SMPLRT_DIV = 4` fest setzen; Verankerungsfenster
   1,0 s → 0,3 s mit Toleranzzähler; Bias-Kalibrierung auf kumulierte
   STATIC-Abtastungen umstellen.
2. **Messfahrt** (~½ Tag): identisches Protokoll wie 06.08.2026 (sechs
   Fahrten, gleiche Strecke, App- und Strava-Aufzeichnung) für den direkten
   Vorher-Nachher-Vergleich.
3. **Dokumentation** (~1 Tag): Project Bible, Decision Log, Validierungs-
   unterlagen, Korrektur von `measurement_log.md`.

**Ausdrücklich nicht mehr vorgesehen:** weitere Optimierungsschleifen an den
Schwellwerten. `MOTION_NORM_STATIC_BAND`, `_JERK_DELTA` und `_SHOCK_DELTA`
bleiben auf ihren jetzigen Werten. Die Messfahrt dient dazu, ihr Verhalten zu
**dokumentieren**, nicht sie zu iterieren. Ergibt die Fahrt eine
Fehlanpassung, wird das als begründetes Ergebnis mit Empfehlung festgehalten.

---

## 5. iOS-App (paralleler Track, Claude in Xcode)

MVP funktionsfertig und am realen iPhone gegen die echte BLE-Verbindung
verifiziert. Für Schema v3 liegt ein Umsetzungsplan in neun Arbeitspaketen vor
(`docs/Umsetzungsplan_Schema_v3_iOS.md`): Decoder auf v3 mit
Mindestversions-/Längenregel, Golden-Bytes-Kreuztest, Persistenz-Migration
(additiv + optional), 10-Hz-Validierungsmodus (E2), CSV-Export 23 → 35 Spalten
(E4), Diagnoseansicht unter Settings. Umsetzung läuft.

Entschiedene Sonderfälle: Ein zu kurzes v3-Frame wird gelesen (v2-Ebene), aber
in einem eigenen Zähler `truncatedV3FrameCount` geführt — weder verwerfen noch
verschweigen. Bei gemischten Frame-Versionen in einer Fahrt trägt die
CSV-Präambel das **Minimum** plus die Zeile `# frame_version_gemischt;ja`.

---

## 6. Historie (verdichtet)

Ältere Arbeitsstände sind kanonisch in der Project Bible geführt. Kurzabriss:
Feldtest 06.08.2026 mit sechs Fahrten falsifizierte die Bremserkennung
(r = −0,132, n = 939) und führte zur Architekturentscheidung V-B (IMU als
schneller Regelpfad, GNSS als langsame Stützreferenz). Davor: BLE-Transport
(NimBLE) nach Board-Tausch validiert, Brownout-Fallstudie dokumentiert;
Serial-Bench-Validierung der Bremslicht-Kennlinie; I²C-Recovery, Fail-Safe
und Task-Watchdog per Fehlerinjektion verifiziert.
