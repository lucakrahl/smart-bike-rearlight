# CSV-Format — Validierungsexport der iOS-App (Frame-Schema v3)

**Smart Bike Rear Light · Bachelorarbeit Krahl**
**Stand 07.08.2026 · gültig ab App-Schema-v3 (AP0–AP8) · Quelle: `RideCSVExporter`**

Dieses Dokument beschreibt das **verbindliche Format** der von der iOS-App exportierten Fahrt-CSV. Es ist die Referenz für jede **externe Auswertung** (Excel, Python/pandas). Die Spaltenreihenfolge ist per Golden-Header-Test im Code eingefroren; Änderungen am Format sind ohne Anpassung dieses Dokuments unzulässig.

---

## 1. Datei-Konventionen

| Eigenschaft | Wert | Grund |
|---|---|---|
| Trennzeichen | Semikolon `;` | deutsches Excel |
| Dezimaltrennzeichen | Komma `,` | deutsches Excel |
| Zeilenende | CRLF (`\r\n`) | Excel-Kompatibilität |
| Kodierung | UTF-8 **mit BOM** | Umlaute in Excel korrekt |
| Excel-Hinweis | erste Zeile `sep=;` | erzwingt Semikolon-Trennung |
| Leere Optionalwerte | leere Zelle | z. B. v3-Spalten bei einer v2-Fahrt |

## 2. Aufbau der Datei

1. Zeile `sep=;`
2. Kommentar-Präambel (`#`-Zeilen, s. Abschnitt 3)
3. Header-Zeile mit **35 Spaltennamen**
4. Datenzeilen (eine je Sample; Rate 1 Hz oder 10 Hz, s. App-Einstellung)

## 3. Präambel

```
sep=;
# SmartBikeRearLight Fahrt-Export
# schema_version;<Frame-Version des Geräts>
# frame_version_gemischt;<ja|nein>
# geraet;SmartBikeRearLight
# app_version;<x.y.z>
# start_utc;<ISO-8601>
# ende_utc;<ISO-8601>
```

- **`schema_version`** trägt die **Frame-Version des Geräts** (Firmware-Vertrag), **nicht** die App-Version. So ist erkennbar, gegen welchen Firmware-Vertrag eine Aufzeichnung entstand. Bei gemischten Versionen innerhalb einer Fahrt steht hier das **Minimum** der vorkommenden `frame_version` (der schwächste Fall bestimmt, welche Spalten durchgängig gefüllt sind).
- **`frame_version_gemischt`** = `ja`, wenn eine Fahrt Samples verschiedener Frame-Versionen enthält (sonst `nein`).
- **`app_version`** ist bewusst eine **eigene** Zeile, getrennt von `schema_version`.

## 4. Spalten (verbindliche Reihenfolge, 35)

Spalten 1–22 stammen aus dem v2-Umfang (`temperature_c` ist ab v3 **entfernt**, Entscheidung E-4). Spalten 23–35 sind die 13 v3-Diagnosefelder in Frame-Offset-Reihenfolge (81→111).

| # | Spalte | Einheit | Frame-Offset | Bedeutung |
|---|---|---|---|---|
| 1 | `t_s` | s | — (abgeleitet) | Monotone Aufzeichnungszeit (`RecordingClock`), **≥ 2 Nachkommastellen** (nötig bei 10 Hz) |
| 2 | `uhrzeit` | hh:mm:ss | — | Uhrzeit des Samples (aus GNSS-UTC bzw. abgeleitet) |
| 3 | `device_timestamp_ms` | ms | 2 | Roher Geräte-Zeitstempel (nicht monoton) |
| 4 | `speed_kmph` | km/h | 50 | GNSS-Geschwindigkeit |
| 5 | `distanz_km` | km | — (abgeleitet) | Kumulierte Distanz (nur bei gültigem Fix integriert) |
| 6 | `hoehe_m` | m | — (abgeleitet) | Barometrische Höhe aus `pressure_pa` |
| 7 | `pressure_pa` | Pa | 34 | BMP280-Luftdruck |
| 8 | `gnss_altitude_m` | m | 58 | GNSS-Höhe (nur Fallback) |
| 9 | `course_deg` | ° | 54 | Kurs über Grund |
| 10 | `fix_status` | Text | 78 | `FIX_OK` / `NO_FIX` / `NO_DATA` |
| 11 | `sats` | Anzahl | 62 | Satelliten |
| 12 | `hdop` | — | 63 | Horizontal Dilution of Precision |
| 13 | `lat` | ° | 42 | Breitengrad |
| 14 | `lon` | ° | 46 | Längengrad |
| 15 | `brake_decel_ms2` | m/s² | 30 | **Roher Verzögerungs-Eingang** (`motion_filter`) |
| 16 | `brake_light_pct` | % | 80 | **Kommandierte LED-Duty** (Ausgang der Bremslogik) |
| 17 | `imu_health` | Text | 76 | `OK` / `RECOVERING` / `FAILED` (Frame-Feld heißt `imu_health_state`) |
| 18 | `baro_valid` | 0/1 | 77 | BMP280 gültig |
| 19 | `system_state` | Text | 74 | `Init` / `RUN` |
| 20 | `init_degraded` | 0/1 | 75 | Start mit eingeschränkter Sensorik |
| 21 | `watchdog_recovered` | 0/1 | 79 | Watchdog-Reset erkannt |
| 22 | `frame_version` | 2/3 | 0 | Frame-Schemaversion dieses Samples |
| 23 | `gnss_accel_ms2` | m/s² | 81 | GNSS-Referenzverzögerung (nur gültig, wenn `gnss_accel_valid=1`, sonst 0) |
| 24 | `pitch_rad` | rad | 85 | Interne Lageschätzung des `motion_filter` |
| 25 | `gyro_bias_rads` | rad/s | 89 | Geschätzter Nullpunktfehler `gyro_x` |
| 26 | `norm_delta_min` | m/s² | 93 | Minimum von (‖a‖ − g) im 100-ms-Fenster |
| 27 | `norm_delta_max` | m/s² | 97 | Maximum von (‖a‖ − g) im 100-ms-Fenster |
| 28 | `jerk_max` | m/s² je 10 ms | 101 | Maximum von \|Δ‖a‖\| dt-normiert im Fenster |
| 29 | `regime_static_n` | Anzahl | 105 | STATIC-Samples im Fenster (typ. 0..10) |
| 30 | `regime_dynamic_n` | Anzahl | 106 | DYNAMIC-Samples im Fenster |
| 31 | `regime_shock_n` | Anzahl | 107 | SHOCK-Samples im Fenster |
| 32 | `bias_calibrated` | 0/1 | 108 | Stufe-1-Bias-Kalibrierung abgeschlossen |
| 33 | `gnss_accel_valid` | 0/1 | 109 | Gültigkeitsurteil der GNSS-Referenz |
| 34 | `dt_max_ms` | ms | 110 | Größtes `dt` im Fenster (gesättigt bei 255) |
| 35 | `loop_max_us` | µs | 111 | Längste Schleifendauer im Fenster (gesättigt bei 65535) |

## 5. Semantik der Fensteraggregate (Spalten 26–31, 34, 35)

Diese Felder beziehen sich auf das **100-ms-Fenster zwischen zwei Frames** (100-Hz-Innentakt der Firmware), nicht auf einen Momentanwert. Die Summe `regime_static_n + regime_dynamic_n + regime_shock_n` ist die Anzahl tatsächlich verarbeiteter IMU-Samples im Fenster (nominell 10; Abweichung ist selbst eine Messgröße und **nicht** auf 10 zu normieren). Bei Sensorausfall im Fenster: Zähler = 0, `norm_delta_*` = `jerk_max` = 0, `dt_max_ms` = `loop_max_us` = 0.

## 6. Auswertungshinweise

- **Bremslicht-Validierung:** `brake_decel_ms2` (Eingang) gegen `brake_light_pct` (Ausgang) — bei `imu_health = FAILED` zeigt der Ausgang korrekt den Fail-Safe-Wert (Schlusslicht).
- **Referenzvergleich:** `gnss_accel_ms2` nur verwenden, wenn `gnss_accel_valid = 1`.
- **Aufzeichnungsrate:** Bei 10 Hz liefern die Fensteraggregate die Sub-Sekunden-Feindynamik; bei 1 Hz sind sie grob unterabgetastet (für Dauerbetrieb).
- **Version filtern:** Über `schema_version` (Präambel) bzw. `frame_version` (Spalte 22) lässt sich unterscheiden, welche Zeilen v3-Diagnosefelder tragen; v2-Zeilen haben die Spalten 23–35 leer.

## 7. Beispiel (gemischte Fahrt, v3 + v2)

```
sep=;
# SmartBikeRearLight Fahrt-Export
# schema_version;2
# frame_version_gemischt;ja
# geraet;SmartBikeRearLight
# app_version;9.9.9
# start_utc;1970-01-01T00:00:00Z
# ende_utc;1970-01-01T00:01:00Z
t_s;uhrzeit;device_timestamp_ms;speed_kmph;distanz_km;hoehe_m;pressure_pa;gnss_altitude_m;course_deg;fix_status;sats;hdop;lat;lon;brake_decel_ms2;brake_light_pct;imu_health;baro_valid;system_state;init_degraded;watchdog_recovered;frame_version;gnss_accel_ms2;pitch_rad;gyro_bias_rads;norm_delta_min;norm_delta_max;jerk_max;regime_static_n;regime_dynamic_n;regime_shock_n;bias_calibrated;gnss_accel_valid;dt_max_ms;loop_max_us
1,50;00:00:01;1334;36,0;0,000;201,5;98900;206,0;10,0;FIX_OK;9;0,9;51,100000;6,700000;2,50;80;RECOVERING;1;RUN;0;1;3;3,10;0,1200;-0,0150;-1,200;2,700;4,400;3;5;2;1;1;12;1234
2,00;00:00:02;1234;10,0;0,003;200,0;98950;205,0;0,0;FIX_OK;9;1,0;51,100000;6,700000;0,00;0;OK;1;RUN;0;0;2;;;;;;;;;;;;;
```

Die v2-Zeile (letzte) hat die 13 v3-Spalten leer.
