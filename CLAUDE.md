# CLAUDE.md — Projektkontext für Claude Code

Dies ist die Kontextdatei für Claude Code. Sie wird beim Start automatisch geladen. **Lies zuerst `docs/project_bible.md`** — das ist die Single Source of Truth (SSOT) dieses Projekts.

## Projekt in einem Satz
Firmware für ein „Smart Bike Rear Light" (ESP32): IMU-gesteuertes Schluss-/Bremslicht, 433-MHz-Funk-Blinker und BLE-Telemetrie an eine Web-App. Bachelorarbeit, wissenschaftlich dokumentiert.

## Goldene Regeln
1. **Die Project Bible sticht.** Bei Widerspruch zwischen Code, Chat und `docs/project_bible.md` gilt die Bible. Implementiere **gegen die SRS** (Kap. 2 der Bible), nicht gegen Bauchgefühl.
2. **Jede Anforderung hat eine ID** (z. B. `FR-TL-06`, `NFR-RT-03`). Referenziere im Code-Kommentar die ID(s), die ein Modul umsetzt — das ist die Traceability für die Thesis.
3. **Nichts erfinden.** Fehlt eine Angabe, frage nach oder markiere sie als `TODO(offen)` — keine stillen Annahmen.
4. **Kein Code ohne Test.** Reine Logik bekommt einen Host-Unit-Test (siehe Teststrategie).

## Coding-Konventionen (aus dem Engineering Charter)
- **Nicht-blockierend:** kooperativer `millis()`-Scheduler, **kein `delay()`** (NFR-RT-03). Jeder Task-Schritt ist kurz; Worst-Case-Loop < 10 ms (NFR-RT-04).
- **State Machines:** Firmware ist als vier parallele Regionen modelliert (Bible Kap. 6.6). Keine flache Mega-FSM.
- **PWM ausschließlich** über `ledcAttach()`/`ledcWrite()` (ESP32 Core v3.x). `ledcSetup()`/`ledcAttachPin()` sind verboten (deprecated).
- **Statische Speicherverwaltung:** keine dynamische Allokation im laufenden Betrieb (NFR-RES-02). Ringpuffer & Puffer vorreserviert.
- **Kommentare erklären das WARUM,** nicht das WAS. Bei nicht-offensichtlichen Werten die Quelle nennen (SRS-ID oder Norm, z. B. „// 1,5 Hz per ECE R6 / FR-BLK-08").
- **Module modular, wartbar, erweiterbar;** klare Schnittstellen je Region (NFR-EXT-01).
- **Sprache:** Code-Bezeichner Englisch, Kommentare Deutsch (Thesis-Sprache) sind ok.

## Architektur (Kurzfassung — Details in Bible Kap. 6)
Vier parallele Regionen, ausgeführt vom kooperativen Scheduler in `loop()`:
- **R1 Lebenszyklus:** `S_INIT` → `S_RUN` (kritischer Sensor = IMU; Init-Timeout 5 s → degradierter RUN). `S_FAULT` reserviert.
- **R2 Rücklicht (rote LED, GPIO26):** `TL_INIT_BLINK` → `TL_SCHLUSSLICHT` ⇄ `TL_BREMSLICHT` ⇄ `TL_NOTBREMS_BLINKEN` (experimentell, default AUS). Zustände exklusiv.
- **R3 Blinker (gelbe LEDs, GPIO25/27):** `BLK_AUS`/`LINKS`/`RECHTS`/`WARN`. Steuerung nur per 433-MHz-Funk.
- **R4 Erfassung/Telemetrie:** Sampling (IMU 100 Hz, BMP 10 Hz, GNSS 1 Hz), Telemetrie 10 Hz, RAM-Ringpuffer bei fehlender BLE-Verbindung.

**Fail-safe-Leitprinzip (FR-SAF-01):** Bei jedem Fehler bleibt das rote Schlusslicht an. Priorität: Schlusslicht > Bremslicht > Blinker > Telemetrie.

## Testbarkeit (NFR-TST) — Pflicht-Architekturprinzip
**Reine Logik strikt von Hardware trennen** (NFR-TST-01):
- `firmware/lib/logic/` — hardwarefreie Logik (Zustandsmaschinen, Bremskennlinie FR-TL-06, RF-Tastenerkennung FR-RF-03/04, Plausibilitätsprüfung). **Kein `#include <Arduino.h>` hier.** Diese Module werden host-seitig getestet.
- `firmware/lib/drivers/` — Hardware-Treiber (Sensor-Lesung, PWM, RF-Dekodierung). Kapseln Arduino/Espressif-APIs.
- `firmware/src/main.cpp` — verdrahtet Treiber + Logik über den Scheduler.
Logik bekommt Eingaben als einfache Werte/Strukturen, nie direkte Hardwarezugriffe → dadurch auf dem PC testbar.

## Build & Test (PlatformIO)
```bash
# Firmware bauen / flashen / Monitor
pio run -e esp32dev
pio run -e esp32dev -t upload
pio device monitor -b 115200

# Host-Unit-Tests der Logik (ohne ESP32)
pio test -e native
```

## Repository-Struktur
```
firmware/    PlatformIO-Projekt (src, include, lib/logic, lib/drivers, test)
webapp/      PWA (Web Bluetooth) — später
docs/        project_bible.md (SSOT-Kopie) + Wissensdatenbank
hardware/    Schaltplan, BOM
cad/         Gehäuse — später
testdata/    aufgezeichnete Sensordaten für Logik-Tests (NFR-TST-02)
```

## Wissensdatenbank & automatische Pflege
Am Ende jeder Arbeitseinheit prüfen, ob Doku aktualisiert werden sollte. Zwei Klassen:

**Darfst du eigenständig aktualisieren (dann committen):**
- `docs/current_context.md` — woran gerade gearbeitet wird, nächster Schritt.
- `docs/open_issues.md` — neue/geschlossene technische To-dos.
- `docs/lessons_learned.md` — Stolpersteine & Erkenntnisse.
- `docs/roadmap.md` — Fortschritt der Meilensteine.

**Nur mit ausdrücklicher Freigabe des Nutzers ändern (Änderung vorschlagen, nicht selbst schreiben):**
- `docs/project_bible.md` — die SSOT (kanonisch im claude.ai-Projekt gepflegt).
- `docs/decision_log.md` — architektonische Entscheidungen.
Bei einer nötigen Bible-/Decision-Log-Änderung: formuliere den Vorschlag und frage: „Soll ich das in die Project Bible / den Decision Log übernehmen?"

## Arbeitsweise mit Claude Code
1. **Verstehen:** relevante SRS-Anforderungen aus der Bible lesen, betroffene Module benennen.
2. **Planen:** kurz Modul + Schnittstelle + Test skizzieren, Zustimmung einholen.
3. **Implementieren:** Modul für Modul, Logik vor Hardware, mit Host-Test.
4. **Verifizieren:** `pio test -e native`, bei Hardware `pio run` + On-Target-Prüfung.
5. **Dokumentieren:** `current_context.md`/`open_issues.md` pflegen; Bible-/Decision-Log-Änderungen vorschlagen.

## Wichtige offene Punkte (vor Implementierung beachten)
- LED-Kanalzuordnung / fehlendes Datenblatt (Bible Kap. 11.1) — betrifft FR-TL-06-Kalibrierung.
- FR-TL-07 (Notbrems-Blinken) ist **default deaktiviert** — § 67 Abs. 4 StVZO (blinkende Schlussleuchte unzulässig).
- RF-Release-Timeout (~150 ms) ist **vorläufig** — final erst nach Verifikationstest (FR-RF-03).
