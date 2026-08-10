# Firmware — Thesis-Transfer · Teil 1: Dokumenten-Statusübersicht

**Smart Bike Rear Light · Bachelorarbeit Krahl · erstellt 10.08.2026**
**Bezug: Prüfauftrag Punkt 1 — Aktualität aller firmwarerelevanten Dokumente.**
**Bezugsstand: Firmware-Abschluss, Commit `835c7b3` vom 10.08.2026.**

Statuslegende: **final** (eingefroren) · **aktuell** (gepflegt, gültig) · **historisch** (bewusst als Vorzustand erhalten) · **überholt** (durch neueren Stand ersetzt) · **verworfen** · **offen** · **widersprüchlich** (aktive Inkonsistenz, Abgleich nötig).

---

## A. Firmware-SSOT & Spezifikation

| Dokument | Version/Stand | Inhalt | Aktuell? | Überholte Inhalte | Fehlende Aktualisierung | Thesis-Relevanz |
|---|---|---|---|---|---|---|
| `project_bible.md` / `claude/project_bible.md` / Repo `docs/project_bible.md` | **v0.19 · 10.08.2026** | Single Source of Truth des Gesamtsystems: SRS (Blöcke A–H), Architektur, Hardware, Elektronik, Firmware, Validierung, Entscheidungen, offene Punkte, Risiken, Abgrenzung | **aktuell** (in dieser Session auf v0.19 gebracht) | — | — | **sehr hoch** — Primärquelle Kap. 6 und Kap. 3 |
| `firmware/include/config.h` | Commit `835c7b3` | Alle Kalibrier- und Strukturkonstanten; einzige Konfigurationsquelle nach dem Umfangsschnitt | **final** (eingefroren) | „NVS-überschreibbar"-Kommentare (in `835c7b3` entfernt) | — | **sehr hoch** — Parameterwerte für Kap. 6.3/6.5, Anhang B |
| `firmware/include/pins.h` | Commit `835c7b3` | GPIO-Zuordnung, verbindliche Pinquelle | **final** | — | — | hoch — Kap. 5.5, 6.1, Anhang A |
| `claude/BLE_Frame_v3_Schnittstelle.md` / Repo `docs/` | 07.08.2026 | Verbindlicher Frame-Vertrag Schema v3, 113 Byte, Offsets, Aggregat-Semantik, Versionsregel, RAM-/Datenratenbilanz | **aktuell** | Zeile „Eintrag in Project Bible / App Bible: folgt in Arbeitsschritt 4" — erledigt | Zwei Formalien fehlen: ausdrückliche **Freeze-Kennzeichnung** und der **Firmware-Git-Hash**, gegen den kreuzvalidiert wurde (`1178017` für den Golden-Vektor) | **sehr hoch** — Kap. 7.1, Schnittstellennachweis |
| `testdata/frame_v3_golden.hex` + `.md` | Erzeugt unter Commit `1178017` | Eingefrorene Referenz-Bytefolge, 113 Byte, plus Wertetabelle; Grundlage des Kreuztests über die Toolchain-Grenze | **aktuell** | — | **Git-Hash `1178017` fehlt** in der `.md`; **Feldanzahl widersprüchlich** (Firmware: „41 unterscheidbare Werte", App: 43 getroffene Felder) | **sehr hoch** — der einzige geräteunabhängige Schnittstellennachweis |

---

## B. Wissensdatenbank (system-tragend, mit Firmware-Anteil)

| Dokument | Version/Stand | Inhalt | Aktuell? | Überholte Inhalte | Fehlende Aktualisierung | Thesis-Relevanz |
|---|---|---|---|---|---|---|
| `decision_log.md` | **10.08.2026** | Begründungen aller technischen Entscheidungen, je Eintrag Entscheidung · Begründung · verworfene Alternative | **aktuell** (in dieser Session um sechs Einträge ergänzt) | — | — | **sehr hoch** — Grundlage der „Entscheidung → Begründung"-Kette in Kap. 4, 5, 6 |
| `current_context.md` | **10.08.2026** | Arbeitsstand, Kernbelege, Zahlen des Abschlussstands | **aktuell** (in dieser Session neu geschrieben) | — | — | mittel — Arbeitsdokument, keine Zitierquelle |
| `open_issues.md` | **10.08.2026** | Offene Punkte, nach Firmware / Hardware / Dokumentation getrennt | **aktuell** (in dieser Session neu strukturiert) | — | — | hoch — speist Kap. 10.3 und den Ausblick |
| `lessons_learned.md` | **10.08.2026** | Problem → Ursache → Lösung/Erkenntnis, neun Einträge | **aktuell** (in dieser Session um drei Einträge ergänzt) | — | — | hoch — Kap. 10.3 Methodenkritik, Kap. 1.3 Methodik |
| `roadmap.md` | **10.08.2026** | Meilensteine M0–M7 mit Status | **aktuell** (in dieser Session nachgezogen) | — | — | mittel — Kap. 1.3 Vorgehensmodell |
| `ble_brownout_fallstudie.md` | 04.08.2026 | Vollständige Root-Cause-Analyse des BLE-Brownout-Bootloops, zehn systematische Versuche | **aktuell** | — | Retest nach Pufferkondensator nicht durchgeführt (abgegrenzt) | **sehr hoch** — Musterbeispiel systematischer Fehlereingrenzung, Kap. 5.4/9.5/10.3 |
| `CLAUDE.md` (Repo-Wurzel) | 06.08.2026 | Arbeitsregeln für Claude Code, u. a. BLE-Vertrag | **überholt** | Schreibt den **v2-Vertrag** fest („81 Byte, `version == 2`"), nennt Persistenz fix 1 Hz und listet BLE/SwiftData/Views als Stubs | Nachzug auf v3 (113 B, Mindestversions-/Längenregel), 10-Hz-Modus | niedrig — Werkzeugdokument, **nicht zitieren**; als Beleg für den Arbeitsprozess (Anhang E, KI-Nutzung) verwertbar |
| `README.md` (Repo-Wurzel) | 06.08.2026 | Bootstrap-Anleitung des Xcode-Projekts | **überholt** | Abschnitt „Weiterbauen" ist vollständig abgearbeitet | Ist-Stand | niedrig |
| `Info-Setup.md` | 06.08.2026 | Info.plist/Capabilities der iOS-App | **aktuell** | — | — | niedrig — App-seitig, nicht Firmware |

---

## C. Validierungs- und Auswertungsdokumente

| Dokument | Version/Stand | Inhalt | Aktuell? | Anmerkung | Thesis-Relevanz |
|---|---|---|---|---|---|
| `docs/Validierung/bench_run_notes.md` | 07.08.2026 | Vollständiges Laborbuch der Bench-Läufe: Schritt A (Experimente A–F), Schritt A6 nach Harness-Fix, dt-Statistik, Reaktionszeiten, Regime-Verteilung, Vorher-Nachher Legacy vs. neu | **aktuell** | Legt selbst offen, dass die Schritt-A-Zahlen für D/E durch einen Harness-Bug verfälscht waren und in A6 korrigiert wurden. Die alten Zahlen stehen unkorrigiert daneben — beim Zitieren zwingend die A6-Werte verwenden | **sehr hoch** — Primärquelle Kap. 9.3 |
| `docs/Validierung/measurement_log.md` | 05.08.2026 | Messprotokoll der Serial-Bench-Experimente A/B/C | **widersprüchlich** | Drei Befunde, s. unten (Befund 1 und 2). Zusätzlich veraltet: Kap. 7 nennt den Feldtest als „(auszuführen)" | hoch — als Protokoll verwertbar **nach** Korrektur |
| `docs/Validierung/stufe1_normgate.md` | ~06.–07.08.2026 | Auslegung und Nachweis des Normbetrags-Gates, Totzonenrechnung T11, Begründung von `MOTION_COMPL_TAU_S` | **aktuell** | Kennzeichnet selbst korrekt: Werte per **Host-Simulation**, nicht am realen Board | hoch — Kap. 6.3, Auslegungsherleitung |
| `docs/Validierung/Feldtest_060826_Auswertung.md` / `claude/Feldtest_2026-08-06_Auswertung.md` | 06.08.2026, **Nachtrag 10.08.2026** | Falsifikationsversuch der IMU-Bremserkennung, sechs Fahrten, Fehlermechanismen A/B, GNSS-Eignungsprüfung, Variantenvergleich V-A/B/C | **aktuell** (in dieser Session um einen Nachtrag ergänzt) | Der Bericht selbst blieb unverändert (Beweisgrundlage); der Nachtrag korrigiert r = −0,132, hält den Stand von E1–E5 fest und verweist auf den erbrachten Wirksamkeitsnachweis | **sehr hoch** — Kap. 9.3, Kap. 10.3 |
| `docs/Messfahrt_2026-08-08_Auswertung.md` / `claude/…` | 09.08.2026, **Nachtrag 10.08.2026** | Feldnachweis Stufe 1, Befunde B1–B9, Statistik, sieben Abbildungen | **aktuell** (Nachtrag in dieser Session ergänzt) | Nachtrag: B5 behoben, B7-Ursachenzuordnung zurückgenommen, Einbaulage umgesetzt | **sehr hoch** — Kernnachweis Kap. 9.3 |
| `claude/Schaltplan_Dokumentation.md` + KiCad/PDF | **Rev. 1.1 · 09.08.2026** | Komponentenübersicht, Pinbelegung, Versorgungskonzept, Kommunikationsschnittstellen, offene Punkte B-1…B-6 | **aktuell** | Rev. 1.1 korrigiert die Position von SW1 | hoch — Kap. 5.5, Anhang A; für Kap. 6 nur als Pin-/Schnittstellenbeleg |
| `docs/Validierung/*.csv`, `abb_A/B/C.png`, `*.raw.log` | 05.–07.08.2026 | Rohdaten und Abbildungen der Bench-Läufe | **final** | `.raw.log` sind nicht committet (`.gitignore`) — für die Reproduzierbarkeit der A6-Zahlen relevant | hoch — Anhang C |
| `analyse/abb/abb1…abb7` (PNG 300 dpi + PDF) | 09.08.2026 | Abbildungen der Messfahrt-Auswertung | **final** | vektoriell für den Word-Satz vorhanden | **sehr hoch** — Kap. 9.3 |

---

## D. Thesis-Planungsdokumente

| Dokument | Version/Stand | Inhalt | Aktuell? | Überholte Inhalte | Thesis-Relevanz |
|---|---|---|---|---|---|
| `claude/Thesis_Strategie_und_Gliederung.md` | 31.07.2026, Gliederung **freigegeben** | Grundsatzentscheidungen (IMRaD, DIN 1505-2, Umfang 50–90 S.), freigegebene 11-Kapitel-Gliederung, Zuordnung offener Punkte zu Kapiteln | **aktuell**, in einem Punkt überholt | **Kap. 7 heißt noch „Datenschnittstelle und Web-App"** — die Web-App ist zugunsten einer nativen iOS-App verworfen (Bible v0.11). Die Zuordnungstabelle nennt außerdem Punkte, die inzwischen geschlossen oder abgegrenzt sind | **sehr hoch** — legt die Kapitelnummern fest: **Firmware = Kap. 6**, Validierung = Kap. 9 |
| `claude/Literaturliste_und_Evidence_Map.md` | 06.08.2026 | Neun nummerierte Quellen, Rechts-/Normenverzeichnis, Evidence Map mit 17 Einträgen (Spalten: Aussage/Kennwert · Beleg · Kapitel · Status), acht offene `[QUELLE ERFORDERLICH]`-Posten | **teils überholt** | Der Eintrag „α = 0,98, Achse Y, `MOTION_BRAKE_SIGN` — Eigenanteil" beschreibt den durch Stufe 1 ersetzten Filter | hoch — Grundlage des Quellenverzeichnisses; **muss um Madgwick 2010 und Mahony 2008 ergänzt werden**, die im Feldtestbericht bereits zitiert sind |
| `claude/Thesis_Transfer_iOS_App.md` + `…_Dokumentenstatus.md` | 10.08.2026 | Analoges Übergabepaket für den iOS-Track | **aktuell** | — | hoch — Formatvorlage und Gegenstück dieses Dokuments; die Cross-System-Schnittstelle ist dort in Punkt 17 geführt |
| `claude/Projektanalyse_und_Struktur_Vorschlag.md` | 21.07.2026, Status „Entwurf" | Erstanalyse vor Anlage der Project Bible | **historisch** | Vollständig überholt (Arduino-Sketches, „BLE nicht begonnen") | niedrig — als Prozessbeleg für den Entwicklungsverlauf, nicht zitieren |
| `Beispiel-Vorlage_Abschlussarbeit.docx`, HSD-Formulare, KI-Richtlinie | Hochschulstand | Formvorgaben, eidesstattliche Versicherungen, KI-Checkliste | **final** | — | hoch — Formalien, Anhang E |

---

## E. Quelltext als Dokument (Firmware, Commit `835c7b3`)

| Artefakt | Umfang | Status | Thesis-Relevanz |
|---|---|---|---|
| `firmware/lib/logic/` | 13 Module, hardwarefrei | **final** | **sehr hoch** — Kap. 6.1 (Modultrennung), 6.2 (FSM), 6.3 (Bremserkennung) |
| `firmware/lib/drivers/` | 7 Treiber | **final** | hoch — Kap. 6.1, 6.5 |
| `firmware/src/main.cpp` | Scheduler + Tasks + `BENCH_MODE`-Harness | **final** | **sehr hoch** — Kap. 6.1 (Ausführungsmodell), 6.6 (Testkonzept) |
| `firmware/test/` | 15 Testdateien, **126 Tests** | **final** | **sehr hoch** — Kap. 6.6 |
| `firmware/platformio.ini` | 3 Environments, Versionen gepinnt | **final** | hoch — Kap. 6.1, Reproduzierbarkeit |

---

## F. Nicht firmware-relevant

`claude/app_bible.md` (v0.23 · 10.08.2026, aktuell) · `App_Bible_v0.21.md` (**überholt**, Schnappschuss vor der v3-Migration) · `claude/CSV_Format_v3_Validierungsexport.md` (aktuell, V3-Labels vollzogen) · `docs/Umsetzungsplan_Schema_v3_iOS.md` (historisch, abgearbeitet) · `BMP280.cpp` (**historisch/Fremdquelle** — eine Arduino-Bibliothek von 2014 fremder Autorschaft aus der frühen Arduino-IDE-Phase, **kein Projektcode**; die Firmware nutzt die Adafruit-Kette. Für die Thesis nicht verwertbar, Verwechslungsgefahr mit dem produktiven Treiber) · `Uebersicht.xlsx` (BOM, korrekturbedürftig, s. Bible Kap. 11.2)
→ App-, Hardware- oder Organisationsbezug; für das **Firmware-Kapitel** nachrangig.

---

## Kritische Befunde (vor der Thesis-Konsolidierung zu klären)

**Befund 1 — Firmware-Hash `d8a4e75` in `measurement_log.md` ist nicht belegbar [widersprüchlich, mittlere Priorität].** Das Messprotokoll führt den Hash als gesicherte Angabe und beruft sich zur Bestätigung auf die `# META`-Zeile jeder CSV. Das ist zirkulär: Laut `bench_run_notes.md` wurde der Wert per Build-Flag `-D FIRMWARE_GIT_HASH=\"d8a4e75\"` **von Hand injiziert**. Er steht in der Datei, weil er hineingeschrieben wurde, nicht weil er aus dem Git-Stand abgeleitet ist. In keinem Dokument existiert eine Verknüpfung zu einem realen Commit. **Maßnahme:** als „manuell gesetzter Build-Marker, nicht automatisch aus Git ermittelt" kennzeichnen, oder den tatsächlichen Commit nachträglich bestimmen.

**Befund 2 — Signalpfad-Aussage in `measurement_log.md` ist zu weit gefasst [widersprüchlich, hohe Priorität].** Kap. 2 behauptet, das Verzögerungsprofil werde „in denselben Signalpfad gespeist, der im Normalbetrieb die Bremslicht-Duty erzeugt (`motion_filter` → `brake_curve` → `tail_light_fsm`)" und spiegele „das reale, integrierte FSM-Verhalten". Beides ist für die Experimente A/B/C nachweislich falsch: `BENCH_MODE` umgeht `motion_filter` und `lifecycle_fsm` und ersetzt `setup()`/`loop()` durch eine Busy-Wait-Schleife. Genau diese Lücke war der Anlass des Feldtests vom 06.08.2026. **Maßnahme:** Formulierung auf `brake_curve` → `tail_light_fsm` einschränken, `motion_filter` streichen, den Harness-Vorbehalt ergänzen. Diese Korrektur ist notwendig, bevor das Protokoll in Anhang C übernommen wird — in der jetzigen Form würde es eine Validierungstiefe behaupten, die nicht erreicht wurde.

**Nicht-Befund (Korrektur einer früheren Aussage in `open_issues.md`).** Dort war vermerkt, `measurement_log.md` enthalte eine falsche Behauptung über eine „15-prozentige Ratenverzerrung der Gyro-Integration". Eine Volltextsuche über alle vier Validierungsdokumente findet weder die Zahl noch die Begriffe. Die Aussage existiert in diesem Dokumentenstand nicht; der Punkt ist gegenstandslos und in der aktualisierten `open_issues.md` entsprechend nicht mehr geführt.

**Befund 3 — `claude/Feldtest_2026-08-06_Auswertung.md` trug r = −0,132 als belastbaren Hauptbefund [überholt, behoben].** Die methodische Korrektur (fehlende Latenzkorrektur der GNSS-Referenzkette) stand bisher nur in der Project Bible, nicht im Bericht selbst. Ein Leser des Berichts hätte die Zahl als Kennzahl übernommen. **Maßnahme ausgeführt:** Nachtrag N.1 ergänzt, der die Zahl zurücknimmt und offenlegt, worauf die Falsifikation stattdessen ruht.

**Befund 4 — Golden-Vektor ohne Provenienz [Lücke].** `testdata/frame_v3_golden.md` nennt den Firmware-Git-Hash nicht, gegen den kreuzvalidiert wurde (`1178017`). Zusätzlich stehen zwei Feldzahlen im Raum (41 gegenüber 43). Der Golden-Vektor ist der einzige geräteunabhängige Schnittstellennachweis der Arbeit; ohne Provenienzangabe ist er nicht reproduzierbar und damit angreifbar. **Maßnahme:** Hash eintragen, Zählweise festlegen und in Firmware- und App-Dokumentation vereinheitlichen.

**Befund 5 — `CLAUDE.md` widerspricht dem eingefrorenen v3-Vertrag [überholt].** Die Regelbasis für Claude Code in Xcode schreibt noch den v2-Vertrag fest (81 Byte, `version == 2`). Für die Thesis irrelevant, für die nächste App-Scheibe aber ein aktives Fehlerrisiko. **Maßnahme:** vor der nächsten App-Arbeit nachziehen.

**Befund 6 — Kap. 7 der freigegebenen Gliederung heißt noch „Web-App" [überholt].** Die Web-App ist seit Bible v0.11 zugunsten einer nativen iOS-App verworfen. Die Kapitelnummer 7 bleibt, der Titel und die Unterkapitel 7.2/7.3 sind neu zu fassen. Betrifft die Firmware nur mittelbar (7.1 BLE-Telemetrie-Schnittstelle bleibt).

---

## In dieser Session ausgeführte Aktualisierungen

| Dokument | Änderung | Beleg |
|---|---|---|
| `project_bible.md` | v0.18 → **v0.19**: Kap. 0.3/0.4, 2.11, 4.3, 6.1, 6.2, 6.4a, 6.5, 6.8, 6.9, 9 (Statustabelle), 9.5.4, 9.5.5, 10, 11, 12 (neu gegliedert in 12.1/12.2) | Commit `835c7b3`, Testergebnis 126/126, Binärgrößen, Quelltextprüfung |
| `decision_log.md` | sechs neue Einträge (Einbaulage an der Treibergrenze, M-01-Behebung, Umfangsschnitt FR-CFG, Kennzeichnung nicht verifizierter Parameter, Versionspinning, Rücknahme der B7-Zuordnung) | Freigabe des Verfassers im Chat |
| `open_issues.md` | neu strukturiert nach Firmware / Hardware / Dokumentation; Firmware-Teil auf „keine offenen Punkte" | Quelltextprüfung Commit `835c7b3` |
| `current_context.md` | vollständig neu geschrieben auf den Abschlussstand | Rückmeldung Claude Code + Flash-Bestätigung |
| `lessons_learned.md` | drei neue Einträge (Testlücke bei Hysterese, Beobachtereffekt der Debug-Ausgaben, Einbaulage an der Abstraktionsgrenze) | Befunde B5/B7, Quelltext |
| `roadmap.md` | M6 auf „abgegrenzt", M7 weitgehend abgehakt, Nachträge zu M1 und M3 | Bible v0.19 Kap. 12.2 |
| `Messfahrt_2026-08-08_Auswertung.md` | Nachtrag N.1–N.3 angehängt, Befundtabelle B5/B7 angepasst; Bericht selbst unverändert | Commit `835c7b3` |
| `Feldtest_2026-08-06_Auswertung.md` | Nachtrag N.1–N.3 angehängt; Bericht selbst unverändert | Bible v0.17 Kap. 9.4, Messfahrt 08.08. |

**Bewusst nicht geändert:** Die beiden Validierungsberichte in ihrem Kern (historische Beweisgrundlage), `measurement_log.md` (Korrektur erfordert eine Entscheidung des Verfassers, s. Befund 1 und 2), `CLAUDE.md` und `README.md` (App-Werkzeugdokumente, kein Thesis-Inhalt), `Projektanalyse_und_Struktur_Vorschlag.md` (bewusst als Momentaufnahme erhalten).
