# CLAUDE.md — iOS-App „Smart Bike Rear Light"

Leitfaden für Claude Code (Xcode) zur Implementierung dieser App. **Verbindliche
Wissensbasis ist die App Bible** (Projekt „Bachelorarbeit", `claude/app_bible.md`).
Dieses Dokument fasst die harten Regeln zusammen; im Zweifel gilt die App Bible.

## Zweck
Nativer iOS-Begleiter eines smarten Fahrrad-Rücklichts (ESP32). Die App ist reine
**Datensenke**: sie empfängt per BLE Telemetrie, zeigt sie im Live-Cockpit an,
zeichnet Fahrten lokal auf und wertet sie danach aus. Single-User, deutschsprachig,
metrisch, alle Daten lokal (kein Netzwerk/Account).

## Unveränderlicher BLE-Vertrag (NICHT ändern — Firmware ist fix, FR-SYS-04)
- Geräte-Name `SmartBikeRearLight`
- Service-UUID `587bb505-9f9d-4ae0-96fd-0b29adfc4b03`
- Characteristic (NOTIFY) `8c604d09-743f-4850-9109-19604a17f358` — nur NOTIFY, kein Write
- Frame: **80 Byte, Little-Endian, gepackt, 10 Hz**, Schema-`version` == 1 (Offset 0)
- Unidirektional ESP32 → App. Offsets/Feldliste: siehe `SmartBikeCore` + App Bible Kap. 10.

## Architektur (App Bible Kap. 9) — Regeln
- **MVVM + leichte Clean-Schichten**, alles über **Protokolle** entkoppelt.
- **Nebenläufigkeit:** `@Observable` + async/await + Actors. BLE/Dekodierung im Actor
  abseits des Main-Threads; `TelemetryStore`/ViewModels `@MainActor`; SwiftData-Writes
  im Hintergrund-`ModelActor`. Reine Engines synchron.
- **Zentraler `TelemetryStore`** ist die einzige Live-Wahrheitsquelle.
- **Composition Root** (`AppEnvironment`) verdrahtet alles manuell — kein DI-Framework.
- **Datenfluss:** `BLEConnectionService → TelemetryFrameDecoder → TelemetryStore →
  ViewModels → Views`. Aufzeichnung: `RideManager` verdichtet auf **1 Hz** →
  `RideRepository` (Hintergrund) + `StatisticsEngine`.

### Wohin gehört was
- `Core/` = **reine, UI-freie Logik** (Swift Package `SmartBikeCore`): Modelle, Decoder,
  Statistik, Metrik-Registry, Layout. **Kein** SwiftUI/CoreBluetooth/SwiftData hier.
- `Services/BLE`, `Services/Persistence`, `Services/Live` = plattformabhängig (App-Target).
- `Features/<Screen>/` = View + ViewModel beisammen.
- `DesignSystem/` = Farben/Typografie/Abstände/Bausteine.

## UX-Regeln (App Bible Kap. 6.7/8) — verbindlich
- **Keine modalen Alerts/Dialoge während der Fahrt** (AR-UX-01). Cockpit ohne Scrollen.
- **Start = Tap, Stopp = Halten (~1 s)** mit Fortschrittsring (AR-UX-02).
- **Gestufte** Status-/Fehlerkommunikation: normal dezent in der Statuszeile, kritisch
  als nicht-modaler Banner (AR-UX-03).
- **Stale/getrennt: Werte abdimmen + kennzeichnen**, fehlende als „—" (AR-UX-05).
- Dezente **Haptik** bei Start/Stopp und Verbindungswechsel (AR-UX-04).

## Design (UX-A)
- SF Pro + **Dynamic Type**; **SF Symbols**; 8-pt-Raster; Kachelradius ~16; Tap-Target ≥ 44.
- App folgt System-Hell/Dunkel. Akzent **AccentColor** adaptiv (Dunkel `#22D3EE`, Hell `#0E7490`).
- Semantik: Rot = Warnung/Bremse/Fehler · Grün = Fix ok · Amber = Suche/kein Fix.
- Cockpit-Ziffern: **SF Pro Rounded, tabellarische Ziffern** (`Theme.numeric`).
- Diagramme (Swift Charts): Geschwindigkeit = Cyan, Höhe = Slate; X-Achse = Distanz.

## Konventionen
- UI-Strings **Deutsch**; Lokalisierung vorbereiten. Einheiten metrisch.
- Kommentare erklären **WARUM** (nicht nur WAS). AR-IDs referenzieren.
- Reine Logik bleibt in `SmartBikeCore` und ist unit-getestet (AR-NFR-TST-01).

## Nicht tun
- Firmware/BLE-Vertrag ändern. · SwiftUI/SwiftData in `SmartBikeCore` ziehen.
- Standortberechtigung anfordern. · Distanz aus lat/lon-Punktdifferenzen (nutze
  Geschwindigkeitsintegration — `StatisticsEngine`).

## Tests
- Core: `swift test` im Ordner `Core/` (läuft auf dem Mac, ohne Simulator).
- App: ViewModels gegen Mock-Stores, `RideManager` gegen Mock-`RideRepository`.

## Aktueller Stand (Bootstrap)
- **Implementiert + getestet:** `TelemetryFrameDecoder`, `StatisticsEngine`, Modelle,
  Enums, `MetricRegistry`, `DashboardLayoutStore` (+ Unit-Tests).
- **Stubs (TODO):** BLE, SwiftData-Persistenz, TelemetryStore/RideManager-Verdrahtung,
  alle Views/ViewModels, Composition Root `live()`. Siehe `// TODO`-Marker.
