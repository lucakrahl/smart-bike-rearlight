# Smart Bike Rear Light — iOS-App (Bootstrap)

Feature-orientiertes SwiftUI-Projekt mit lokalem Swift-Package `SmartBikeCore`
für die reine, host-testbare Logik. Struktur & Entscheidungen: App Bible Kap. 9.7.

## Einrichten in Xcode (einmalig)
1. **Neues Projekt:** Xcode → iOS App, Name `SmartBikeRearLight`, Interface *SwiftUI*,
   Language *Swift*, Storage *SwiftData* (oder ohne — Store wird manuell gesetzt).
   Projekt in diesen Ordner `ios-app/` legen.
2. **Local Package hinzufügen:** File → Add Package Dependencies… → *Add Local…* →
   Ordner `Core/` wählen. Anschließend Target „SmartBikeRearLight" → *Frameworks,
   Libraries* → `SmartBikeCore` hinzufügen.
3. **Quellen einbinden:** Ordner `App/`, `Features/`, `Services/`, `DesignSystem/`,
   `Resources/` in die Projekt-Navigation ziehen (*Create groups*). Die von Xcode
   automatisch angelegte `ContentView.swift`/App-Datei durch die hiesige
   `App/SmartBikeRearLightApp.swift` ersetzen.
4. **Capabilities/Info.plist:** siehe `Resources/Info-Setup.md`
   (Bluetooth-Beschreibung, Background Mode `bluetooth-central`, AccentColor-Asset).
5. **Signing:** Personal Team, „Automatically manage signing".

## Tests
- Core (Mac, schnell): im Ordner `Core/` → `swift test`, oder in Xcode Cmd-U.

## Weiterbauen
Der Implementierungsleitfaden steht in `CLAUDE.md`. Reihenfolge-Empfehlung:
Persistenz (`SwiftDataStore` + Mapping) → BLE (`BLEConnectionService`) →
Decode-Consumer + `TelemetryStore` → Cockpit (Standard-Layout zuerst) →
Verlauf/Detail → Editor → Feinschliff (Haptik, Stale, Diagramme, Route).
