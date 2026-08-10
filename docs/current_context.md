# Current Context

> Von Claude Code eigenständig pflegbar (siehe `CLAUDE.md`).
> **Stand 10.08.2026 — Firmware abgeschlossen und eingefroren.**

## Kurzfassung

**Die Firmware ist fertig.** Commit `835c7b3` vom 10.08.2026 ist der
Abschlussstand: 126/126 Host-Tests grün, beide Build-Umgebungen fehlerfrei,
keine `TODO`- oder `FIXME`-Marker im eigenen Quellcode, Auslieferungsstand auf
das Gerät geflasht und im Normalbetrieb geprüft. Weitere Firmware-Änderungen
sind nur noch bei einem funktionsverhindernden Fehler vorgesehen.

Der Kernbeleg der Arbeit liegt gemessen vor: Stufe 1 (`motion_filter`,
Normbetrags-Gate) ist auf der Messfahrt vom 08.08.2026 im Feld verifiziert —
9 von 9 Bremsvorgängen erkannt, r = +0,85 gegen die GNSS-Referenz bei
gleichem Zeitversatz (Vergleichsfahrten vom 06.08.: Median +0,15), kein
Ruhesockel mehr, keine Fehlauslösung in 25 Beschleunigungsepochen.

**Der Schwerpunkt liegt ab jetzt auf Auswertung, Konstruktion und Text.**

---

## 1. Abschlussstand der Firmware (Commit `835c7b3`, 10.08.2026)

| Größe | Wert |
|---|---|
| Host-Tests | **126/126** grün (`pio test -e native`) |
| Builds | `esp32dev` und `esp32dev_bench` SUCCESS |
| Flash | 674 487 B (21,4 %) |
| RAM | 106 912 B (32,6 %) |
| `TODO`/`FIXME` im eigenen Code | **0** |
| Gerät | geflasht auf `/dev/cu.usbserial-110`, verifiziert |

### Was die Abschlussänderung enthält

1. **Mangel M-01 behoben.** Der `else`-Zweig in `tail_light_fsm.cpp` führte
   `held_brake_duty_pct_` auch unterhalb der Einschaltschwelle nach und fror
   damit den Schlusslichtwert ein — die 300-ms-Mindesthaltezeit nach FR-TL-06
   war im Fahrbetrieb in allen 14 aufgezeichneten Fällen wirkungslos. Der Wert
   wird jetzt nur noch oberhalb `BRAKE_ON_MS2` nachgeführt. Ein neuer
   Regressionstest fährt einen Bremsvorgang monoton durch das Hystereseband
   (4,0 → 2,5 → 1,8 → 1,0 m/s²) und sichert zu, dass die Duty während der
   Haltezeit oberhalb des Schlusslichtwerts bleibt.
2. **Einbaulage-Transformation** (bereits vorher entwickelt, mit diesem Stand
   committet und geflasht): `lib/logic/imu_mount_orientation.h` bildet die
   180°-Drehung der Lochrasterplatine an der Treibergrenze zurück,
   `IMU_MOUNT_SIGN_X/Y/Z` = −1/−1/+1 auf Beschleunigung **und** Drehrate.
   Determinante +1, dθ/dt = ω_x bleibt gültig. `MOTION_BRAKE_SIGN` bleibt +1.
3. **Auslieferungsstand.** Alle drei `TODO(temp debug)`-Prints entfernt,
   `DEBUG_SERIAL = false`, `taskConfigConsole()` (Debug-Hang-Hook) entfernt.
4. **Umfangsschnitt FR-CFG-02/03** als begründete Abgrenzung dokumentiert
   (Project Bible Kap. 12.2), nicht implementiert.
5. **Aufräumen:** veraltete Kommentare korrigiert, zwei Entwurfsentscheidungen
   kommentiert (Gyro-Bias-Tor auf roher Drehrate, `t_imu = now` statt rekursiv),
   tote Symbole entfernt, `lib_deps` gepinnt, alle `TODO(offen)`-Marker in
   begründete Abgrenzungen umformuliert.

### Gepinnte Bibliotheksversionen

Adafruit MPU6050 2.2.9 · Adafruit BMP280 3.0.0 · Adafruit Unified Sensor 1.1.15 ·
TinyGPSPlus 1.1.0 · rc-switch 2.6.4 · NimBLE-Arduino 2.5.0

### Methodische Korrektur an Befund B7

Die im Fahrbetrieb gemessene Worst-Case-Schleifenzeit von 6,7 ms war in der
Erstauswertung allein dem 1-Hz-GNSS-Slot zugeschrieben worden. Diese Zuordnung
war nicht belegt: Im Aufzeichnungsstand lagen **drei** Debug-Ausgaben mit exakt
1 Hz innerhalb des Messfensters von `loop_max_us` (zusammen ≈ 173 Byte, bei
115 200 Bd ≈ 87 µs je Byte). Beide Ursachen sind aus der Zeitreihe nicht
trennbar, weil beide exakt mit 1,00 s periodisch sind. **NFR-RT-04 (< 10 ms)
bleibt in jeder Lesart erfüllt**; zurückgenommen wird nur die Aussage, der
NMEA-Parselauf sei die dominierende Einzellast. Im Auslieferungsstand ist die
UART-Last entfallen.

---

## 2. Stufe 1 — Normbetrags-Gate (abgeschlossen, feldverifiziert)

`motion_filter` klassifiziert jedes Sample in eines von drei Regimen
(STATIC / DYNAMIC / SHOCK) und unterdrückt die Accelerometer-Korrektur
außerhalb von STATIC. Zusätzlich: dt-parametrierte Blendfaktoren, Verankerung
über `atan2f` nur innerhalb des STATIC-Bands, zweistufige Gyro-Bias-Schätzung,
schwache Nachglättung (3-Punkt-Median + 15-Hz-Tiefpass).

### Kernbeleg (Bench-Experiment D, Hardware, 07.08.2026)

| Filter | Ausgang bei 4 s Dauerbremsung |
|---|---|
| Legacy (α = 0,98) | 3,924 → **0,000** (Signal vollständig gelöscht) |
| Neu (Normbetrags-Gate) | 3,9 → **3,836** (gehalten) |

Vorher-Nachher-Nachweis für Fehlermechanismus A (Scheinneigung), auf realer
Hardware gemessen. Die analytische Vorhersage lautete 3,83 — Modell und Messung
stimmen überein.

### Feldnachweis (Messfahrt 08.08.2026, 10 Hz, Schema v3)

| Größe | Wert |
|---|---|
| Erkannte Bremsvorgänge | 9 von 9 |
| Korrelation zur GNSS-Referenz (fester Versatz −2,0 s) | **r = +0,85** (06.08.: Median +0,15) |
| Unabhängig geschätzter Versatz −1,6 s | r = +0,808 (95 % CI +0,748…+0,855) |
| Fehlauslösungen bei Beschleunigung | 0 von 25 Epochen |
| Ruhesockel im Stillstand | Median 0,00, P99 0,08 m/s² (vorher ≈ 3,0) |

### Bekannte Restdämpfung (quantifiziert)

Bei einer Bremsung von 2,2 m/s² über 300 ms Anstieg beträgt die verbleibende
Dämpfung **5,9 %** (vorher 20,7 %). Daraus folgt die für die Arbeit maßgebliche
Aussage: Die nominelle Ansprechschwelle von 2,0 m/s² entspricht einer
**effektiven Ansprechschwelle von rund 2,13 m/s²**. Eine Angabe von „2,0 m/s²"
wäre nicht belegbar.

### Gemessene Zeiteigenschaften

| Größe | Messwert | Anforderung |
|---|---|---|
| Reaktionszeit Bremsbeginn → LED | ≤ 10 ms (B), 20 ± 10 ms (F) | NFR-RT-01 ≤ 50 ms ✅ |
| Worst-Case-Schleifenzeit, Prüfstand | 0,651 ms | NFR-RT-04 < 10 ms ✅ |
| Worst-Case-Schleifenzeit, Fahrbetrieb | 6,7 ms | NFR-RT-04 < 10 ms ✅ |

**Messauflösung:** Der Abtasttakt beträgt 10 ms; feinere Reaktionszeiten sind
nicht auflösbar. Angaben unter 10 ms sind als „≤ 10 ms" zu führen, nicht als
„0 ms".

---

## 3. Telemetrie-Frame Schema v3 (abgeschlossen)

Frame 81 → **113 Byte**, `version` = 3, Offsets 0–80 byte-identisch zu v2.
Verbindlicher Vertrag: `docs/BLE_Frame_v3_Schnittstelle.md` (Repo) bzw.
`claude/BLE_Frame_v3_Schnittstelle.md` (Projekt) — beide Fassungen geprüft
byte-identisch.

**Neu gegenüber v2:** `gnss_accel_ms2` + `gnss_accel_valid` (Referenzgröße, E1),
`pitch_rad`, `gyro_bias_rads`, `bias_calibrated` (Innensicht des Filters),
`norm_delta_min/_max`, `jerk_max`, drei Regime-Zähler (Fensteraggregate über
100 ms), `dt_max_ms`, `loop_max_us`.

**Zentraler Entwurfspunkt:** Regime-Entscheidung und Jerk-Kriterium leben auf
dem 100-Hz-Takt, das Frame läuft mit 10 Hz. Statt zu unterabtasten werden
**Aggregate über das 100-ms-Fenster** übertragen.

**Kreuztest über Toolchain-Grenzen:** `testdata/frame_v3_golden.hex` (113 Byte)
plus `testdata/frame_v3_golden.md` (Wertetabelle). Die Firmware erzeugt die
Bytefolge, die iOS-App dekodiert genau diese Bytes — nicht den eigenen Encoder.
Der Bootstrap-Lauf schlägt bei fehlender Referenzdatei bewusst fehl.

---

## 4. Arbeitsplan bis zur Abgabe

Nach den Umfangsschnitten vom 07.08. und 10.08.2026 werden **nur noch Punkte
umgesetzt, die die Funktion blockieren**; alles Übrige ist als begründete
Abgrenzung in Project Bible Kap. 12.2 geführt.

1. ~~**Reparaturrunde**~~ — erledigt 07.08.2026, Commit `1178017`.
2. ~~**Messfahrt**~~ — erledigt 08.08.2026, ausgewertet in
   `docs/Messfahrt_2026-08-08_Auswertung.md`.
3. ~~**Firmware-Abschluss**~~ — erledigt 10.08.2026, Commit `835c7b3`, geflasht.
4. **Konstruktion / Gehäuse** — offen.
5. **Dokumentation und Thesis-Text** — Schwerpunkt. Offene Punkte in
   `open_issues.md` unter „Dokumentation und Thesis-Text".
6. **Hardware-Entscheidungen** — Befund B-1 (UART-Pegel) und B-6 (Nennstrom
   SW1) sind noch zu entscheiden, s. `open_issues.md`.

---

## 5. iOS-App (paralleler Track) — Schema v3 abgeschlossen

**Stand App Bible v0.22 (07.08.2026): AP0–AP8 abgeschlossen, 89 Tests grün**
(SmartBikeCore 57, App-Unit 29, UITest 3), committet.

Umgesetzt: Decoder auf 113 Byte mit Mindestversions-/Längenregel (`version ≥ 2 &
len ≥ 81` für v2-Felder, `≥ 3 & ≥ 113` für v3-Felder, überzählige Bytes
ignorieren, `< 81` oder `version < 2` verwerfen); reiner Decoder mit
Ergebnis-Enum, Zähler im Store; gemeinsamer `TelemetryFrameEncoder` als einzige
Byte-Quelle für Mock und Round-Trip-Test; Persistenz additiv um die 13 v3-Felder
erweitert; umschaltbarer 10-Hz-Validierungsmodus mit Batch-Persistenz
(Skalentest 6000 Samples ohne Verlust); CSV-Export mit 35 Spalten und
eingefrorenem Golden-Header; read-only Diagnoseansicht unter Settings.

**Am realen iPhone verifiziert:** `truncatedV3FrameCount = 0`, `dt_max_ms = 10`,
`loop_max_us = 53`.

**Format-Referenz für jede externe Auswertung:**
`docs/CSV_Format_v3_Validierungsexport.md` — 35 Spalten, Semikolon, deutsches
Dezimalkomma, CRLF, UTF-8 mit BOM, Präambel mit Geräte-Frame-Version und
`frame_version_gemischt`.

**Weiterhin offen (App):** Verlaufs-Gesamtübersicht, finale SF-Symbol-Wahl,
optionale Höhen-Referenzdruck-Kalibrierung — Future Work.

---

## 6. Historie (verdichtet)

Ältere Arbeitsstände sind kanonisch in der Project Bible geführt. Kurzabriss:
Feldtest 06.08.2026 mit sechs Fahrten falsifizierte die Bremserkennung
(r = −0,132, n = 939; die Zahl ist methodisch nicht belastbar, weil die Latenz
der GNSS-Referenzkette unberücksichtigt blieb) und führte zur
Architekturentscheidung V-B (IMU als schneller Regelpfad, GNSS als langsame
Stützreferenz). Die Reparaturrunde vom 07.08.2026 klärte den Ruhesockel von
3,0 m/s² (unkompensierter Gyro-Nullpunktfehler von −4,61 °/s) und das praktisch
unerreichbare Kalibrierfenster. Davor: BLE-Transport (NimBLE) nach Board-Tausch
validiert, Brownout-Fallstudie dokumentiert; Serial-Bench-Validierung der
Bremslicht-Kennlinie; I²C-Recovery, Fail-Safe und Task-Watchdog per
Fehlerinjektion verifiziert. Am 09.08.2026 wurde die Elektronik vollständig
geklärt und als KiCad-Schaltplan Rev. 1.1 dokumentiert.
