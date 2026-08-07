# BLE-Telemetrie-Frame — Schnittstellenvertrag Schema v3

**Status:** verbindlicher Umsetzungsvertrag für Firmware und iOS-App.
**Stand:** 07.08.2026 · **Vorgänger:** Schema v2 (81 Byte, Project Bible v0.14)
**Eintrag in Project Bible / App Bible:** folgt in Arbeitsschritt 4 (Doku-Durchgang).

Dieses Dokument ist die einzige gemeinsame Wahrheitsquelle für die beiden
getrennten Toolchains (Claude Code / VS Code für die Firmware, Claude in Xcode
für die iOS-App). Beide Seiten setzen ausschließlich gegen diese Tabelle um.
Abweichungen sind Fehler, auch wenn sie „funktionieren".

---

## 1. Warum v3

Drei Bedarfe, die alle aus dem Feldtest vom 06.08.2026 und der Stufe-1-Sanierung
des `motion_filter` stammen:

**(a) Referenzgröße im Frame (E1, freigegeben).** Der Feldtest musste die
GNSS-Referenzverzögerung nachträglich aus der CSV differenzieren. Das ist
fehleranfällig und nicht reproduzierbar. Die Referenz gehört synchron in
dasselbe Frame wie der IMU-Wert, damit Ist und Soll ohne Nachbearbeitung
gegenübergestellt werden können.

**(b) Sichtbarkeit der Filter-Innensicht.** Stufe 1 hat drei Schwellwerte
eingeführt (`MOTION_NORM_STATIC_BAND`, `_JERK_DELTA`, `_SHOCK_DELTA`), die
bisher **geschätzt und nicht gemessen** sind. Sie lassen sich nur parametrieren,
wenn die Feldaufzeichnung die Größen enthält, auf die sie wirken: den
Normbetragsabstand ‖a‖ − g, die Änderungsrate und die resultierende
Regime-Entscheidung. Ebenso muss im Feld überprüfbar sein, ob die
Bias-Kalibrierung überhaupt greift — sonst läuft das System unbemerkt im
konservativen Rückfall (τ_slow = 30 s statt 90 s).

**(c) Ratenproblem: 100 Hz Innenleben, 10 Hz Frame.** Die Regime-Entscheidung
und das Jerk-Kriterium leben auf dem 100-Hz-Abtasttakt. Ein 10-Hz-Frame würde
sie um den Faktor zehn unterabtasten — genau die Stöße, um deren Erkennung es
geht, verschwinden dabei. Lösung: Es werden **keine Momentanwerte, sondern
Aggregate über das 100-ms-Fenster** übertragen (Minimum, Maximum, Zählwerte).
Damit bleibt die 100-Hz-Information erhalten, ohne die Datenrate anzuheben.

Zusätzlich schließt v3 die Messlücke bei NFR-RT-04: Die Bench-Zeitstatistik
charakterisiert den Bench-Harness, nicht den produktiven Scheduler. Die Felder
`dt_max_ms` und `loop_max_us` messen das Zeitverhalten im Normalbetrieb.

---

## 2. Unveränderte Rahmenbedingungen

| Eigenschaft | Wert |
|---|---|
| Geräte-Name | `SmartBikeRearLight` |
| Service-UUID | `587bb505-9f9d-4ae0-96fd-0b29adfc4b03` |
| Characteristic (NOTIFY) | `8c604d09-743f-4850-9109-19604a17f358` |
| Richtung | unidirektional ESP32 → App (FR-SYS-04), kein Write |
| Byte-Reihenfolge | Little-Endian, gepackt, keine Alignment-Annahmen |
| Senderate | 10 Hz |
| Reconnect/Backfill | Advertising automatisch, Nachlieferung gepufferter Frames |

**Neu:** Framelänge **113 Byte** (v2: 81), `version` = **3** (Offset 0).
Erforderliche MTU ≥ 116; ausgehandelt werden 185 (Nutzlast 182) — weiterhin
eine Notification pro Frame, keine Fragmentierung.

---

## 3. Frame-Layout v3

### 3.1 Offsets 0–80 — unverändert aus v2

Die v2-Belegung bleibt **byte-identisch**. Ein v2-Decoder, der die ersten
81 Byte liest, funktioniert an einem v3-Gerät weiter.

| Off | Bytes | Typ | Feld |
|---|---|---|---|
| 0 | 2 | uint16 | `version` (=3) |
| 2 | 4 | uint32 | `timestamp_ms` |
| 6 | 4 | float | `accel_x_ms2` |
| 10 | 4 | float | `accel_y_ms2` |
| 14 | 4 | float | `accel_z_ms2` |
| 18 | 4 | float | `gyro_x_rads` |
| 22 | 4 | float | `gyro_y_rads` |
| 26 | 4 | float | `gyro_z_rads` |
| 30 | 4 | float | `brake_decel_ms2` — roher Eingang aus `motion_filter` |
| 34 | 4 | float | `pressure_pa` |
| 38 | 4 | float | `temperature_c` |
| 42 | 4 | float | `lat` |
| 46 | 4 | float | `lon` |
| 50 | 4 | float | `speed_kmph` |
| 54 | 4 | float | `course_deg` |
| 58 | 4 | float | `altitude_m` |
| 62 | 1 | uint8 | `sats` |
| 63 | 4 | float | `hdop` |
| 67 | 2 | uint16 | `utc_year` |
| 69 | 1 | uint8 | `utc_month` |
| 70 | 1 | uint8 | `utc_day` |
| 71 | 1 | uint8 | `utc_hour` |
| 72 | 1 | uint8 | `utc_minute` |
| 73 | 1 | uint8 | `utc_second` |
| 74 | 1 | uint8 | `system_state` (0=Init, 1=Run) |
| 75 | 1 | uint8 | `init_degraded` (0/1) |
| 76 | 1 | uint8 | `imu_health_state` (0=OK, 1=RECOVERING, 2=FAILED) |
| 77 | 1 | uint8 | `baro_valid` (0/1) |
| 78 | 1 | uint8 | `gnss_fix_status` (0=NO_DATA, 1=NO_FIX, 2=FIX_OK) |
| 79 | 1 | uint8 | `watchdog_recovered` (0/1) |
| 80 | 1 | uint8 | `brake_light_pct` (0..100, kommandierte LED-Duty) |

### 3.2 Offsets 81–112 — neu in v3

| Off | Bytes | Typ | Feld | Einheit / Wertebereich |
|---|---|---|---|---|
| 81 | 4 | float | `gnss_accel_ms2` | m/s², positiv = Verzögerung. Nur gültig, wenn `gnss_accel_valid == 1`; sonst `0.0f` |
| 85 | 4 | float | `pitch_rad` | rad, interne Lageschätzung des `motion_filter` |
| 89 | 4 | float | `gyro_bias_rads` | rad/s, geschätzter Nullpunktfehler `gyro_x` |
| 93 | 4 | float | `norm_delta_min` | m/s², Minimum von (‖a‖ − g) im 100-ms-Fenster |
| 97 | 4 | float | `norm_delta_max` | m/s², Maximum von (‖a‖ − g) im 100-ms-Fenster |
| 101 | 4 | float | `jerk_max` | m/s² je 10 ms, Maximum von \|Δ‖a‖\| dt-normiert im Fenster |
| 105 | 1 | uint8 | `regime_static_n` | Anzahl STATIC-Samples im Fenster (typ. 0..10) |
| 106 | 1 | uint8 | `regime_dynamic_n` | Anzahl DYNAMIC-Samples im Fenster |
| 107 | 1 | uint8 | `regime_shock_n` | Anzahl SHOCK-Samples im Fenster |
| 108 | 1 | uint8 | `bias_calibrated` | 0/1 — Stufe-1-Bias-Kalibrierung abgeschlossen |
| 109 | 1 | uint8 | `gnss_accel_valid` | 0/1 — Gültigkeitsurteil aus `gnss_speed_ref` |
| 110 | 1 | uint8 | `dt_max_ms` | ms, größtes `dt_s` im Fenster, gesättigt bei 255 |
| 111 | 2 | uint16 | `loop_max_us` | µs, längste Schleifendauer im Fenster, gesättigt bei 65535 |

**Gesamtlänge: 113 Byte.**

### 3.3 Semantik der Fensteraggregate

Die Felder ab Offset 93 sowie die drei Regime-Zähler beziehen sich auf das
**100-ms-Fenster zwischen zwei Frames**, nicht auf einen Momentanwert. Sie
werden im 100-Hz-Pfad akkumuliert und beim Senden des Frames zurückgesetzt.

Die Summe der drei Regime-Zähler entspricht der Anzahl tatsächlich
verarbeiteter IMU-Samples im Fenster. Sie ist nominell 10, weicht bei
Taktschwankung aber ab — diese Abweichung ist selbst eine Messgröße und darf
nicht auf 10 normiert werden.

Enthält ein Fenster keine Samples (Sensorausfall), gilt:
Zähler = 0, `norm_delta_min` = `norm_delta_max` = `jerk_max` = `0.0f`,
`dt_max_ms` = 0, `loop_max_us` = 0.

### 3.4 Ausrichtung

Wie schon in v2 (`hdop` auf Offset 63) sind mehrere Felder nicht typ-ausgerichtet.
Firmware-seitig gilt `#pragma pack(1)` bzw. `memcpy`; App-seitig zwingend
`withUnsafeBytes` + `loadUnaligned`. Ein direkter Pointer-Cast ist auf beiden
Seiten unzulässig.

---

## 4. Versions- und Kompatibilitätsregeln

Die App prüft **nicht auf Gleichheit**, sondern auf Mindestversion und
Mindestlänge:

- `version >= 2` **und** Länge ≥ 81 → v2-Felder lesen
- `version >= 3` **und** Länge ≥ 113 → zusätzlich v3-Felder lesen
- Längere Frames als erwartet: überzählige Bytes ignorieren, nicht verwerfen
  (Vorwärtskompatibilität, AR-CONN-04)
- Kürzere Frames als 81 Byte: verwerfen und als Fehler zählen

Damit bleibt ein v2-Gerät an einer v3-App lauffähig und umgekehrt. Das ist keine
Bequemlichkeit, sondern Voraussetzung dafür, dass Firmware und App in zwei
getrennten Toolchains unabhängig voneinander weiterentwickelt werden können.

---

## 5. Auswirkung auf Ressourcen

| Größe | v2 | v3 | Delta |
|---|---|---|---|
| Framelänge | 81 B | 113 B | +32 B |
| Ringpuffer (600 Frames) | 48 600 B | 67 800 B | +19 200 B |
| RAM gesamt (geschätzt) | 87 632 B (26,7 %) | ≈ 106 800 B (≈ 32,6 %) | +5,9 Prozentpunkte |
| BLE-Nutzlast je Notification | 81 von 182 B | 113 von 182 B | weiterhin unfragmentiert |
| Datenrate | 810 B/s | 1130 B/s | +40 % |

Der Ringpufferzuwachs ist der dominierende Posten. Er ist vertretbar, muss aber
nach dem Build gegen den tatsächlichen Wert geprüft werden. Sollte der
RAM-Verbrauch 40 % überschreiten, ist die Ringpuffertiefe von 600 auf 400 Frames
(40 s Backfill) zu reduzieren — **nicht** die Diagnosefelder zu streichen.

---

## 6. Abgrenzung: was v3 **nicht** enthält

Die GNSS-**Fusion** (langsame Biaskorrektur, τ_b ≈ 10 s) bleibt hinter ihrem
Konfigurationsflag und ist deaktiviert (Entscheidung E5). v3 überträgt die
GNSS-Referenz ausschließlich zur **Beobachtung**, nicht zur Korrektur. Nur so
bleibt der Vorher-Nachher-Vergleich der Stufe 1 unverfälscht — würde die Fusion
mitlaufen, ließe sich hinterher nicht mehr trennen, welcher Anteil der
Verbesserung vom Normbetrags-Gate und welcher von der GNSS-Stützung stammt.

Ebenfalls nicht enthalten: die Umkonfiguration des L86 auf 5 Hz / 115200 Bd
(Stufe 2). `gnss_speed_ref` arbeitet in v3 auf den vorhandenen 1-Hz-Fixes. Die
daraus abgeleitete Referenzbeschleunigung ist entsprechend grob — das ist für
eine Korrelationsanalyse ausreichend und war auch die Datenbasis des Feldtests
vom 06.08.2026.

---

## 7. CSV-Export der App

Der Export wird von 23 auf **35 Spalten** geändert (23 − 1 entfallen + 13 neu):

- **entfällt:** `temperature_c` (Entscheidung E4 — im Feldtest durchgängig leer)
- **neu:** `gnss_accel_ms2`, `gnss_accel_valid`, `pitch_rad`, `gyro_bias_rads`,
  `norm_delta_min`, `norm_delta_max`, `jerk_max`, `regime_static_n`,
  `regime_dynamic_n`, `regime_shock_n`, `bias_calibrated`, `dt_max_ms`,
  `loop_max_us`

Format unverändert: Trennzeichen `;`, deutsches Dezimalkomma, CRLF,
Kopfzeilen-Präambel mit `schema_version`.

**Wichtig:** In der Präambel muss `schema_version` die **Frame-Version des
Geräts** (3) tragen, nicht die App-Version. Nur so ist später erkennbar, mit
welchem Firmware-Vertrag eine Aufzeichnung entstanden ist.

---

## 8. Aufzeichnungsrate (E2, freigegeben)

Neben der bestehenden 1-Hz-Verdichtung erhält die App einen umschaltbaren
**Validierungsmodus mit 10 Hz**, also ohne Verdichtung. Begründung: Die
Bremsdynamik spielt sich in 0,3 bis 3 s ab; bei 1 Hz liegen über einer
typischen Bremsung ein bis drei Stützstellen — zu wenig, um Anstieg,
Haltezeit und Abfall zu belegen. Für den Dauerbetrieb bleibt 1 Hz die
Voreinstellung (Speicherbedarf, Akku).

Speicherabschätzung 10 Hz: rund 35 Spalten × ca. 12 Byte × 10 Hz ≈ 4 kB/s,
also ≈ 14 MB je Stunde CSV. Für Messfahrten von 15 bis 30 Minuten unkritisch.
