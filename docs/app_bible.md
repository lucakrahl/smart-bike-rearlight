# App Bible — iOS-App „Smart Bike Rear Light“
**Bachelorarbeit Krahl · Maschinenbau & Produktentwicklung (B.Eng.)**
**Version 0.23 · Stand 10.08.2026 · Status: aktiv gepflegt (Single Source of Truth der App)**

> Version 0.23 zieht die abgeschlossene **Frame-Schema-v3-Migration** (AP0–AP8) vollständig nach und **vereinheitlicht die Entscheidungs-Kürzel**: Die iOS-Nachträge heißen **V3-1…V3-4** (zuvor `E-1…E-4`, was mit den Feldtest-Entscheidungen E1–E5 kollidierte). Das Dokument ist **selbsttragend** und ersetzt v0.21/v0.22.
>
> Diese App Bible ist die oberste Wissensinstanz **der iOS-App**. Sie beschreibt ausschließlich die App. Die **Project Bible** bleibt die technische Wahrheit des Gesamtsystems (Firmware, Hardware, BLE-Vertrag); bei Widersprüchen zwischen Chat und App Bible gilt die App Bible, bei Widersprüchen zum Gesamtsystem die Project Bible. Der maßgebliche Schnittstellenvertrag für v3 ist `claude/BLE_Frame_v3_Schnittstelle.md`. Chats dienen der Diskussion; der offizielle App-Stand steht ausschließlich hier.

---

## 0. Meta & Regeln

### 0.1 Geltungsregeln
- Die App Bible bildet jederzeit den offiziellen Entwicklungsstand der App ab.
- Die App Bible wird **niemals ungefragt** geändert (Nutzer-Freigabe erforderlich).
- Ebenentrennung: **App Bible** = offizieller App-Stand · **Kap. 12 Decision Log** = Begründung · **Chats** = Diskussion.
- Firmware und BLE-Schnittstelle werden durch die App **nicht** verändert (FR-SYS-04).

### 0.2 Kennzeichnungslegende
- **[bestätigt]** — vom Nutzer freigegeben · **[Entwurf]** — vorgeschlagen · **[offen]** — in Klärung · **[aus Project Bible]** — Systemvorgabe, referenziert.

### 0.3 Änderungsprotokoll
| Version | Datum | Änderung | Anlass |
|---|---|---|---|
| 0.1–0.7 | 29.07.2026 | Phase 1 (Anforderungsanalyse) A–F + Personalisierung. | Phase 1 |
| 0.8 | 29.07.2026 | Phase 1 freigegeben; Phase 2 (IA) bestätigt; IA-/Flussdiagramm. | Freigabe Phase 1 + IA |
| 0.9 | 29.07.2026 | UX-A (Designsystem) bestätigt; Style-Tile. | Freigabe UX-A |
| 0.10 | 29.07.2026 | UX-B (Bedienkonzept) bestätigt; AR-UX-01…05. | Freigabe UX-B |
| 0.11 | 29.07.2026 | UX-C (Wireframes) + UX-D (Editor-Feinbild); Phase 3 abgeschlossen. | Freigabe UX-C/D |
| 0.12 | 29.07.2026 | Phase 4 (Softwarearchitektur) bestätigt; Architekturdiagramm. | Freigabe SA-A/B/C |
| 0.13 | 29.07.2026 | Phase 5 (Projektstruktur) bestätigt (Kap. 9.7). | Freigabe Phase 5 |
| 0.14 | 31.07.2026 | **Phase 5 freigegeben; Phase 6 (Implementierung in Xcode/Claude Code) läuft** (Umsetzungsstand Kap. 0.5). **Designentscheidung Liquid Glass** ergänzt (Kap. 8): Material nur für Chrome/Steuerelemente, Inhalte glasfrei. | Nutzerfreigabe Liquid-Glass-Regel |
| 0.15 | 01.08.2026 | **Recovery-UX (AR-DATA-04) umgesetzt & verifiziert** (Unit-Tests 3/3 grün + on-screen via Debug-Seed); Umsetzungsstand Kap. 0.5 (inkl. Test-/Validierungshilfen) und offene Punkte Kap. 13 nachgezogen. | Abschluss AR-DATA-04 |
| 0.16 | 01.08.2026 | Recovery-UX (AR-DATA-04) auf **drei Optionen** präzisiert (Abschließen / Verwerfen / **Weiter fahren** mit Zeitstempel-Kontinuität via `tOffset`). **Firmware-BLE-Brown-Out gelöst** (Board-Tausch, s. Project Bible v0.13): Blocker in Kap. 0.5/10/13 aufgelöst; Tausch `MockTelemetrySource` → echte `BLEConnectionService` ist damit entblockt und der nächste Umsetzungsschritt. | Faktenstand Firmware-BLE + Recovery-Detail |
| 0.17 | 01.08.2026 | **Future Work** ergänzt (Kap. 5): integrierte Fahrrad-Navigation im Cockpit (Routenwahl vor der Fahrt, Turn-by-turn, Live-Karte) evaluiert und bewusst nach V2 abgegrenzt — Begründung dokumentiert. | Evaluierung Navigations-Idee |
| 0.18 | 03.08.2026 | **Echte BLE-Verbindung am realen iPhone verifiziert** (Mock → `BLEConnectionService`); **Feldrobustheit** ergänzt: monotone `RecordingClock` (Zeit/Distanz gegen Geräte-Uhr-Reset), Stale-/GNSS-Fix-Validität (AR-UX-05; AR-LIVE-03 teilweise), **Höhe barometrisch statt GNSS**. Umsetzungsstand/Datenmodell (Kap. 0.5/11) + drei Decision-Log-Zeilen (Kap. 12) + offene Punkte (Kap. 13) nachgezogen. | Abschluss BLE-Anbindung + Feldrobustheit |
| 0.19 | 05.08.2026 | **BLE-Vertrag auf Firmware-Schema v2 nachgezogen (Kap. 10):** Frame 80→81 Byte, neues Feld `brake_light_pct` (Offset 80, uint8, kommandierte LED-Duty = **Ausgang** der Bremslogik) neben `brake_decel_ms2` (roher Verzögerungs-**Eingang**) — ermöglicht die empirische „Verzögerung ⇄ Lichtintensität“-Validierung. Schema-Version `1`→`2`, MTU-Mindestwert ≥ 84 Byte. | Firmware Schema v2 (Project Bible v0.14) |
| 0.20 | 05.08.2026 | **Schema v2 app-seitig genutzt:** Decoder auf 81 Byte / `version==2`, `brake_light_pct` dekodiert; **Bremslicht-Validierung** in der Fahrt-Detailansicht (Doppelachsen-Diagramm Verzögerung ⇄ Lichtintensität, Fail-Safe markiert) + **validierungsvollständiger CSV-Export** (23 Spalten inkl. System-/Health-/Baro-/GNSS-Feldern + Metadaten-Kopf; ohne rohe IMU-Achsen). Datenmodell (Kap. 11), Scope (Kap. 5) und offene Punkte (Kap. 13) nachgezogen. | Bremslicht-Evidenz + Validierungs-Export |
| 0.21 | 05.08.2026 | **System-/Sensorwarnungen (AR-LIVE-03) vollständig** (IMU-Health/degraded/Watchdog/Init/Baro → gestufte, nicht-modale Statuszeilen-Chips, antippbar) + **Ø/Fahrzeit = „bewegt"** (Bewegungsschwelle 1,0 km/h, nur bei gültigem Fix; Gesamtzeit intern als `totalDuration` behalten). Umsetzungsstand/§6.2/§9.4/§11 + Decision-Log-Zeile + §13 nachgezogen. | Abschluss AR-LIVE-03 + Ø/Fahrzeit-Entscheidung |
| **0.22** | **07.08.2026** | **Frame-Schema-v3-Migration abgeschlossen (AP0–AP8), 89 Tests grün, gegen Firmware-Golden-Vektor kreuzvalidiert.** Decoder auf v3 (**113 Byte**, 13 neue Diagnosefelder) mit **Mindestversions-/Längenregel** (`version==` → `≥`); kurzes-v3-Frame-Zähler `truncatedV3FrameCount` (V3-1); **Golden-Vektor-Kreuztest** gegen die Firmware (V3-3); Decoder **rein** (Ergebnis-Enum, Zähler im Store); gemeinsamer `TelemetryFrameEncoder` (eine Byte-Quelle Mock/Test); Persistenz um 13 v3-Felder **+ `temperature_c`** additiv erweitert (leichtgewichtige Migration); **umschaltbarer 10-Hz-Validierungsmodus** (E2, Feldtest) mit Batch-Persistenz; **CSV-Export 35 Spalten** (`temperature_c` raus [E4, Feldtest]; `t_s` ≥ 2 Nachkommastellen [V3-4]; Präambel-Minimum + `frame_version_gemischt` [V3-2]); read-only **Diagnoseansicht** unter Settings (V3-1-Zähler sichtbar). Kap. 0.5/5/6/7/9/10/11/12/13/14 nachgezogen. | Abschluss Schema v3 (Vertrag `claude/BLE_Frame_v3_Schnittstelle.md`) |
| **0.23** | **10.08.2026** | **Entscheidungs-Kürzel vereinheitlicht:** iOS-Nachträge `E-1…E-4` → **V3-1…V3-4** (Kollision mit Feldtest-Kürzeln E1–E5 aufgelöst, s. `open_issues.md`/`decision_log.md`); Legende in Kap. 12 ergänzt. Keine inhaltliche Änderung an Code/Verhalten. | Doku-Konsistenz (Label-Kollision) |

### 0.4 Entwicklungsphasen (Charter)
Phase 1 **[freigegeben]** → Phase 2 IA **[abgeschlossen]** → Phase 3 UX **[abgeschlossen]** → Phase 4 Softwarearchitektur **[abgeschlossen]** → Phase 5 Projektstruktur **[freigegeben]** → Phase 6 Implementierung **[in Umsetzung — Xcode + Claude Code; Schema-v3-Migration abgeschlossen]**.

### 0.5 Umsetzungsstand Phase 6 (Xcode/Claude Code, Stand 07.08.2026)
Entwicklung slice-weise in Xcode 26.3 mit dem eingebauten Claude-Assistenten; nach jeder getesteten Scheibe ein Git-Commit. Bislang **umgesetzt & verifiziert:**
- Projekt-Bootstrap (Xcode-App + lokales Package `SmartBikeCore`); Core-Unit-Tests grün (Decoder, Statistik, kumulierte Distanz).
- **Live-Cockpit** (Bereitschaft + Aufzeichnung), **manuelle Aufzeichnung** mit Live-Distanz/Fahrzeit/Ø, Hold-to-Stop.
- **Persistenz** (SwiftData) + **Verlauf**; **Fahrt-Detail** (Statistik + Höhen-/Geschwindigkeitsdiagramm über Distanz + Routenkarte).
- **Liquid Glass** für Steuerelemente (s. Kap. 8).
- **Recovery unterbrochener Aufzeichnungen (AR-DATA-04):** Dialog „Unterbrochene Fahrt gefunden“ → Abschließen / Verwerfen / Weiter fahren (Zeitstempel-Kontinuität via `tOffset`). Unit-Tests grün (3/3), on-screen nachgewiesen.
- **Echte BLE-Verbindung (`BLEConnectionService`, Core Bluetooth):** `MockTelemetrySource` → reale Quelle getauscht (eine Umschaltstelle; Simulator nutzt weiter den Mock). **Am realen iPhone verifiziert:** Auto-Connect per Service-UUID, Live-Empfang, Auto-Reconnect; Verbindungszustand (Suchend/Verbindend/Verbunden/„kein Fix“) in der Statuszeile.
- **Robuste Aufzeichnungszeit (`RecordingClock`):** monotone Zeit gegen **Geräte-Uhr-Reset** und Funklücken — behebt „Dauer 1191:44:03 / 784 km“; am Gerät bestätigt (s. `docs/lessons_learned.md`).
- **Stale-/Ungültig-Behandlung (AR-UX-05):** bei Verbindungsverlust Werte „–“/abgedimmt; bei GNSS-Fix-Verlust „kein Fix“, **Distanz nur bei gültigem Fix** integriert. Am Gerät verifiziert.
- **Höhe barometrisch (BMP280):** Höhe/Höhenmeter/Höhenprofil aus `pressure_pa`; GNSS-Höhe nur Fallback. Höhenprofil-Robustheit; Fahrt-Detail lädt off-main.
- **Schema-v2-Dekodierung:** Decoder auf 81 Byte; `brake_light_pct` gelesen (durch v3 abgelöst, s. u.). v2-Mock speiste plausible Werte.
- **Bremslicht-Validierung + Validierungs-Export:** Fahrt-Detail mit **Doppelachsen-Diagramm** (Verzögerung ⇄ Bremslicht %, Fail-Safe markiert) + **validierungsvollständiger CSV-Export** via Share-Sheet. Am Simulator geprüft.
- **System-/Sensorwarnungen (AR-LIVE-03) vollständig:** aus den Firmware-Statusfeldern (`imu_health_state` RECOVERING/FAILED, `init_degraded`, `watchdog_recovered`, `system_state` Init, `baro_valid`) gestufte, **nicht-modale Warn-Chips** in der Statuszeile (Warnung=Rot, Info=Amber), antippbar → aufklappbare Liste; reine Ableitung (`SystemWarnings.derive`) host-getestet. Chips nur bei realen Firmware-Zuständen sichtbar (Mock = gesund).
- **Ø/Fahrzeit „bewegt“:** Fahrzeit + Ø-Geschwindigkeit zählen nur **bewegte** Zeit (Speed ≥ 1,0 km/h **und** gültiger Fix); reine Gesamt-Aufzeichnungszeit intern als `totalDuration` erhalten. Cockpit + Verlauf zeigen die bewegten Werte.

**Neu abgeschlossen — Frame-Schema-v3-Migration (AP0–AP8), 89 Tests grün (SmartBikeCore 57, App-Unit 29, UITest 3), gegen Firmware-Golden-Vektor kreuzvalidiert:**
- **AP1/AP2 — Decoder v3:** `TelemetryFrame` um 13 v3-Felder (Optionals); Decoder liest 113-Byte-v3-Frames über `withUnsafeBytes`+`loadUnaligned`. **Regel geändert:** statt `version == 2` nun **Mindestversion/-länge** (`version≥2 & len≥81` → v2-Felder; `version≥3 & len≥113` → v3-Felder; überzählige Bytes ignorieren; `len<81` oder `version<2` → verwerfen + Fehlerzähler). Decoder **rein** (Ergebnis-Enum `.ok/.truncatedV3/.rejected`), Zähler (`decodeErrorCount`, `truncatedV3FrameCount`) im `TelemetryStore` (MainActor).
- **AP2b — Golden-Vektor-Kreuztest (V3-3):** Der Produktions-Decoder dekodiert einen von der Firmware bereitgestellten 113-Byte-Golden-Vektor (`testdata/frame_v3_golden.hex/.md`) und prüft **jedes** Feld gegen die Firmware-Wertetabelle — **nicht** über den eigenen Encoder. Bestätigt: App-Decoder und Firmware-Encoder meinen dieselbe Bytebelegung (43 Felder getroffen).
- **AP3 — Mock v3:** `MockTelemetrySource` sendet 113-Byte-v3-Frames über den gemeinsamen, reinen `TelemetryFrameEncoder` (eine Byte-Quelle für Mock **und** Round-Trip-Test), inkl. Sonderfälle `gnss_accel_valid==0` (dann `gnss_accel_ms2=0`) und `bias_calibrated==0`. Kein BLE-Write ans Gerät (FR-SYS-04).
- **AP4 — Store-Split:** genau zwei v3-Größen als Live-Status promotet (`bias_calibrated`, `gnss_accel_valid`); die 11 Analyse-/Aggregatfelder bleiben im letzten Frame und gehen nur in die Persistenz. Keine Cockpit-Warn-Chips daraus (AR-UX-01).
- **AP5 — Persistenz + Migration:** `TrackSample`/`TrackPoint` um die 13 v3-Felder additiv (Optionals); `temperature_c` additiv **in die Persistenz** aufgenommen (war zuvor nur im Live-Pfad). Leichtgewichtige, additive SwiftData-Migration; Altdaten laden mit nil-v3-Feldern.
- **AP6 — Aufzeichnungsrate (E2, Feldtest):** umschaltbarer `RecordingMode` **1 Hz (Default) / 10 Hz (Validierung)**; Umschalten nur außerhalb einer Aufzeichnung; **Batch-Persistenz** (~1 Save/s statt 1 Save/Sample) über den Hintergrund-ModelActor; Skalen-Test 6000 Samples (10 min) ohne Verlust, Zeitstempel eindeutig.
- **AP7 — CSV 35 Spalten:** `temperature_c` aus der CSV entfernt (E4, Feldtest), 13 v3-Felder in Offset-Reihenfolge angehängt; Präambel-`schema_version` = **Geräte-Frame-Version** (bei gemischten Versionen Minimum + `frame_version_gemischt`, V3-2); `t_s` ≥ 2 Nachkommastellen (V3-4); Header per Golden-Test eingefroren. Format-Referenz: `docs/CSV_Format_v3_Validierungsexport.md`.
- **AP8 — Diagnoseansicht:** read-only Ansicht unter Settings mit v3-Status (`bias_calibrated`, `gnss_accel_valid`), Zeitbudget (`dt_max_ms`/`loop_max_us`) und Defekt-Indikator `truncatedV3FrameCount` (V3-1). Nichts davon im Cockpit, nicht-modal (AR-UX-01). Am realen iPhone verifiziert (u. a. `truncatedV3FrameCount == 0`, dt_max 10 ms, loop_max 53 µs).

**Test-/Validierungshilfen:** Debug-Seed `SEED_INTERRUPTED_RIDE=1` (nur `#if DEBUG`, via `SIMCTL_CHILD_…`) erzeugt reproduzierbar den Recovery-Dialog ohne UI-Interaktion. Für den Zeit-/Distanz-Fix existiert ein belegbares **Vorher/Nachher** am Gerät (1191:44:03 / 784 km vs. korrekt) — Validierungs-Abbildung. Der **Golden-Vektor-Kreuztest** ist der geräteunabhängige Schnittstellen-Nachweis.

**Offen:** Cockpit-Editor (AR-LIVE-08); GNSS-Freiland-Fix für echte Route/Höhen-Absolutwert; optionale Höhen-Referenzdruck-Kalibrierung; finale SF-Symbol-Wahl (ans Ende gelegt); reale Feldvalidierung der App-Daten (gegen Strava, Leitfaden liegt vor); Feintuning.

---

## 1. Vision
Die App ist der native iOS-Begleiter des smarten Fahrrad-Rücklichts. Sie empfängt dessen Live-Telemetrie über BLE, zeigt dem Fahrer während der Fahrt wenige sicherheitsrelevante Kennzahlen ablenkungsarm an, zeichnet jede Fahrt verlustarm lokal auf und macht sie danach als Statistik, Höhenprofil und Verlauf auswertbar. **[bestätigt]**

## 2. Ziele
- **Technisch hochwertig & wartbar:** modulare, testbare, erweiterbare, dokumentierte SwiftUI-App.
- **Zuverlässige Live-Anzeige:** stabiler BLE-Empfang, ablenkungsarmes Cockpit.
- **Verlustarme Aufzeichnung:** nutzt firmwareseitigen Ringpuffer + Reconnect-Backfill (FR-TEL-04).
- **Auswertbare Historie:** lokale, dauerhafte Fahrtspeicherung; Statistik & Visualisierung.
- **Personalisierbar:** der Fahrer passt das Live-Cockpit an (User-Centered Design).
- **Wissenschaftlich nachvollziehbar:** jede Entscheidung begründet und maschinenlesbar dokumentiert.

## 3. Zielnutzer & Rahmenbedingungen
- **Zielnutzer:** Eigennutzung — ein Fahrer, ein iPhone, ein Rücklicht; kein Onboarding/Multi-Device im MVP. **[bestätigt]**
- **Sprache:** Deutsch · **Einheiten:** metrisch. **[bestätigt]**
- **Plattform:** iOS, Swift/SwiftUI, Core Bluetooth, Swift Charts, SF Symbols. **[aus Project Bible 7]**
- **Deployment-Target:** neueste iOS-Version (Xcode 26 / iOS 26, Eigennutzung). **[bestätigt]**
- **Test/Deploy:** physisches iPhone; Personal Team (7-Tage-Profil). **[aus Project Bible 7.5/7.6]**
- **Berechtigungen:** nur Bluetooth (`NSBluetoothAlwaysUsageDescription`); keine Standortberechtigung. **[bestätigt]**

## 4. Nutzungskontext & User Journeys
**Vor der Fahrt** — Auto-Connect → Status/Fix prüfen → manuell starten (Nur-Anzeige-Zustand). *Kein Akku-Check (OUT-01).*
**Während der Fahrt** — personalisierbares Cockpit + Statuszeile; Hintergrundbetrieb. Keine Live-Karte; Bremsereignisse → V2.
**Nach der Fahrt** — Fahrtenliste → Detailansicht (Statistik + Höhen-/Geschwindigkeitsdiagramm über Distanz + Routenkarte + Bremslicht-Validierungsdiagramm). Gesamtübersicht (Could). **CSV-Validierungsexport (Fahrt-Detail, 35 Spalten) vorhanden; GPX → V2.**

## 5. Scope — MVP vs. V2
**MVP-Kennzahlen [Project Bible 2.4]:** aktuelle/Ø/Max-Geschwindigkeit, Distanz, Fahrzeit, Höhe, Höhenmeter, Satellitenanzahl, BLE-Status. Kein Akkustand.
**MVP-Zusatz:** Fahrtenliste + Detailansicht; Höhen-/Geschwindigkeitsdiagramm (über Distanz); Routenkarte **Should** (fix-abhängig); Gesamtübersicht **Could**; Hintergrund-Aufzeichnung; automatischer Reconnect; personalisierbares Live-Cockpit (Variante B, Should — Standard-Layout zuerst); **CSV-Validierungsexport der Fahrt** (35 Spalten, Schema v3; alle Frame-Felder außer rohen IMU-Achsen, `temperature_c` aus dem Export entfernt) als Validierungswerkzeug (v0.20/0.22); **umschaltbarer 10-Hz-Validierungsmodus** (Default 1 Hz) als Messwerkzeug für die Bremsdynamik (v0.22); **read-only Diagnoseansicht** unter Settings (v3-Status/Zeitbudget/Defekt-Indikator, v0.22).
**V2 / Won't:** Freiform-Dashboard, Live-Karte, **integrierte Fahrrad-Navigation im Cockpit** (Routenwahl vor der Fahrt + Turn-by-turn + Live-Karte während der Fahrt), Bremsereignisse, Temperaturdiagramm, GPX-Export, Cloud-Sync, Apple Watch, HealthKit, Strava, Widgets, Live Activities, Multi-Device, Batterie-Telemetrie, Auto-Fahrterkennung, erweiterte Sensorfusion (GNSS-Fusion), Sturz-/Road-Quality, dauerhafte IMU-Hochrate.

**Future Work — integrierte Fahrrad-Navigation (evaluiert 01.08.2026, bewusst nach V2 abgegrenzt).** Idee: vor der Fahrt eine Fahrradroute wählen, während der Fahrt Turn-by-turn-Anweisungen + Live-Karte im Cockpit. Bewusst *nicht* im MVP, aus vier Gründen: (1) **API-Grenze:** Apples MapKit-Entwickler-API bietet kein Fahrrad-Routing (`MKDirections.Request.transportType` kennt nur `.automobile`/`.walking`/`.transit`); Radrouting erfordert einen Fremddienst (z. B. Mapbox, Google Routes `BICYCLE`, OpenRouteService/GraphHopper) mit externer Abhängigkeit, API-Schlüssel, Netzwerk und ggf. Kosten. (2) **Standort:** Turn-by-turn erzwingt die iPhone-eigene Live-Ortung (Core Location) — Bruch der bewussten Entscheidung „keine Standortberechtigung“ (AR-NFR-SEC-01); die GNSS-Daten des Rücklichts sind Telemetrie, keine Navigations-Ortung. (3) **Bedienkonzept:** widerspricht der Entscheidung „keine Live-Karte“ und dem Ein-Blick-Prinzip (AR-UX-01, Ablenkungsarmut). (4) **Produktcharakter/Scope:** macht aus der reinen Datensenke (AR-CTX-01) eine aktive Navigations-App — faktisch ein zweites Produkt. Turn-by-turn ist zudem selbst zu bauen (MapKit liefert nur `MKRoute.steps`, keine Navigations-UI) oder erfordert ein schweres SDK (Mapbox Navigation). Empfohlener Alternativpfad: Navigation an eine dedizierte App (komoot/Apple Karten) delegieren; die App bleibt Telemetrie-/Rücklicht-Begleiter. **Post-Ride-Routendarstellung** (aus GNSS-Daten, ohne Standortberechtigung) bleibt hiervon unberührt und im Scope (AR-VIS-03).

---

## 6. Anforderungskatalog

### 6.1 ID-Schema
`AR-<Kategorie>-NN`. Kategorien: `CTX`, `CONN`, `LIVE`, `REC`, `DATA`, `STAT`, `VIS`, `UX`, `NFR-<…>`. MoSCoW.

### 6.2 Funktionale & kontextuelle Anforderungen (bestätigt)
**AR-CTX-01** Reine Datensenke · Must. **AR-CTX-02** Duales Nutzungsmodell · Must. **AR-CTX-03** Single-User · Must. **AR-CTX-04** Sprache Deutsch · Should.
**AR-REC-01** Manuelle Fahrtaufzeichnung · Must.
**AR-DATA-01** Daten-Lebenszyklus · Must. **AR-DATA-02** Persistenz 1 Hz (Default) / **10 Hz (Validierungsmodus, v0.22)** · Must. **AR-DATA-03** SwiftData, repository-gekapselt · Must. **AR-DATA-04** Verlustschutz & Recovery · Must **[umgesetzt & verifiziert, v0.15/0.16 — drei Optionen]**.
**AR-LIVE-01** Standard-Layout „Hero + 3“ · Must. **AR-LIVE-02** Statuszeile · Must. **AR-LIVE-03** System-/Sensorwarnung · Should **[umgesetzt & verifiziert (v0.21): GNSS-Fix/Stale + Firmware-Statuswarnungen IMU/degraded/Watchdog/Init/Baro]**. **AR-LIVE-04** Start/Stopp-Steuerung · Must. **AR-LIVE-05** Zwei Cockpit-Zustände · Must. **AR-LIVE-06** Sekundärwerte · Should. **AR-LIVE-07** Live-Berechnung Distanz & Ø · Must **[Ø/Fahrzeit = „bewegt“, v0.21]**. **AR-LIVE-08** Konfigurierbares Dashboard · Should. **AR-LIVE-09** Metrik-Katalog · Should.
**AR-STAT-01** Fahrtenliste · Must. **AR-STAT-02** Fahrt-Detailansicht · Must **[inkl. Bremslicht-Validierungsdiagramm + validierungsvollständiger CSV-Export (35 Spalten), v0.20/0.22]**. **AR-STAT-03** Gesamtübersicht · Could.
**AR-VIS-01** Höhenprofil über Distanz · Must **[Höhe barometrisch, v0.18]**. **AR-VIS-02** Geschwindigkeit über Distanz · Must. **AR-VIS-03** Routenkarte (Post-Ride) · Should.
**AR-CONN-01…08** Verbindungs-Lebenszyklus, Auto-Reconnect, Hintergrund-Aufzeichnung, **robuste versionsgeprüfte Dekodierung (Schema v3, Mindestversions-/Längenregel `≥`, kurzes v3-Frame gezählt statt verworfen)**, Backfill, Stale-Erkennung, Bluetooth-Zustände/Berechtigung, Verbindungsstatus · Must (06 Should) **[Grundverbindung, Live-Empfang, Auto-Reconnect am realen Gerät verifiziert (v0.18); v3-Dekodierung kreuzvalidiert (v0.22)]**.
**Neu (Diagnose, v0.22):** read-only Diagnoseansicht unter Settings (v3-Status `bias_calibrated`/`gnss_accel_valid` + Zeitbudget `dt_max_ms`/`loop_max_us` + Defekt-Indikator `truncatedV3FrameCount`); bewusst **nicht** im Cockpit (AR-UX-01).

### 6.3 Nichtfunktionale Anforderungen (bestätigt)
**AR-NFR-PERF-01** Ruckelfreie 10-Hz-Verarbeitung · Must **[gilt auch für den 10-Hz-Aufzeichnungsmodus → Batch-Persistenz statt Save-pro-Sample, v0.22]**. **AR-NFR-TST-01** Reine Logik voll host-testbar · Must **[um Golden-Vektor-Kreuztest erweitert, v0.22]**. **AR-NFR-EXT-01** Erweiterbarkeit · Should **[Mindestversions-Regel erlaubt unabhängige Firmware-/App-Weiterentwicklung]**. **AR-NFR-SEC-01** Lokal & datensparsam · Must. **AR-NFR-ROB-01** Robustheit · Must. **AR-NFR-A11Y-01** Barrierefreiheit · Should.

### 6.4 Live-Cockpit-Aufbau & Personalisierung (Block C + UX-C/D) — [bestätigt]
Drei Zonen (Primär/Statuszeile/Steuerung), zwei Zustände (Bereitschaft/Aufzeichnung). Standard „Hero + 3“: Geschwindigkeit Hero (3×1) + Distanz/Fahrzeit/Ø (1×1).
**AR-LIVE-08 — Konfigurierbares Dashboard (Feinbild UX-D):** 3-Spalten-Raster; Größen 1×1/2×1/3×1/2×2; Mindestgröße 1×1; max ~6 Kacheln, kein Scrollen (AR-UX-01); Gesten: Betreten über „Bearbeiten“ (nur außerhalb Aufzeichnung), Live-Vorschau, Halten+Ziehen (Verschieben), Tap → Größen-Chips + Metrik-Wähler, „–“ (Entfernen), „+ Kachel“, „Fertig“ speichert, „Auf Standard zurücksetzen“; persistiert (`DashboardLayout`), host-testbar. · Should.
**AR-LIVE-09 — Metrik-Katalog (Registry):** erweiterbar; jede Metrik liefert Wert/Einheit/Label/Formatierung. · Should.

### 6.5 Nachher-Ansichten (Block D) — [bestätigt]
Fahrtenliste (Gesamtübersicht + Wisch-Löschen) → Detailansicht (Kennzahlen + Höhen-/Geschwindigkeitsdiagramm über Distanz + Routenkarte + Bremslicht-Validierungsdiagramm + CSV-Export).

### 6.6 BLE-Verbindungsverhalten (Block E) — [bestätigt]
Auto-Connect + Auto-Reconnect, Hintergrundbetrieb, robuste versionsgeprüfte Dekodierung (Schema v3), Backfill, Stale-Erkennung. Keine Standortberechtigung.

### 6.7 Bedienkonzept & Interaktion (UX-B) — [bestätigt]
**AR-UX-01** Ein-Blick-Prinzip (keine modalen Unterbrechungen im Fahrbetrieb; Cockpit ohne Scrollen erfassbar) · Must. **AR-UX-02** Start = Tap / Stopp = Halten (~1 s) · Must. **AR-UX-03** Gestufte Status-/Fehlerkommunikation · Must **[Warn-Chips AR-LIVE-03, v0.21]**. **AR-UX-04** Dezente Haptik · Should. **AR-UX-05** Stale-Werte abdimmen + kennzeichnen · Must **[umgesetzt & verifiziert, v0.18]**.

---

## 7. Informationsarchitektur & Navigation — [bestätigt, Phase 2]
TabBar mit drei Tabs (Start-Tab = Live): **Live** (Cockpit → Editor; „Gerät suchen“) · **Verlauf** (Fahrtenliste → Fahrt-Detail) · **Einstellungen** (Gerät · Cockpit · **Aufzeichnung 1 Hz/10 Hz** · **Diagnose** · Anzeige · Info). Post-Stopp: Zusammenfassung als Sheet über Live. IA-/Flussdiagramm + Screen-Wireframes als Thesis-Abbildungen.

## 8. UX & Designsystem — [UX-A…D bestätigt]
**Designsystem (UX-A):** SF Pro + Dynamic Type; SF Symbols; 8-pt-Raster, Kachelradius ~16 pt, Tap-Target ≥ 44 pt; App folgt System-Hell/Dunkel. Farbe neutral/monochrom + adaptiver „Electric Cyan“-Akzent (Dunkel `#22D3EE`, Hell `#0E7490`, Kontrast ≥ 4,5:1); Semantik Rot/Grün/Amber; Diagramme Geschwindigkeit=Cyan, Höhe=Slate. Ziffern SF Pro Rounded, tabellarisch. Style-Tile erstellt.
**Material — Liquid Glass (iOS 26) [bestätigt]:** **nur** für Chrome/Steuerelemente einsetzen — Statuszeile-Pille, Start/Stopp-Button, schwebende Aktions-Buttons (TabBar/Navigation/Sheets vergibt iOS 26 automatisch; nicht mit eigenen Hintergründen überschreiben). **Inhalte bleiben glasfrei und lesbar** (Kacheln, Charts, Listenzeilen) — Vorrang für Glanceability/Lesbarkeit (AR-UX-01). API `.glassEffect(_:in:)` (nach den Layout-Modifiern), `.interactive()` nur auf tippbaren Elementen, `.buttonStyle(.glass/.glassProminent)`, mehrere nahe Glas-Elemente in `GlassEffectContainer`. Fallback < iOS 26: `.ultraThinMaterial`. **Dieselbe Regel steht in `CLAUDE.md`** (Repo), damit sie für Claude Code in Xcode verbindlich ist. Die **Diagnoseansicht** (v0.22) ist eine schlichte, nicht-modale Form-Liste.
**Bedienkonzept (UX-B):** AR-UX-01…05.
**Wireframes (UX-C) + Editor-Feinbild (UX-D):** als Thesis-Abbildungen erstellt; Details s. 6.4.

## 9. Softwarearchitektur — [bestätigt, Phase 4]

### 9.1 Grundmuster
**MVVM + leichte Clean-Schichten**, alle Schichtgrenzen über **Protokolle** entkoppelt. Nebenläufigkeit/State: **`@Observable` (Observation) + async/await + Actors**. **Zentraler `TelemetryStore`** als einzige Live-Wahrheitsquelle. **Manueller Composition Root** (kein DI-Framework). Verworfen: schlankes MV (zu geringe Testisolation), volle Clean Architecture (Overhead), TCA/Redux (externe Abhängigkeit).

### 9.2 Schichten & Schnittstellen (Stand v3)
| # | Modul | Typ | Verantwortung | Schnittstelle | Anforderungen |
|---|---|---|---|---|---|
| 1 | `BLEConnectionService` | **Actor** (Core Bluetooth) | Scan/Connect/Auto-Reconnect/**NOTIFY-Abo (kein Write)** | `TelemetrySource`: `AsyncStream<Data>` + `ConnectionState` | AR-CONN-01/02/03/07/08 |
| 2 | `TelemetryFrameDecoder` | **rein** | `Data → Ergebnis` (`.ok/.truncatedV3/.rejected`); **Mindestversions-/Längenregel v3**, LE, feste Offsets, `loadUnaligned` | reine Funktion, host-testbar | AR-CONN-04, AR-NFR-TST-01 |
| 2a | `TelemetryFrameEncoder` | **rein** | `TelemetryFrame → Data` (113 B); **einzige** Byte-Layout-Quelle für Mock + Round-Trip-Test; **nie** an einen BLE-Write angebunden (FR-SYS-04) | reine Funktion, host-testbar | AR-NFR-TST-01 |
| 3 | `TelemetryStore` | **@MainActor · @Observable** | letzter Frame + Live-Werte + Connection/Fix/Stale-Status + Warnungen + **Zähler** (`decodeErrorCount`, `truncatedV3FrameCount`) + **zwei v3-Live-Status** (`bias_calibrated`, `gnss_accel_valid`) | beobachtbar für ViewModels | AR-LIVE-02/03, AR-UX-05 |
| 4 | `RideManager` | **@Observable** | Aufzeichnungs-Lebenszyklus, **`RecordingMode` 1/10 Hz**, 1-Hz-Verdichtung bzw. 10-Hz-ohne-Verdichtung, **Batch-Persistenz**, Backfill idempotent, Recovery, monotone Zeit (`RecordingClock`) | nutzt `RideRepository`, `StatisticsEngine` | AR-REC-01, AR-DATA-01/04, AR-CONN-05 |
| 5 | `RideRepository` / `SwiftDataStore` | **Protokoll** / **Hintergrund-ModelActor** | CRUD Fahrten/Samples, **`appendBatch` (ein `save()` je Batch)**, Recovery `recording`, Löschen; Mock für Tests | `RideRepository`-Protokoll | AR-DATA-02/03/04 |
| 6 | `StatisticsEngine` + `RecordingClock` + `LiveEvaluators` + `SystemWarnings` + `RideCSVExporter` + `RecordingMode` | **rein** | Statistik (bewegt); monotone Zeit/dt-Kappung; LiveDataState/Fix-Gültigkeit/Baro-Höhe; Warnungs-Ableitung; **35-Spalten-Validierungs-CSV**; Raten-Bucketing | reine Typen, host-testbar | AR-LIVE-03/07, AR-UX-05, AR-STAT/VIS |
| 7 | `MetricRegistry` + `DashboardLayoutStore` | **rein** | `metricId → Wert/Einheit/Label/Format`; Layout laden/speichern + Constraints | rein, host-testbar | AR-LIVE-08/09 |
| 8 | ViewModels (`CockpitVM`, `HistoryVM`, `RideDetailVM`, `SettingsVM`, `EditorVM`, **`DiagnosticsViewModel` + `DiagnosticsReadout` (rein)**) | **@Observable** | Präsentationslogik je Screen; Diagnose-Ableitung host-testbar | nur Protokoll-Abhängigkeiten | — |
| 9 | SwiftUI Views (inkl. **`DiagnosticsView`**) | zustandsarm | Darstellung/Bindung | binden an ViewModels | — |

*(Für die hardwareunabhängige Entwicklung existiert zusätzlich `MockTelemetrySource` als `TelemetrySource`-Implementierung — Kap. 0.5. Auswahl Mock/echt an einer einzigen Stelle im Composition Root; Simulator immer Mock; Mock sendet **Schema-v3-Frames** über den gemeinsamen `TelemetryFrameEncoder`, inkl. Sonderfälle `gnss_accel_valid==0`/`bias_calibrated==0`, sonst Status „gesund“.)*

### 9.3 Nebenläufigkeit & Datenfluss
BLE-Empfang und Dekodierung laufen **abseits des Main-Threads** (Actor); `TelemetryStore` und ViewModels sind **`@MainActor`**; SwiftData-Schreibvorgänge laufen über einen **Hintergrund-`ModelActor`** (Main-Thread nie blockiert), im 10-Hz-Modus **gebatcht** (~1 Save/s). Reine Engines sind synchron/thread-neutral. Der Decoder ist **rein** (kein geteilter Zustand); die Zähler (`decodeErrorCount`, `truncatedV3FrameCount`) werden im Decode-Consumer/`TelemetryStore` (MainActor) geführt — kein Cross-Actor-Zugriff auf einen zustandsbehafteten Decoder. Die Fahrt-Detailansicht baut Diagramm-Serien/Route **und den CSV-Text off-main** auf (kurzer Ladezustand). **UI-Aktualisierung: volle 10 Hz** (pro UI-Frame zusammengefasst), AR-NFR-PERF-01.
**Kern-Fluss:** `BLEConnectionService → TelemetryFrameDecoder → TelemetryStore → ViewModels → Views`. Seitenstrang bei Aufzeichnung: `RideManager` verdichtet (1 Hz) bzw. übernimmt (10 Hz) → `RideRepository` (Hintergrund, gebatcht) und `StatisticsEngine`.

### 9.4 Zustandsmodell (explizite, testbare Enums)
`ConnectionState` (getrennt/suchend/verbindend/verbunden/Bluetooth-aus/nicht-autorisiert) · `LiveDataState` (frisch/veraltet/keine — AR-UX-05, alle 300 ms auch ohne neue Frames neu bewertet) · `RecordingState` (leer/aufzeichnend/abschließend — AR-DATA-01) · **`RecordingMode` (1 Hz / 10 Hz)**. **Decode-Ergebnis** als Enum (`.ok / .truncatedV3 / .rejected`); `truncatedV3FrameCount` (V3-1) als Defekt-Indikator (zu kurze v3-Frames = abgeschnittene Notification/MTU-Problem). GNSS-Fix-Gültigkeit (`gnss_fix_status`/`sats`) gated GNSS-abgeleitete Werte + Distanzintegration. **Firmware-Statusfelder (IMU-Health, degraded, Watchdog, Init, Baro) → gestufte Warn-Chips in der Statuszeile (`SystemWarnings`, AR-LIVE-03, v0.21);** `imu_health` zusätzlich als Fail-Safe-Markierung im Bremslicht-Diagramm. **`bias_calibrated`/`gnss_accel_valid` → Diagnoseansicht (nicht Cockpit, v0.22).**

### 9.5 Dependency Injection
Manueller **Composition Root** beim App-Start erzeugt die konkreten Implementierungen und reicht sie via Initializer / SwiftUI-Environment durch. Kein DI-Framework.

### 9.6 Testarchitektur
Reine Einheiten (Decoder, Encoder, StatisticsEngine, RecordingClock, LiveEvaluators, SystemWarnings, RideCSVExporter, MetricRegistry, DashboardLayoutStore, RecordingMode) per Host-Unit-Tests; `RideManager` gegen **Mock-`RideRepository`** + synthetischen Frame-Stream (inkl. Recovery-Pfad AR-DATA-04, Raten-/Batch-/Umschaltsperre-Tests und **Skalen-Test 6000 Samples**); ViewModels (inkl. `DiagnosticsViewModel`) gegen Mock-Stores — ohne Gerät/UI (AR-NFR-TST-01). **Golden-Vektor-Kreuztest (V3-3):** Produktions-Decoder gegen den Firmware-Golden-Vektor (`testdata/frame_v3_golden.hex/.md`) — der **einzige** Test, der die Schnittstelle selbst geräteunabhängig validiert; **nicht** über den eigenen Encoder. Round-Trip nutzt den gemeinsamen `TelemetryFrameEncoder`. Echte BLE-Verbindung nur On-Target. Architektur-/Schichtendiagramm als Thesis-Abbildung erstellt. **Stand: 89 Tests grün** (SmartBikeCore 57, App-Unit 29, UITest 3).

### 9.7 Projektstruktur & Ablageort (Phase 5) — [bestätigt]
- **Ablageort:** neuer Ordner **`ios-app/`** im bestehenden Monorepo `smart-bike-rearlight/` (neben `firmware/`, `cad/`, `docs/`). Das Xcode-Projekt liegt in `ios-app/SmartBikeRearLight/`. Ein Repo/eine Historie, gemeinsame Doku. (📖 Project-Bible-relevant: löst „Ablageort App-Code“ aus Project Bible 6.4 auf.)
- **Gruppierung:** **feature-orientiert** (je Screen View + ViewModel beisammen) + geteilte Bereiche.
- **Modularisierung:** reine, UI-freie Logik als lokales Swift Package **`SmartBikeCore`** (physische Logik-Grenze, schnelle Host-Tests auf dem Mac; analog zur `logic/`-Trennung der Firmware).

```
ios-app/SmartBikeRearLight/
  SmartBikeRearLight.xcodeproj
  CLAUDE.md · README.md
  App/            SmartBikeRearLightApp.swift · RootTabView.swift · AppEnvironment.swift (Composition Root)
  Features/       Cockpit/ · History/ · Settings/ (inkl. DiagnosticsView/DiagnosticsViewModel)   (je View + ViewModel)
  Core/  (Swift Package „SmartBikeCore“ — rein, UI-frei)
    Sources/SmartBikeCore/  Models/ · Telemetry/ (Decoder + Encoder) · Statistics/ · Recording/ (inkl. RecordingMode) · Live/ · Export/ · Metrics/
    Tests/SmartBikeCoreTests/ (inkl. FrameGoldenVectorTests)
  Services/  BLE/ (inkl. MockTelemetrySource) · Persistence/ · Live/
  DesignSystem/  Theme · Components (MetricTile, StatusBar, GlassBackground …)
  Resources/     Assets.xcassets (AccentColor adaptiv, AppIcon)
  SmartBikeRearLightTests/  (App-/Persistenz-/Diagnose-Tests)
  testdata/  frame_v3_golden.hex · frame_v3_golden.md  (von der Firmware bereitgestellt, Golden-Vektor)
```

**Ordner → Schichten (Kap. 9.2):** `Core` = Schichten 2/2a/6/7 + Modelle · `Services/BLE` = Schicht 1 · `Services/Persistence` = Schicht 5 · `Services/Live` = Schichten 3/4 · `Features` = Schichten 8/9 · `App` = Composition Root.

---

## 10. BLE-Vertrag (autoritativ, aus Firmware — Schema v3) [aus `claude/BLE_Frame_v3_Schnittstelle.md`]
> Quelle: `claude/BLE_Frame_v3_Schnittstelle.md` (Schema v3 ab 07.08.2026, 113 Byte) sowie `firmware/include/config.h`, `firmware/lib/logic/telemetry_frame.h`, `firmware/lib/drivers/ble_telemetry.cpp`. **Unveränderlich durch die App** (FR-SYS-04). **Firmware-BLE-Transport am realen Gerät validiert** (Board-Tausch behebt den früheren Brown-Out; Advertising mit Gerätenamen in der **Scan-Response**, 128-Bit-Service-UUID im Primärpaket, MTU 185; s. Project Bible / `docs/ble_brownout_fallstudie.md`). Hinweis für die App-Zentrale: **per Service-UUID scannen** (`scanForPeripherals(withServices:)`), Name ggf. aus `CBAdvertisementDataLocalNameKey` (Scan-Response) lesen. **App-Decoder gegen den Firmware-Golden-Vektor kreuzvalidiert** (V3-3).

- **Geräte-Name:** `SmartBikeRearLight` · **Service-UUID:** `587bb505-9f9d-4ae0-96fd-0b29adfc4b03` · **Characteristic (NOTIFY):** `8c604d09-743f-4850-9109-19604a17f358`
- **Frame:** **113 Byte**, Little-Endian, gepackt, 10 Hz · **Schema-Version:** `3` (Offset 0) · **MTU:** ≥ 116 nötig (ausgehandelt 185, Nutzlast 182 → 113-Byte-Frame in einer Notification, keine Fragmentierung)
- **Reconnect/Backfill:** Advertising nach Disconnect automatisch; Nachlieferung gepufferter Frames (~60 s @ 10 Hz).

**Frame-Layout (Offsets):**

| Off | Bytes | Typ | Feld |
|---|---|---|---|
| 0 | 2 | uint16 | version (=3) |
| 2 | 4 | uint32 | timestamp_ms |
| 6 | 4 | float | accel_x_ms2 |
| 10 | 4 | float | accel_y_ms2 |
| 14 | 4 | float | accel_z_ms2 |
| 18 | 4 | float | gyro_x_rads |
| 22 | 4 | float | gyro_y_rads |
| 26 | 4 | float | gyro_z_rads |
| 30 | 4 | float | brake_decel_ms2 (roher Verzögerungs-**Eingang**, motion_filter) |
| 34 | 4 | float | pressure_pa |
| 38 | 4 | float | temperature_c |
| 42 | 4 | float | lat (double→float, ~2 m) |
| 46 | 4 | float | lon (dito) |
| 50 | 4 | float | speed_kmph |
| 54 | 4 | float | course_deg |
| 58 | 4 | float | altitude_m |
| 62 | 1 | uint8 | sats |
| 63 | 4 | float | hdop (nicht typ-aligned) |
| 67 | 2 | uint16 | utc_year |
| 69 | 1 | uint8 | utc_month |
| 70 | 1 | uint8 | utc_day |
| 71 | 1 | uint8 | utc_hour |
| 72 | 1 | uint8 | utc_minute |
| 73 | 1 | uint8 | utc_second |
| 74 | 1 | uint8 | system_state (0=Init,1=Run) |
| 75 | 1 | uint8 | init_degraded (0/1) |
| 76 | 1 | uint8 | imu_health_state (0=OK,1=RECOVERING,2=FAILED) |
| 77 | 1 | uint8 | baro_valid (0/1) |
| 78 | 1 | uint8 | gnss_fix_status (0=NO_DATA,1=NO_FIX,2=FIX_OK) |
| 79 | 1 | uint8 | watchdog_recovered (0/1) |
| 80 | 1 | uint8 | brake_light_pct (0..100, kommandierte LED-Duty; **Ausgang** der Bremslogik) |
| 81 | 4 | float | gnss_accel_ms2 (GNSS-Referenzverzögerung; nur gültig wenn `gnss_accel_valid==1`, sonst 0) |
| 85 | 4 | float | pitch_rad (interne Lageschätzung motion_filter) |
| 89 | 4 | float | gyro_bias_rads (geschätzter Nullpunktfehler gyro_x) |
| 93 | 4 | float | norm_delta_min (Min von ‖a‖−g im 100-ms-Fenster) |
| 97 | 4 | float | norm_delta_max (Max von ‖a‖−g im Fenster) |
| 101 | 4 | float | jerk_max (Max \|Δ‖a‖\| dt-normiert im Fenster) |
| 105 | 1 | uint8 | regime_static_n (STATIC-Samples im Fenster) |
| 106 | 1 | uint8 | regime_dynamic_n (DYNAMIC-Samples) |
| 107 | 1 | uint8 | regime_shock_n (SHOCK-Samples) |
| 108 | 1 | uint8 | bias_calibrated (0/1, Stufe-1-Bias-Kalibrierung fertig) |
| 109 | 1 | uint8 | gnss_accel_valid (0/1, Gültigkeitsurteil GNSS-Referenz) |
| 110 | 1 | uint8 | dt_max_ms (größtes dt im Fenster, sat. 255) |
| 111 | 2 | uint16 | loop_max_us (längste Schleifendauer im Fenster, sat. 65535) |

**Gesamtlänge: 113 Byte.** Die Felder ab Offset 93 sowie die Regime-Zähler sind **Fensteraggregate** über das 100-ms-Fenster zwischen zwei Frames (100-Hz-Innentakt), keine Momentanwerte; Summe der drei Regime-Zähler = Anzahl verarbeiteter IMU-Samples (nominell 10, Abweichung ist selbst Messgröße, nicht auf 10 normieren).

**App-Decoder — Versions-/Kompatibilitätsregel (Mindestversion/-länge, AR-CONN-04):** feste Offsets, Little-Endian, keine Alignment-Annahmen (`withUnsafeBytes` + `loadUnaligned`).
- `len < 81` → verwerfen, `decodeErrorCount +1`.
- `version < 2` → verwerfen, `decodeErrorCount +1`.
- `version ≥ 2 & len ≥ 81` → v2-Felder (0–80) lesen.
- `version ≥ 3 & len ≥ 113` → zusätzlich v3-Felder (81–112) lesen.
- `version ≥ 3 & 81 ≤ len < 113` (kurzes v3-Frame, **V3-1**) → v2-Ebene lesen (v3 = nil), **kein** Fehler, `truncatedV3FrameCount +1` (Defekt-Indikator: abgeschnittene Notification/MTU).
- überzählige Bytes ignorieren (Vorwärtskompatibilität).

Damit bleibt ein v2-Gerät an einer v3-App lauffähig und umgekehrt — Voraussetzung für die **unabhängige Weiterentwicklung** von Firmware und App in getrennten Toolchains. **Umgesetzt, host-getestet und gegen den Firmware-Golden-Vektor kreuzvalidiert (v0.22).**
**Bremsgrößen (Eingang ⇄ Ausgang):** `brake_decel_ms2` (Offset 30) ist der **rohe, ungegatete** Verzögerungs-Eingang aus `motion_filter`; `brake_light_pct` (Offset 80) ist die **tatsächlich kommandierte** LED-Duty (Ausgang der `tail_light_fsm`, inkl. Fail-Safe/Hysterese/300-ms-Haltezeit). Beide synchron im Frame ⇒ **empirische Validierung der Bremslicht-Logik**; bei `imu_health_state ≠ 0` zeigt das Ausgangsfeld korrekt den Fail-Safe-Wert (Schlusslicht). *(App-seitige Auflösung 1 Hz oder — im Validierungsmodus — 10 Hz; für die Sub-Sekunden-Feindynamik ergänzend Serial-Bench @100 Hz, s. Kap. 13.)*
**Referenzgröße:** `gnss_accel_ms2` (Offset 81) nur verwenden, wenn `gnss_accel_valid==1`; dient der **Beobachtung** (Ist/Soll-Vergleich), nicht der Korrektur (GNSS-Fusion ist firmwareseitig deaktiviert, s. Vertrag Kap. 6).
**Höhenquelle (App):** Höhe/Höhenmeter/Höhenprofil aus `pressure_pa` (BMP280, barometrisch); `altitude_m` (GNSS) nur Fallback bei `baro_valid==0` und gültigem Fix. **Zeitbasis:** `timestamp_ms` gilt als **nicht monotone** Quelle (Reset bei Geräteneustart) und wird über die `RecordingClock` abgesichert (Kap. 11/12).

## 11. Datenmodell & Persistenz — [bestätigt]
Technik: **SwiftData** (Zugriff über `RideRepository`, Schreibvorgänge Hintergrund-ModelActor, im 10-Hz-Modus gebatcht). Auflösung **1 Hz (Default) oder 10 Hz (Validierungsmodus, `RecordingMode`)**. Aufbewahrung: unbegrenzt lokal, manuelles Löschen. Reine Core-Werttypen (`TrackPoint`, `RideStatistics`, `DashboardLayout` …) werden von/zu den SwiftData-`@Model`-Klassen gemappt.
- **`Ride`** — `id`, `startedAt`, `endedAt`, `status`, Ref. `BLEDevice`, eingebettete `RideStatistics`, 1:n `TrackSample`.
- **`TrackSample`** — `t`, `lat`, `lon`, **`altitude_m` (optional; barometrisch aus `pressure_pa`, `nil` wenn höhenlos)**, `speed_kmph`, `course_deg`, `sats`, `hdop`, `gnss_fix_status`; Rohwerte `pressure_pa`, `gnss_altitude_m`, **`temperature_c` (optional; ab v0.22 persistiert, war zuvor nur im Live-Pfad)**; GNSS-UTC bei Fix. **Ab Schema v2:** `brake_decel_ms2` (Eingang), `brake_light_pct` (Ausgang), `imu_health`, `baro_valid`, `system_state`, `init_degraded`, `watchdog_recovered`, `device_timestamp_ms`, `frame_version`. **Ab Schema v3 zusätzlich (13 Felder, optional):** `gnss_accel_ms2`, `pitch_rad`, `gyro_bias_rads`, `norm_delta_min`, `norm_delta_max`, `jerk_max`, `regime_static_n`, `regime_dynamic_n`, `regime_shock_n`, `bias_calibrated`, `gnss_accel_valid`, `dt_max_ms`, `loop_max_us`. **Keine rohen IMU-Achsen** (Aliasing bei 1 Hz → Serial-Bench). Beim „Weiter fahren“ nach Recovery wird `t` über `tOffset` fortgeschrieben. Alle Zusatzfelder **optional** → additive, verlustfreie SwiftData-Migration (Altdaten laden mit nil-v3-Feldern; leichtgewichtig, kein benutzerdefinierter Migrationsplan nötig).
- **`RideStatistics`** — **Dauer & Ø-Geschwindigkeit als „bewegte“ Werte** (nur Zeit/Samples mit Speed ≥ 1,0 km/h **und** gültigem Fix; reine Gesamt-Aufzeichnungszeit als `totalDuration` intern erhalten), Distanz, Max-Geschwindigkeit, Höhenmeter auf/ab (1-m-Totzone, nur zwischen Punkten mit gültiger Höhe), min/max Höhe.
- **`RecordingMode`** (Core, rein) — 1 Hz / 10 Hz; Auswahl in Settings persistiert (UserDefaults); Umschalten nur außerhalb einer Aufzeichnung.
- **`BLEDevice`** — Peripheral-Identifier, Name, zuletzt verbunden.
- **`DashboardLayout`** — geordnete `TileConfig` (`metricId`, `size` 1×1/2×1/3×1/2×2, Position); rücksetzbar.
- **`Settings`** — Einheiten, aktives `DashboardLayout`, `RecordingMode`, spätere Optionen.

**Validierungs-Export (CSV, 35 Spalten, v0.22):** Die Fahrt-Detailansicht exportiert die Fahrt als CSV (`RideCSVExporter`, **35 Spalten**) — alle o. g. Felder, **Roh- und abgeleitete Werte nebeneinander**, mit **Metadaten-Präambel**. Gegenüber v0.20: **`temperature_c` aus der CSV entfernt** (E4, Feldtest — durchgängig leer), **13 v3-Felder angehängt** (Offset-Reihenfolge 81→111). Präambel-`schema_version` = **Frame-Version des Geräts**; bei gemischten Versionen in einer Fahrt **Minimum** + Zeile `# frame_version_gemischt;ja/nein` (V3-2). `t_s` mit **≥ 2 Nachkommastellen** (V3-4; sonst doppelte Zeitstempel bei 10 Hz). Konvention: `;`-Trennzeichen, Dezimalkomma, CRLF, UTF-8-BOM, `sep=;` (deutsches Excel); leere Optionalfelder leer; `fix_status` als `FIX_OK/NO_FIX/NO_DATA`; `imu_health` als Text (Frame-Feld `imu_health_state`). Header per Golden-Test eingefroren. Ausgabe via iOS-Share-Sheet. **Ohne rohe IMU-Achsen.** Verbindliche Format-Doku: `docs/CSV_Format_v3_Validierungsexport.md`.

**Zeitbasis:** **monotone Aufzeichnungszeit (`RecordingClock`)** — der Geräte-Zeitstempel wird als nicht-monotone Quelle behandelt (Reset-Erkennung, `dt`-Kappung 1,5 s, Duplikatverwerfung), s. Kap. 12 / `docs/lessons_learned.md`. **Distanz/Ø nur bei gültigem GNSS-Fix** integriert; `lat`/`lon` für Route gespeichert. **Höhe barometrisch** (Kap. 10).

## 12. Decision Log (App)

> **Entscheidungs-Kürzel (Legende):** iOS-Nachträge = **V3-1** (`truncatedV3FrameCount`), **V3-2** (CSV-Minimum + `frame_version_gemischt`), **V3-3** (Golden-Vektor-Kreuztest), **V3-4** (`t_s` ≥ 2 Nachkommastellen). Feldtest-Entscheidungen (v3-Vertrag/06.08.2026) bleiben **E1–E5**; hier referenziert: **E2** = 10-Hz-Aufzeichnung, **E4** = `temperature_c` aus dem CSV. Kein Kürzel doppelt belegt.

| Entscheidung | Begründung | Verworfene Alternative |
|---|---|---|
| Duales Nutzungsmodell | nutzt 10-Hz-Stream, deckt MVP, HIG | reiner Post-Ride / reines Live-Display |
| Manuelle Fahrtaufzeichnung | deterministisch, testbar, fix-unabhängig | Auto-Erkennung / Hybrid |
| Single-User / Eigennutzung | spart Onboarding-/Multi-Device-Komplexität | Mehrnutzer-App |
| Sprache Deutsch | Konsistenz zu Thesis/Screenshots | Englisch |
| BLE-Vertrag aus Firmware verankert | Firmware client-agnostisch | App diktiert Frame/UUIDs |
| Persistenz 1 Hz · SwiftData repository-gekapselt | kleine DB, modern, testbar | 10 Hz roh / Core Data / SQLite |
| Inkrementelle Persistenz + Recovery | kein Datenverlust bei Absturz | erst am Fahrtende speichern |
| Recovery: Nutzer wählt Abschließen / Verwerfen / Weiter fahren (kein automatisches Fortsetzen; Fortsetzen mit Zeitstempel-Kontinuität via `tOffset`) | Datensouveränität, kein stiller Verlust/Fehlabschluss; deterministisch testbar | automatisch fortsetzen / automatisch verwerfen |
| **Monotone Aufzeichnungszeit (`RecordingClock`) statt roher Geräte-Zeitstempel** | Geräte-`millis()` resettet bei Neustart → uint32-Unterlauf (1191 h / 784 km); Reset-Erkennung + `dt`-Kappung (1,5 s) hält Zeit/Distanz robust; Fix an der Quelle | roher Geräte-Zeitstempel / Wrapping-Arithmetik |
| **Stale-/GNSS-Validität: Werte bei Verbindungs-/Fix-Verlust „–“/„kein Fix“/abgedimmt; keine Distanz ohne Fix** | ehrliche Anzeige statt eingefrorener Werte (AR-UX-05); keine Scheindistanz im Stand ohne Fix | letzten Wert halten / weiter integrieren |
| **Höhe barometrisch (BMP280) statt GNSS** | relativ genau (~1 m), fix-unabhängig (auch indoor/0 Sats), ruhiger Verlauf; GNSS-Höhe vertikal stark verrauscht | GNSS-Höhe als Primärquelle |
| **BLE-Schema v2: `brake_light_pct` (Ausgang) neben `brake_decel_ms2` (Eingang)** | erlaubt empirische „Verzögerung ⇄ Lichtintensität“-Validierung direkt aus App-Daten | nur Eingang senden + App rechnet Duty nach (kann Fail-Safe/Hysterese/Halten nicht reproduzieren) |
| **Validierungs-CSV-Export (alle Frame-Felder außer rohen IMU-Achsen) + Doppelachsen-Bremslicht-Diagramm** | evidenzbasierte Gesamtsystem-Auswertung direkt aus App-Daten (Excel); Roh + abgeleitet nachvollziehbar; IMU-/Feindynamik via Serial-Bench @100 Hz statt 1-Hz-Aliasing | rohe IMU-Achsen bei 1 Hz mitschreiben / kein Export / GPX statt CSV |
| **Ø/Fahrzeit als „bewegte“ Zeit (Schwelle 1,0 km/h, nur bei gültigem Fix; Gesamtzeit intern als `totalDuration` behalten)** | Radcomputer-Konvention; Stopps verfälschen Schnitt/Fahrzeit nicht; Schwelle filtert GNSS-Jitter im Stand | Gesamt-Aufzeichnungszeit als Fahrzeit/Schnitt |
| **Mindestversions-/Längenregel statt Gleichheit (`version==` → `≥`, AR-CONN-04)** | erlaubt unabhängige Weiterentwicklung von Firmware & App (v2↔v3 wechselseitig lauffähig); Vorwärtskompatibilität durch Ignorieren überzähliger Bytes | `version==` (bricht bei jedem Schema-Schritt) |
| **V3-1: kurzes v3-Frame lesen + `truncatedV3FrameCount` (getrennt vom Fehlerzähler)** | ein v3-meldendes, zu kurzes Frame ist defekt (abgeschnitten/MTU); Verwerfen vernichtet gültige v2-Nutzdaten, stilles Durchwinken versteckt den Defekt | verwerfen / stumm durchwinken |
| **V3-3: Golden-Vektor-Kreuztest gegen Firmware (nicht über eigenen Encoder)** | einziger Test, der die Schnittstelle selbst prüft; schließt gemeinsamen Denkfehler zweier symmetrischer Suiten aus | nur App-interner Round-Trip |
| **Decoder rein (Ergebnis-Enum), Zähler im Store** | hält Core UI-/zustandsfrei (Kap. 9.2); entschärft Cross-Actor-Race an der Diagnoseansicht | zustandsbehafteter Decoder mit Instanz-Zählern |
| **Gemeinsamer `TelemetryFrameEncoder` als einzige Byte-Quelle (Mock + Test), nie an BLE-Write gebunden** | keine Divergenz zwischen Mock und Test; FR-SYS-04 gewahrt | doppelter Encoder / Encoder am Write |
| **AP4: nur `bias_calibrated`/`gnss_accel_valid` live; 11 Aggregate nur Persistenz** | Statusgrößen sofort deutbar; Fensteraggregate erst in der Nachauswertung sinnvoll | alle v3-Felder live spiegeln |
| **E2 (Feldtest): 10-Hz-Validierungsmodus (Default 1 Hz), Batch-Persistenz** | Bremsdynamik 0,3–3 s braucht Sub-Sekunden-Auflösung; 1 Hz zu grob; Batching hält AR-NFR-PERF-01 | dauerhaft 10 Hz / Save-pro-Sample |
| **`temperature_c` in Persistenz aufnehmen, nur aus CSV entfernen (E4, Feldtest)** | additiv/nicht-destruktiv; „speichere, was ankommt, entscheide Export getrennt“; E4 betrifft nur den Export | aus Persistenz weglassen / im CSV behalten (leer) |
| **V3-2: CSV-`schema_version` = Minimum bei gemischten Versionen + `frame_version_gemischt`** | der schwächste Fall bestimmt, welche Spalten durchgängig gefüllt sind | Maximum (verspricht leere Spalten) |
| **V3-4: `t_s` ≥ 2 Nachkommastellen** | bei 10 Hz sonst doppelte Zeitstempel → zeitbasierte Auswertung unbrauchbar | eine Nachkommastelle |
| **Diagnosegrößen in Settings-Diagnoseansicht, nicht ins Cockpit; keine Warn-Chips daraus** | Ablenkungsarmut/kein Scrollen im Fahrbetrieb (AR-UX-01); `bias_calibrated==0` kurz nach Boot ist normal (kein Fehlalarm) | Diagnose ins Cockpit / als Warn-Chip |
| Keine Live-Karte, nur Post-Ride-Route (Should) | Ablenkung/Fix-Abhängigkeit vermeiden | Live-Karte / keine Karte |
| Integrierte Fahrrad-Navigation → V2/Future Work (Kap. 5) | Apple-MapKit ohne Radrouting; erzwingt Standortberechtigung + Live-Karte (Bruch AR-NFR-SEC-01/AR-UX-01/AR-CTX-01); Fremddienst-Abhängigkeit; Scope | im MVP integrieren / Fremd-Navigations-SDK |
| Bremsereignisse → V2 · Distanz aus speed-Integration | IMU-Hochrate nötig; robust gegen Quantisierung | Live-Auswertung / Punktdifferenzen |
| Standard-Layout „Hero + 3“ · Variante B (Should, Default zuerst) | glanceable, trifft Nutzerwunsch, abgabesicher | andere 4. Kachel / Freiform / keine |
| 3-Spalten-Raster · max ~6 Kacheln, kein Scrollen · Chips für Größe | freigegebene Optik; wahrt Ein-Blick-Prinzip; sichtbar/barrierefrei | 2-Spalten / mehr + Scrollen / Pinch |
| Diagramme Höhe + Geschwindigkeit über Distanz | aussagekräftig, vergleichbar, Standard | nur Höhe / + Temperatur / X=Zeit |
| Automatischer Reconnect · Hintergrund-Aufzeichnung | Single-User, realistischer Fahrbetrieb | manueller Scan / nur Vordergrund |
| Versionsgeprüfte Dekodierung · keine Standortberechtigung | Vorwärtskompatibilität; datensparsam | ungeprüftes Parsen / Standortfreigabe |
| Deployment neueste iOS · reine Logik voll testbar · lokal-only | neueste APIs, Nachvollziehbarkeit, Datensparsamkeit | ältere iOS / nur Kernstellen / Cloud |
| Navigation TabBar 3 Tabs · Post-Stopp-Sheet · Editor aus Live+Einstellungen | iOS-Standard, natürlicher Abschluss, auffindbar | Single-Screen / Tab-Wechsel / nur Einstellungen |
| Farbe neutral/monochrom + adaptiver Cyan · Rot nur Semantik · App folgt System · Ziffern SF Pro Rounded tabellarisch | ruhig/kontrastreich, eindeutig, stabil ablesbar | durchgehend farbig / Rot-Akzent / dauerhaft dunkel / Standard-SF-Pro |
| Ein-Blick-Prinzip · Start=Tap/Stopp=Halten · gestufte Kommunikation · Haptik · Stale abdimmen | Sicherheit & Klarheit ohne Dauerablenkung | modale Dialoge / Tap-Stopp / immer Banner / keine Haptik / Wert halten |
| **Liquid Glass nur für Chrome/Steuerelemente** | premium iOS-26-Optik, HIG-konform, Inhalte bleiben lesbar (Glanceability) | Glas auf Inhalten/überall / gar kein Glas |
| MVVM + leichte Clean-Schichten | modular, testbar, wartbar, thesis-sauber, ohne Überbau | schlankes MV / volle Clean / TCA |
| @Observable + async/await + Actors | moderne Concurrency, klar isoliert, testbar | Combine |
| Zentraler TelemetryStore | eine Live-Wahrheitsquelle, konsistent | jedes VM abonniert selbst |
| Manueller Composition Root | abhängigkeitsarm, transparent | DI-Framework |
| Hintergrund-ModelActor für Schreibvorgänge · Detailaufbau off-main | Main-Thread nie blockiert (Performance, flüssiges Öffnen der Fahrt) | Schreiben/Aufbereiten im Main-Kontext |
| UI-Update volle 10 Hz | flüssigste Anzeige (Nutzerwunsch) | Drosselung ~5 Hz |
| Mock-Datenquelle für Entwicklung · eine Umschaltstelle · Simulator immer Mock | hardwareunabhängig entwickeln/testen; Simulator hat kein BLE | erst mit echtem Gerät entwickeln |
| Debug-Seed für Recovery-Nachweis (nur DEBUG) | reproduzierbare, scriptbare Prüfprozedur ohne UI-Interaktion; thesis-taugliches Messprotokoll | manueller Crash via Tippen / kein on-screen-Nachweis |
| Ablageort `ios-app/` im Monorepo · feature-orientiert · reine Logik als Package `SmartBikeCore` | eine Historie, Zusammengehöriges beisammen, erzwungene Logik-Grenze + schnelle Host-Tests | eigenes Repo / flach / alles im App-Target |

## 13. Offene Punkte
- **Recovery-UX (AR-DATA-04):** ✔ umgesetzt & verifiziert.
- **Echte BLE-Verbindung (AR-CONN):** ✔ am realen iPhone verifiziert (Verbinden per Service-UUID, Live-Werte, Auto-Reconnect).
- **Robuste Aufzeichnung (Zeit/Distanz, `RecordingClock`):** ✔ am Gerät bestätigt (s. `docs/lessons_learned.md`).
- **Stale-/GNSS-Validität (AR-UX-05) + Höhe barometrisch:** ✔ am Gerät verifiziert.
- **System-/Sensorwarnungen (AR-LIVE-03):** ✔ vollständig — Firmware-Statusfelder → gestufte, nicht-modale Statuszeilen-Chips; reine Ableitung host-getestet.
- **Ø/Fahrzeit „bewegt“:** ✔ entschieden & umgesetzt (Schwelle 1,0 km/h; Gesamtzeit intern erhalten).
- **Frame-Schema-v3-Migration (AP0–AP8):** ✔ abgeschlossen, committet, 89 Tests grün, gegen Firmware-Golden-Vektor kreuzvalidiert (Decoder, Mock, Store-Split, Persistenz+Migration, 10-Hz-Modus, 35-Spalten-CSV, Diagnoseansicht).
- **Serial-Bench-Validierung (Bremskennlinie @100 Hz):** firmwareseitiges Log für die **Sub-Sekunden-Feindynamik** (300-ms-Halten/Hysterese/<50-ms-Anstieg) — ergänzt den App-Export; auf der Firmware-Seite (s. `docs/roadmap.md` M7). *(Der 10-Hz-App-Modus deckt die Feindynamik nun teilweise selbst ab.)*
- **Reale Feldvalidierung der App-Daten** (gegen Strava; Leitfaden liegt vor; 10-Hz-Modus für Bremsdynamik nutzen): ausstehend.
- **Golden-Vektor-Provenienz:** sicherstellen, dass `frame_v3_golden.md` den **Firmware-Git-Hash** vermerkt (Nachweis, gegen welchen Firmware-Stand kreuzvalidiert wurde).
- **Cockpit-Editor (AR-LIVE-08):** offen — nächste App-Scheibe.
- **Post-Ride-Routenkarte (Should)** und **absolute Höhe** hängen am noch offenen **GNSS-Fix im Freiland** — ohne Fix → Route V2; barometrische Höhe relativ korrekt, Absolutwert wetterabhängig.
- **Optionale Höhen-Referenzdruck-Kalibrierung** (Absolut-Höhe) — Future.
- **Finale SF-Symbol-Wahl** je Screen — bewusst ans Ende gelegt.

## 14. Verhältnis zur Project Bible
Project Bible = Gesamtsystem-SSOT. App Bible = App-SSOT. Schnittstellenvertrag v3 = `claude/BLE_Frame_v3_Schnittstelle.md`. App-Entscheidungen mit Systemwirkung zusätzlich als 📖 PROJECT-BIBLE-RELEVANT melden; Project Bible nur mit ausdrücklicher Freigabe ändern.
**Erledigt (Project Bible v0.13):** Ablageort des iOS-App-Codes = `ios-app/` im Monorepo (Kap. 9.7) sowie der aktuelle App-Stand sind in die Project Bible übernommen.
**Erledigt (Project Bible v0.14):** BLE-Frame-Schema v2 (`brake_light_pct`, 81 Byte) firmwareseitig umgesetzt und in Kap. 10 nachgezogen.
**Offen für Project Bible (auf Freigabe):** BLE-Vertrag Kap. 10 auf **v3 (113 Byte)** nachziehen; App-Stand „Schema-v3-Migration abgeschlossen + Golden-Vektor-Kreuztest“ (Validierungstabelle Kap. 9); Verweis auf `docs/CSV_Format_v3_Validierungsexport.md` als Format-Referenz der Gesamtsystem-Validierung; App-seitiger **10-Hz-Validierungsmodus** als zusätzliche Datenquelle.