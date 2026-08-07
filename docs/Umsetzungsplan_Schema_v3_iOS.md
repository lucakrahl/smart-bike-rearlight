# Umsetzungsplan — Telemetrie-Frame Schema v3 (iOS-App)

**Smart Bike Rear Light · Bachelorarbeit Krahl**
**Für: Claude in Xcode · Stand 07.08.2026**
**Quellen (verbindlich):** `claude/BLE_Frame_v3_Schnittstelle.md` (Schnittstellenvertrag), App Bible `claude/app_bible.md` Kap. 9 (Architektur), 10 (BLE-Vertrag), 13 (offene Punkte).

> Arbeitsweise: Die Pakete werden **streng nacheinander** abgearbeitet. Jedes Paket muss **grün** sein (kompiliert + alle genannten Tests bestehen), bevor das nächste beginnt. Nach jedem grünen Paket ein Git-Commit mit der AP-Nummer im Betreff.

---

## Nicht verhandelbare Randbedingungen (Leitplanken für jedes Paket)

- **`SmartBikeCore` bleibt UI-frei:** kein SwiftUI, kein CoreBluetooth, kein SwiftData im Package. Nur Foundation-Werttypen/reine Logik.
- **Reine Datensenke, unidirektional:** kein Write zum Gerät (FR-SYS-04).
- **Keine modalen Alerts während der Fahrt** (AR-UX-01). Diagnosegrößen dürfen das Cockpit nicht überladen und kein Scrollen erzwingen.
- **Alles lokal, kein Netzwerk.**

---

## Paketübersicht & Abhängigkeiten

| AP | Titel | Ebene (Kap. 9.2) | Hängt ab von |
|---|---|---|---|
| AP0 | Baseline sichern | — | — |
| AP1 | `TelemetryFrame` um v3-Felder erweitern | Core (Schicht 2/Modelle) | AP0 |
| AP2 | Decoder v3 + Mindestversions-/Längenregel | Core (Schicht 2) | AP1 |
| AP2b | Kreuztest gegen Firmware-Golden-Vektor | Core (Schicht 2, Test) | AP2 |
| AP3 | `MockTelemetrySource` auf v3 | Services/BLE | AP2b |
| AP4 | `TelemetryStore`: Live-Wahrheit vs. Persistenz | App (Schicht 3) | AP2 |
| AP5 | Persistenzmodell + Schema-Migration | Services/Persistence | AP1 |
| AP6 | Aufzeichnungsrate 10 Hz (E2) | Services/Live (Schicht 4) | AP5 |
| AP7 | CSV-Export 35 Spalten (E4) | Core (Schicht 6) | AP5 |
| AP8 | Diagnoseansicht unter Settings | App (Schicht 8/9) | AP4 |

Reihenfolge-Empfehlung: **AP0 → AP1 → AP2 → AP2b → AP3 → AP4 → AP5 → AP6 → AP7 → AP8.**
Nach AP3 läuft die App im Simulator bereits sichtbar auf v3 (Decode-Pfad end-to-end), bevor Persistenz/Export angefasst werden — bewusst früher Integrationspunkt.

> **Freigabe-Nachtrag (07.08.2026):** Plan freigegeben mit vier Ergänzungen E-1…E-4, hier bereits eingearbeitet: E-1 (kurzes v3-Frame → eigener Zähler `truncatedV3FrameCount`, AP2/AP8), E-2 (gemischte Versionen → CSV-Präambel Minimum + `frame_version_gemischt`, AP7), E-3 (Kreuztest gegen Firmware-Golden-Vektor als eigenes AP2b), E-4 (`t_s` bei 10 Hz ≥ 2 Nachkommastellen, AP6/AP7).

---

## AP0 — Baseline sichern

**Ziel:** definierter grüner Ausgangszustand, um Regressionen eindeutig zuzuordnen.

**Zieldateien:** keine (nur Git).

**Vorgehen:** aktuellen `main`-Stand bauen; `swift test` in `SmartBikeCore` + App-Tests laufen lassen; Testanzahl notieren; Git-Tag `pre-v3` setzen.

**Akzeptanzkriterien:** alle bestehenden Tests grün; Tag gesetzt.

**Tests:** bestehende Suite (Referenzlauf).

---

## AP1 — `TelemetryFrame` um die v3-Felder erweitern

**Ziel:** reiner Werttyp trägt die 13 neuen Felder — **ohne Verhaltensänderung**, reiner Compile-Gate.

**Zieldateien:** `SmartBikeCore/Sources/.../Telemetry/TelemetryFrame.swift`.

**Vorgehen & Entscheidung:**
- Die 13 neuen Felder werden als **Optionals** ergänzt (`gnss_accel_ms2: Float?`, …, `loop_max_us: UInt16?`). Begründung: Ein v2-Frame an einer v3-App (oder ein v3-Frame kürzer als 113 Byte) besitzt diese Felder nicht. `nil` bedeutet ausdrücklich „im Frame nicht vorhanden" und ist ehrlicher als ein Sentinel-Wert; es ist die Datentyp-Seite der Vorwärtskompatibilität (AR-CONN-04). Die v2-Felder (Offsets 0–80) bleiben nicht-optional.
- `frame_version` (bereits vorhanden) bleibt die getragene Geräte-Vertragsversion.
- Felder exakt in Vertrags-Benennung (Kap. 3.2) übernehmen, gleiche Typen (Float / UInt8 / UInt16).

**Akzeptanzkriterien:** Package kompiliert; `TelemetryFrame` lässt sich mit und ohne v3-Felder konstruieren; bestehende Tests unverändert grün.

**Tests:** ein Konstruktor-/Default-Test (v3-Felder `nil` konstruierbar); Round-Trip folgt in AP2.

---

## AP2 — Decoder v3 + Mindestversions-/Längenregel

**Ziel:** `Data → TelemetryFrame?` liest v2- und v3-Frames korrekt und wendet die neue Versionsregel an. **Kernstück der Migration.**

**Zieldateien:** `SmartBikeCore/Sources/.../Telemetry/TelemetryFrameDecoder.swift`; Testdatei `.../TelemetryFrameDecoderTests.swift`; optional reiner Test-Encoder `TelemetryFrameEncoding.swift` (nur im Test-Target) für Round-Trips.

**Vorgehen & Entscheidungen:**
- Zugriff **zwingend** über `withUnsafeBytes` + `loadUnaligned(fromByteOffset:as:)` für **jedes** Feld. Kein Pointer-Cast. Mehrere v3-Offsets sind nicht typ-ausgerichtet (81/85/89/93/97/101 als `Float` auf ungerader 4er-Ausrichtung; `loop_max_us` als `UInt16` auf Offset 111). Little-Endian.
- **Entscheidungslogik (genau in dieser Reihenfolge):**
  1. `length < 81` → **verwerfen**, Fehlerzähler +1, `return nil`.
  2. `version < 2` → **verwerfen**, Fehlerzähler +1, `return nil` (deckt version 1 ab).
  3. sonst v2-Felder (Offsets 0–80) lesen.
  4. `version >= 3 && length >= 113` → zusätzlich v3-Felder (Offsets 81–112) lesen; sonst v3-Felder `nil`.
  5. **Kurzes v3-Frame (E-1):** `version >= 3 && 81 <= length < 113` → Frame **lesen** (v2-Ebene, v3-Felder `nil`), **nicht** verwerfen, aber einen **eigenen** Zähler `truncatedV3FrameCount` +1 (getrennt vom allgemeinen Fehlerzähler). Begründung: Ein Gerät, das v3 meldet und zu kurz sendet, ist defekt (abgeschnittene Notification / fehlgeschlagene MTU-Aushandlung). Verwerfen würde gültige v2-Nutzdaten einer Messfahrt vernichten; stilles Durchwinken würde den Defekt verstecken, bis in der Auswertung die Diagnosefelder fehlen. Der Zähler wird in AP8 angezeigt.
  6. Bytes jenseits der erwarteten Länge **ignorieren** (nicht verwerfen) → Vorwärtskompatibilität.
- Die Regel ersetzt die bisherige Gleichheitsprüfung `version == 2` (App Bible Kap. 10). **Wichtig für die Thesis:** Das ist die Voraussetzung, dass Firmware und App in getrennten Toolchains unabhängig weiterlaufen (v2-Gerät ↔ v3-App und umgekehrt).

**Akzeptanzkriterien:** Alle Felder an den korrekten Offsets; Versionsregel exakt wie oben; allgemeiner Fehlerzähler steigt bei `length < 81` und `version < 2`; `truncatedV3FrameCount` steigt **nur** bei kurzem v3-Frame (und **nicht** der allgemeine Fehlerzähler); kein Crash bei unausgerichteten Offsets.

**Tests (Host, `SmartBikeCore`):**
- **Round-Trip:** bekannte `TelemetryFrame` → Test-Encoder → Decoder → Gleichheit aller Felder.
- **Offset-Prüfung je neuem Feld:** einzeln gesetztes Byte-Muster ergibt exakt den erwarteten Wert (13 Felder).
- **Versionsregel-Grenzfälle (Matrix):** Länge **80 / 81 / 112 / 113 / 200** × `version` **1 / 2 / 3 / 4**. Erwartungen u. a.: 80→nil+Fehler; 81/v2→v2-Felder, v3=nil; 113/v3→v3 gelesen; 200/v4→v3 gelesen, Rest ignoriert; jede `version 1`→nil+Fehler.
- **Kurzes v3-Frame (E-1):** Länge **112** mit `version 3` → liefert ein **gültiges** Frame (v2-Felder gesetzt, v3-Felder `nil`), erhöht `truncatedV3FrameCount` um 1, **nicht** den allgemeinen Fehlerzähler.
- **Nicht-ausgerichtete Felder:** gezielter Test für `gnss_accel_ms2` (Off 81) und `loop_max_us` (Off 111).

---

## AP2b — Kreuztest gegen Firmware-Golden-Vektor  *(E-3, neu — der einzige Test, der die Schnittstelle selbst validiert)*

**Ziel:** Beweisen, dass App-Decoder und **Firmware**-Encoder dieselbe Bytebelegung meinen — nicht nur, dass der App-Decoder zu seinem eigenen Encoder passt. Ein gemeinsamer Denkfehler in beiden Encodern bliebe sonst in beiden Suiten grün und würde erst in den Messfahrt-Daten sichtbar.

**Zieldateien:** Testdatei `SmartBikeCore/Tests/.../FrameGoldenVectorTests.swift`; Eingangsdateien aus dem Firmware-Teil des Repos: `testdata/frame_v3_golden.hex` (113 Byte als Hex) und `testdata/frame_v3_golden.md` (Wertetabelle je Feld).

**Vorgehen & Entscheidungen:**
- Der Test liest **exakt die Bytefolge** aus `frame_v3_golden.hex` ein (Hex → `Data`), dekodiert sie mit dem **Produktions-Decoder** aus AP2 und prüft **jedes Feld** gegen die Sollwerte aus `frame_v3_golden.md`.
- **Dieser Test darf nicht über den eigenen (Test-)Encoder laufen.** Der Golden-Vektor ist die einzige geräteunabhängige Quelle; würde er aus dem App-Encoder erzeugt, prüfte er wieder nur die eigene Symmetrie.
- Float-Vergleiche mit kleiner Toleranz (die Wertetabelle nennt gerundete Werte); Ganzzahlfelder exakt.
- Fehlt eine der beiden Eingangsdateien (Firmware-Stand noch nicht eingecheckt), **schlägt der Test bewusst fehl** (kein stilles Überspringen) — so bleibt sichtbar, dass die Schnittstelle noch nicht kreuzvalidiert ist.

**Akzeptanzkriterien:** Der Test dekodiert den Golden-Vektor und bestätigt jedes Feld gegen die Firmware-Wertetabelle; er ist unabhängig vom App-Encoder; fehlende Golden-Dateien führen zu einem klaren Testfehler.

**Tests:** genau dieser eine Golden-Vektor-Test (er *ist* das Arbeitspaket).

> Abstimmung nötig: Die Firmware muss `testdata/frame_v3_golden.hex` + `.md` bereitstellen. Solange sie fehlen, bleibt AP2b rot — das ist gewollt und markiert die offene Kreuzvalidierung.

---

## AP3 — `MockTelemetrySource` auf v3

**Ziel:** hardwareunabhängige, plausible v3-Frames im Simulator; früher End-to-End-Sichtpunkt.

**Zieldateien:** `Services/BLE/MockTelemetrySource.swift` (nutzt nur Foundation `Data` — bleibt außerhalb Core; erlaubt).

**Vorgehen & Entscheidung:**
- 113-Byte-Frames mit `version=3` erzeugen; v2-Felder wie bisher plausibel, v3-Felder plausibel bespielen.
- Die geforderten **Sonderfälle** über die Zeit erzeugen: Phasen mit `gnss_accel_valid == 0` (dann `gnss_accel_ms2 = 0.0`) und mit `bias_calibrated == 0` (z. B. die ersten Sekunden nach „Boot"), danach beide auf 1.
- Regime-Zähler so bespielen, dass ihre Summe ~10 ist und bei Bremsphasen `regime_shock_n`/`regime_dynamic_n` steigt; `norm_delta_*`, `jerk_max` konsistent dazu.
- Frame-Bytebau über denselben reinen Encoder-Helfer wie im AP2-Test (eine Quelle für die Byte-Anordnung → keine Divergenz).

**Akzeptanzkriterien:** Simulator empfängt v3-Frames; Decoder liefert vollständige Frames; über einen Lauf treten beide Sonderfälle nachweislich auf; bestehende Simulator-Flows unverändert.

**Tests:** Encoder↔Decoder-Round-Trip des Mock-Bauwegs (Core-Test); optional ein kurzer Stream-Test, dass die Sonderfälle auftreten.

---

## AP4 — `TelemetryStore`: Live-Wahrheit vs. Persistenz  *(hier wolltest du meine Einordnung)*

**Ziel:** klar festlegen, welche v3-Größen als **Live-Wahrheit** promotet werden und welche reine Analysedaten sind.

**Zieldateien:** `Services/Live/TelemetryStore.swift` (+ ggf. `SystemWarnings`-Ableitung in Core, falls du eine Statuszeile speist).

**Meine Einordnung (ich stimme deinem Ausgangspunkt zu, mit einer Präzisierung):**

- **Live promoten (abgeleiteter Status):** **`bias_calibrated`** und **`gnss_accel_valid`**. Begründung: Beides sind binäre **Qualitäts-/Statusgrößen** mit sofortiger Aussage. `bias_calibrated == 0` bedeutet, dass das System im konservativen Rückfall läuft (τ_slow 30 s statt 90 s) — man will im Feld sehen, ob die Kalibrierung greift. `gnss_accel_valid` sagt, ob die Referenz gerade überhaupt gilt. Beide gehören zur „Innensicht Gesundheit", die die App bereits für IMU-Health führt (Kap. 9.4).
- **Nur Persistenz (Analyse):** die übrigen **11** Felder — `gnss_accel_ms2`, `pitch_rad`, `gyro_bias_rads`, `norm_delta_min`, `norm_delta_max`, `jerk_max`, `regime_static_n`, `regime_dynamic_n`, `regime_shock_n`, `dt_max_ms`, `loop_max_us`. Es sind **Fensteraggregate bzw. Analysegrößen**, die erst in der Nachauswertung (Korrelation Ist/Soll, Filter-Parametrierung) Sinn ergeben. `gnss_accel_ms2` ist bewusst **nicht** live: als Momentanzahl ohne Gegenüberstellung zur IMU-Verzögerung nicht deutbar, und nur bei gültiger Referenz überhaupt belastbar.

**Präzisierung zu deinem Ausgangspunkt (Widerspruch im Detail, nicht im Prinzip):**
- Trenne sauber zwischen **„Store hält den letzten Frame"** (das tut er ohnehin — damit sind *alle* Felder technisch live über `lastFrame` erreichbar) und **„als Live-Property/Warnung promotet"**. Nur `bias_calibrated`/`gnss_accel_valid` werden zu abgeleitetem Zustand; alles andere bleibt im Frame liegen und wird nur vom Persistenz-/Analysepfad konsumiert. Das hält den Store schlank und die Semantik ehrlich.
- **`dt_max_ms`/`loop_max_us`** sind grenzwertig (Performance-Gesundheit). Ich würde sie trotzdem **nicht** als Live-Wahrheit promoten, sondern in der Diagnoseansicht (AP8) direkt aus `lastFrame` lesen. Grund: Es sind Fenster-Sättigungswerte für die spätere NFR-RT-04-Auswertung, kein Fahr-relevanter Live-Zustand.
- **Keine Cockpit-Warn-Chips** aus `bias_calibrated==0`/`gnss_accel_valid==0`: kurz nach Boot ist `bias_calibrated==0` normal (Kalibrierung läuft), ein Chip dafür wäre Fehlalarm und verletzt die Ablenkungsarmut (AR-UX-01). Diese beiden Status gehören in die Diagnoseansicht (AP8), nicht in die Fahr-Statuszeile.

**Akzeptanzkriterien:** Store exponiert genau zwei neue abgeleitete Status (`bias_calibrated`, `gnss_accel_valid`); die 11 Analysefelder werden **nicht** in Live-Properties gespiegelt; keine neue Cockpit-Anzeige; keine Warn-Chips aus den neuen Feldern.

**Tests:** Store-Update-Test (letzter Frame setzt die zwei Status korrekt, inkl. der Sonderfälle 0/1); Negativtest, dass Analysefelder nicht als Live-Property auftauchen (API-Form).

---

## AP5 — Persistenzmodell + Schema-Migration  *(eigenes Paket, wie gewünscht)*

**Ziel:** `TrackSample` trägt die neuen Felder; **bestehende Fahrten bleiben lesbar**.

**Zieldateien:** `Services/Persistence/…` (`TrackSample`-`@Model`, ggf. `SchemaV3` + `MigrationPlan`), Mapping Core-Werttyp ↔ `@Model`.

**Vorgehen & Entscheidungen:**
- Die 13 Felder als **optionale** Properties additiv ergänzen (analog AP1).
- **`temperature_c` bleibt im Persistenzmodell** und wird **nur aus dem CSV** entfernt (AP7). Begründung: Das Feld ist weiterhin im Frame (Offset 38); es aus dem Modell zu löschen wäre eine **destruktive** Migration mit Datenverlust ohne Nutzen. E4 betrifft ausdrücklich nur den Export.
- **Migrationsart:** rein additive, optionale Properties ⇒ **leichtgewichtige (automatische) SwiftData-Migration**, keine benutzerdefinierte Datenumformung nötig. Falls im Projekt bereits ein `VersionedSchema`/`SchemaMigrationPlan` existiert: eine `SchemaV3`-Stufe als *lightweight* ergänzen. Falls kein versioniertes Schema existiert: additive Optionals werden automatisch migriert — dann keine Plan-Datei nötig, aber der Migrationsnachweis (unten) ist Pflicht.
- **`frame_version`** je Sample weiter schreiben (Quelle für die CSV-Präambel in AP7).

**Auswirkung auf Schreiblast/Migration (benannt, wie gefordert):**
- Bei 1 Hz ist die zusätzliche Feldzahl vernachlässigbar (13 kleine optionale Werte je Sample).
- Der eigentliche Schreiblast-Sprung kommt erst mit **AP6 (10 Hz)** — dort behandelt.
- Migrationsrisiko: additive Optionals sind unkritisch, **solange** nichts entfernt/umbenannt und kein Feld nicht-optional wird. Genau das macht dieses Paket zum sensibelsten für Altdaten (siehe Abschnitt „Altdaten-Risiko" am Ende).

**Akzeptanzkriterien:** Ein **vor** der Änderung angelegter Store öffnet nach der Änderung fehlerfrei; alte Fahrten laden vollständig; neue Felder dort `nil`; neue Fahrten schreiben die Felder.

**Tests:** Migrations-/Öffnungstest gegen einen vorbefüllten Alt-Store (oder Fixture); Mapping-Test Core-Werttyp ↔ `@Model` (Hin- und Rückrichtung, inkl. `nil`-Felder).

---

## AP6 — Aufzeichnungsrate 10 Hz (Entscheidung E2)

**Ziel:** umschaltbarer Validierungsmodus **10 Hz (ohne Verdichtung)**; Voreinstellung bleibt **1 Hz**.

**Zieldateien:** `Services/Live/RideManager.swift`, `Settings`-Modell + `Features/Settings/…` (Umschalter), Persistenz-Schreibpfad (Batching).

**Vorgehen & Entscheidungen:**
- Aufzeichnungsmodus als Enum (`.compact1Hz` Default / `.validation10Hz`). Umschaltung **nur außerhalb einer laufenden Aufzeichnung** (konsistente Rate je Fahrt).
- **Schreiblast (benannt):** 10 Hz × ~35 Felder ⇒ ~4 kB/s, ~14 MB/h CSV (Vertrag Kap. 8). Für die DB heißt das 10 Inserts/s statt 1. **Kein** `context.save()` je Sample. Stattdessen im Hintergrund-`ModelActor` **batchen** (z. B. Puffer von ~10 Samples ⇒ ein Save/s), damit AR-NFR-PERF-01 (ruckelfrei, Main-Thread nie blockiert) hält.
- `RecordingClock` und `StatisticsEngine` bleiben unverändert korrekt: dt wird ohnehin aus echten Zeitdifferenzen gerechnet (dt-Kappung 1,5 s), Distanz/Ø/„bewegt" sind ratenunabhängig.
- Voreinstellung/Persistenz des Modus in `Settings`; UI-Hinweis (nicht-modal) auf höheren Speicherbedarf.

**Akzeptanzkriterien:** Default 1 Hz unverändert; 10 Hz-Modus erzeugt ~10 Samples/s; Schreibvorgänge gebündelt (nicht 1 Save/Sample); UI bleibt flüssig; Umschalten während Aufzeichnung gesperrt; der je Sample gespeicherte Zeitstempel hat genug Auflösung, dass bei 10 Hz **keine doppelten `t_s`-Werte** entstehen (Formatierung in AP7: ≥ 2 Nachkommastellen, E-4).

**Tests:** `RideManager` gegen Mock-`RideRepository`: Sample-Rate/-Anzahl je Modus; Nachweis der Batch-Schreibaufrufe (Anzahl Saves ≪ Anzahl Samples); Sperre der Umschaltung während Aufzeichnung.

---

## AP7 — CSV-Export 35 Spalten (Entscheidung E4)

**Ziel:** Export von 23 auf **35** Spalten, extern stabil und dokumentiert.

**Zieldateien:** `SmartBikeCore/Sources/.../Export/RideCSVExporter.swift`; Testdatei; **CSV-Spaltendoku** (Kopf-Kommentar im Exporter *und* ein Golden-Header-Test als verbindliche Quelle).

**Vorgehen & Entscheidungen:**
- **Entfällt:** `temperature_c`. **Neu:** die 13 v3-Felder.
- **Spaltenreihenfolge (stabil, dokumentiert):** bestehende Reihenfolge **unverändert lassen, nur `temperature_c` herausnehmen**, danach die **13 neuen in Vertrags-Offset-Reihenfolge** anhängen (81→111): `gnss_accel_ms2, pitch_rad, gyro_bias_rads, norm_delta_min, norm_delta_max, jerk_max, regime_static_n, regime_dynamic_n, regime_shock_n, bias_calibrated, gnss_accel_valid, dt_max_ms, loop_max_us`. Der **exakte Header wird per Golden-Test eingefroren** — er ist die maßgebliche Doku für die externe Auswertung.
- **Präambel `schema_version` = Frame-Version des Geräts**, gelesen aus dem persistierten `frame_version` der Fahrt — **nicht** die App-Version. So ist erkennbar, gegen welchen Firmware-Vertrag eine Aufzeichnung entstand.
- **Gemischte Versionen in einer Fahrt (E-2):** `schema_version` = **Minimum** der vorkommenden `frame_version` (nicht Maximum — der schwächste Fall bestimmt, welche Spalten durchgängig gefüllt sind; das Maximum würde Spalten versprechen, die nicht durchgehend belegt sind). Zusätzlich eine Präambelzeile `# frame_version_gemischt;ja` (bei einheitlicher Version `;nein`).
- **`t_s`-Auflösung (E-4):** die Zeitspalte `t_s` führt **mindestens 2 Nachkommastellen**. Mit nur einer Stelle entstehen bei 10 Hz und Taktschwankung doppelte Zeitstempel, was jede zeitbasierte externe Auswertung unbrauchbar macht.
- Format unverändert: Trennzeichen `;`, deutsches Dezimalkomma, CRLF, UTF-8-BOM + `sep=;`. Leere Optionalfelder → leere Zelle (v2-Fahrt ⇒ v3-Spalten leer). Ausgabe via Share-Sheet.

**Akzeptanzkriterien:** exakt 35 Datenspalten; eingefrorene Reihenfolge; `temperature_c` fehlt; Präambel trägt Geräteversion (bei gemischten Versionen das Minimum) + `frame_version_gemischt`-Zeile; `t_s` ≥ 2 Nachkommastellen; Dezimalkomma/CRLF/`;`/BOM korrekt; v2-Fahrt exportiert v3-Spalten leer.

**Tests (Host):** Spaltenzahl = 35; **Golden-Header** (exakter String, inkl. der Präambel-Erweiterung E-2); Dezimalkomma-Formatierung (z. B. `1,25`); `t_s` mit ≥ 2 Nachkommastellen; Präambel-`schema_version` = Geräteversion (Test mit App-Version ≠ 3 stellt sicher, dass nicht die App-Version erscheint); **gemischte Versionen** → Minimum + `frame_version_gemischt;ja`; einheitliche Version → `;nein`; `temperature_c` nicht vorhanden; v2-Sample ⇒ leere v3-Zellen.

---

## AP8 — Diagnoseansicht unter Settings  *(hier wolltest du meine Einordnung)*

**Ziel:** minimale, zurückhaltende Sichtbarkeit der Live-Status — **nicht** im Cockpit.

**Zieldateien:** `Features/Settings/DiagnosticsView.swift` (+ kleines ViewModel), Verlinkung in `SettingsView`.

**Meine Einordnung — ich stimme dir zu und schärfe nach:**
- **Ins Cockpit: nichts** von den Diagnosegrößen. Das Fahr-Cockpit bleibt bei den Fahr-Kennzahlen (AR-UX-01, kein Scrollen). Richtig so.
- **In die Diagnoseansicht (Settings): `bias_calibrated` und `gnss_accel_valid` als Statuszeile** (grün/amber, nicht-modal). Ergänzend würde ich **`dt_max_ms`/`loop_max_us`** als reine Zahlen aufnehmen (Performance-Beleg für NFR-RT-04) — direkt aus `lastFrame`, kein Store-Promote. Optional die Regime-Zähler als Live-Rohanzeige, weil sie beim Feld-Debugging der Filterschwellen helfen; das ist aber „nice to have".
- **`truncatedV3FrameCount` (E-1) anzeigen** — die Zahl der zu kurz empfangenen v3-Frames. > 0 ist ein Defekt-Indikator (abgeschnittene Notification / MTU-Problem) und gehört sichtbar in die Diagnose.
- Ansicht ist **read-only** (Datensenke), aktualisiert live aus dem Store; kein Alert, keine Interaktion Richtung Gerät.

**Akzeptanzkriterien:** Ansicht nur unter Settings erreichbar; zeigt die zwei Status live **und `truncatedV3FrameCount`**; Cockpit unverändert; kein Scrollzwang im Cockpit; keine modale Unterbrechung.

**Tests:** ViewModel-Host-Test der Ableitung (Status-Text/Farbe aus Frame-Werten, inkl. 0/1-Fälle). Reine View ohne Testpflicht.

---

## Migrations- & Schreiblast-Analyse (Zusammenfassung)

- **Modelländerung (AP5):** additiv + optional ⇒ leichtgewichtige, automatische Migration; kein Datenverlust, **sofern** nichts entfernt/umbenannt wird und `temperature_c` im Modell bleibt.
- **10 Hz (AP6):** 10× Insert-Rate; beherrschbar durch **Batch-Saves** im Hintergrund-`ModelActor`. Speicher: ~14 MB/h CSV, DB entsprechend — für 15–30-min-Messfahrten unkritisch, für Dauerbetrieb bewusst nicht Default.
- **RAM/Firmware-Seite** (Ringpuffer +19 kB, Vertrag Kap. 5) betrifft die Firmware, nicht die App — hier nur zur Kenntnis.

---

## Getroffene Annahmen (weil Vertrag/App Bible sie nicht abdecken)

1. **v3-Felder als Optionals** im `TelemetryFrame`/`TrackSample` (`nil` = nicht im Frame). Der Vertrag legt die App-seitige Repräsentation nicht fest.
2. **Kurzes v3-Frame** (`version==3`, Länge in [81,113)): **ENTSCHIEDEN (E-1)** — wird als gültiges v2-Level gelesen (v3-Felder `nil`), kein Verwerfen, aber Zählung in `truncatedV3FrameCount` (getrennt vom Fehlerzähler), Anzeige in AP8. Siehe AP2/AP8.
3. **`version < 2`** (inkl. version 1) mit Länge ≥ 81 → **verwerfen + Fehlerzähler**. Der Vertrag formuliert die Leseregeln nur positiv ab `>= 2`; der geforderte „version 1"-Grenzfalltest impliziert Ablehnung.
4. **`temperature_c` bleibt im Persistenzmodell**, entfällt nur im CSV. E4 nennt ausdrücklich nur den Export.
5. **CSV-`schema_version`** aus dem persistierten `frame_version` der Fahrt; **gemischte Versionen ENTSCHIEDEN (E-2)** — Präambel trägt das **Minimum** plus Zeile `# frame_version_gemischt;ja/nein`. Siehe AP7.
6. **`bias_calibrated==0` / `gnss_accel_valid==0` erzeugen keine Cockpit-Warn-Chips**, sondern erscheinen nur in der Diagnoseansicht — zur Wahrung von AR-UX-01 und gegen Fehlalarm kurz nach Boot.
7. **10 Hz-Schreibpfad wird gebündelt** (Batch-Saves); Vertrag/App Bible geben kein Schreibmuster vor, aber AR-NFR-PERF-01 verlangt es faktisch.
8. **CSV-Spaltenreihenfolge:** bestehende Reihenfolge minus `temperature_c`, dann 13 neue in Offset-Reihenfolge; per Golden-Test fixiert (die App Bible dokumentiert die exakte v2-Reihenfolge nicht auf Spaltenebene).
9. **Format-Details** (UTF-8-BOM, `sep=;`) bleiben wie in v2 erhalten (Vertrag: „Format unverändert").
10. **Diagnoseansicht** ist dauerhaft (nicht nur `#if DEBUG`) unter Settings verfügbar, read-only. Falls du sie DEBUG-gaten willst, ist das eine Einzeiler-Änderung.

---

## Welche Pakete können frühere Aufzeichnungen unbrauchbar machen?

- **AP5 (Persistenzmodell/Migration) — das einzige Paket mit echtem Altdaten-Risiko.** Nur *falsch* umgesetzt (nicht-optionales Feld, Entfernen/Umbenennen von Properties, `temperature_c` aus dem Modell löschen, inkompatible Store-Konfiguration) kann SwiftData daran hindern, den bestehenden Store zu öffnen ⇒ **alte Fahrten unlesbar**. *Richtig* umgesetzt (additiv + optional, `temperature_c` behalten) bleiben alle Altdaten erhalten. Deshalb ist der **Migrations-/Öffnungstest gegen einen Alt-Store** das zentrale Akzeptanzkriterium dieses Pakets.
- **AP7 (CSV) — betrifft nicht die gespeicherten Fahrten, sondern deren externe Auswertung.** Gespeicherte Fahrten bleiben intakt und re-exportierbar, aber der **Header ändert sich** (23→35 Spalten, `temperature_c` weg): extern gepflegte Auswertungen/Skripte, die auf das alte 23-Spalten-Format fixiert sind, **brechen** und müssen auf das neue, per `schema_version` unterscheidbare Format umgestellt werden. Re-Export einer alten (v2-)Fahrt liefert die v3-Spalten leer.
- **AP6 (10 Hz)** und alle übrigen Pakete berühren bestehende Aufzeichnungen **nicht**.

---

## Definition of Done (Gesamt)

Alle APs grün; `swift test` (Core) + App-Tests bestehen; Simulator zeigt v3 über den Mock; ein Alt-Store öffnet und exportiert korrekt; 1 Hz bleibt Default, 10 Hz umschaltbar; CSV trägt 35 Spalten mit Geräte-`schema_version`. **Dokumentation nachziehen** (freigabepflichtig): App Bible Kap. 10 auf v3, Kap. 13 offene Punkte, Decision Log (E2/E4) — separat, nicht Teil dieser Code-Pakete.