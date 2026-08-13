# Golden-Referenz-Frame v3 — Schnittstellen-Kreuztest

**Zweck:** `frame_v3_golden.hex` enthält die 113 Byte (226 Hex-Zeichen, eine
Zeile) eines mit fest definierten, gut unterscheidbaren Werten serialisierten
Schema-v3-Telemetrie-Frames. Firmware (`firmware/test/test_frame_v3_golden/`)
und App testen damit **dieselbe eingefrorene Bytefolge**, nicht nur die
Symmetrie ihres eigenen Encoders gegen ihren eigenen Decoder — ein
gemeinsamer Denkfehler (falscher Offset, falsche Skalierung) auf beiden
Seiten bliebe sonst unsichtbar.

**Quelle:** `firmware/lib/logic/telemetry_frame.cpp::telemetryFrameSerialize()`,
Werte definiert in `firmware/test/test_frame_v3_golden/test_frame_v3_golden.cpp::buildGoldenFrame()`.
Layout gemäß `docs/BLE_Frame_v3_Schnittstelle.md` Kap. 3.

**Herkunft der Bytefolge:** erzeugt mit Commit `1178017` vom 07.08.2026
(letzte Änderung an `frame_v3_golden.hex` sowie an
`telemetry_frame.cpp`/`.h`, im Repo per `git log` verifiziert); seither
unverändert. Ablieferungsstand der Firmware ist Commit `835c7b3` vom
10.08.2026 — die Bytefolge selbst ist davon nicht betroffen, da Schema v3
und Serialisierung seit `1178017` unangetastet blieben. Umfang: 44 Felder
/ 113 Byte / 39 unterscheidbare Werte (s. Werte-Tabelle unten).

**Diese Datei ist bewusst eingefroren, nicht automatisch nachzuziehen.**
Ändert sich das Frame-Layout oder eine Skalierung absichtlich, müssen
`frame_v3_golden.hex` und diese Tabelle bewusst neu erzeugt (Firmware-Test
einmal mit gelöschter `.hex`-Datei laufen lassen) und im Diff geprüft werden.

## Werte-Tabelle

Float-Werte sind IEEE-754-`float32` — der App-seitige Vergleich sollte mit
einer kleinen Toleranz (z. B. `1e-4`) erfolgen, nicht auf exakte Gleichheit
(s. Spalte „Serialisiert", die den tatsächlich aus der Bytefolge dekodierten
`float32`-Wert zeigt).

| Offset | Bytes | Typ | Feld | Gesetzter Wert | Serialisiert (float32) |
|---|---|---|---|---|---|
| 0 | 2 | uint16 | `version` | 3 | — |
| 2 | 4 | uint32 | `timestamp_ms` | 305419896 (0x12345678) | — |
| 6 | 4 | float | `accel_x_ms2` | 1.1 | 1.100000023841858 |
| 10 | 4 | float | `accel_y_ms2` | 2.2 | 2.200000047683716 |
| 14 | 4 | float | `accel_z_ms2` | 3.3 | 3.299999952316284 |
| 18 | 4 | float | `gyro_x_rads` | 4.4 | 4.400000095367432 |
| 22 | 4 | float | `gyro_y_rads` | 5.5 | 5.5 |
| 26 | 4 | float | `gyro_z_rads` | 6.6 | 6.599999904632568 |
| 30 | 4 | float | `brake_decel_ms2` | 7.7 | 7.699999809265137 |
| 34 | 4 | float | `pressure_pa` | 101325.5 | 101325.5 |
| 38 | 4 | float | `temperature_c` | 23.4 | 23.399999618530273 |
| 42 | 4 | float | `lat` | 51.2277 (double, Downcast) | 51.227699279785156 |
| 46 | 4 | float | `lon` | 6.7735 (double, Downcast) | 6.773499965667725 |
| 50 | 4 | float | `speed_kmph` | 25.5 | 25.5 |
| 54 | 4 | float | `course_deg` | 123.4 | 123.4000015258789 |
| 58 | 4 | float | `altitude_m` | 45.6 | 45.599998474121094 |
| 62 | 1 | uint8 | `sats` | 11 | — |
| 63 | 4 | float | `hdop` | 1.23 | 1.2300000190734863 |
| 67 | 2 | uint16 | `utc_year` | 2026 | — |
| 69 | 1 | uint8 | `utc_month` | 8 | — |
| 70 | 1 | uint8 | `utc_day` | 7 | — |
| 71 | 1 | uint8 | `utc_hour` | 15 | — |
| 72 | 1 | uint8 | `utc_minute` | 42 | — |
| 73 | 1 | uint8 | `utc_second` | 33 | — |
| 74 | 1 | uint8 | `system_state` | 1 (Run) | — |
| 75 | 1 | uint8 | `init_degraded` | 1 (true) | — |
| 76 | 1 | uint8 | `imu_health_state` | 2 (FAILED) | — |
| 77 | 1 | uint8 | `baro_valid` | 1 (true) | — |
| 78 | 1 | uint8 | `gnss_fix_status` | 2 (FIX_OK) | — |
| 79 | 1 | uint8 | `watchdog_recovered` | 1 (true) | — |
| 80 | 1 | uint8 | `brake_light_pct` | 88 | — |
| 81 | 4 | float | `gnss_accel_ms2` | 8.8 | 8.800000190734863 |
| 85 | 4 | float | `pitch_rad` | 0.1234 | 0.1234000027179718 |
| 89 | 4 | float | `gyro_bias_rads` | 0.005678 | 0.0056779999285936356 |
| 93 | 4 | float | `norm_delta_min` | -1.11 | -1.1100000143051147 |
| 97 | 4 | float | `norm_delta_max` | 9.99 | 9.989999771118164 |
| 101 | 4 | float | `jerk_max` | 1.357 | 1.3569999933242798 |
| 105 | 1 | uint8 | `regime_static_n` | 3 | — |
| 106 | 1 | uint8 | `regime_dynamic_n` | 6 | — |
| 107 | 1 | uint8 | `regime_shock_n` | 2 | — |
| 108 | 1 | uint8 | `bias_calibrated` | 1 (true) | — |
| 109 | 1 | uint8 | `gnss_accel_valid` | 1 (true) | — |
| 110 | 1 | uint8 | `dt_max_ms` | 13 | — |
| 111 | 2 | uint16 | `loop_max_us` | 4567 | — |

**Gesamtlänge: 113 Byte.**

## Verwendung app-seitig

`frame_v3_golden.hex` als `Data` einlesen, durch den v3-Decoder schicken und
jedes Feld gegen die Spalte „Gesetzter Wert" prüfen (Floats mit Toleranz,
s. o.). Ein Fehlschlag bedeutet: Offset, Typ, Skalierung oder Byte-Reihenfolge
weicht zwischen Firmware-Encoder und App-Decoder ab — genau der
Fehlerklasse, die die getrennten Symmetrie-Tests beider Seiten nicht
aufdecken können.
