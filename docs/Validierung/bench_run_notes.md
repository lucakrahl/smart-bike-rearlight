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
