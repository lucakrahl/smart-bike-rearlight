# Info.plist & Capabilities (in Xcode setzen)

- **NSBluetoothAlwaysUsageDescription** = z. B. „Die App verbindet sich per Bluetooth
  mit deinem Rücklicht, um Fahrtdaten anzuzeigen und aufzuzeichnen."
- **Background Modes** → „Uses Bluetooth LE accessories" (`bluetooth-central`) — AR-CONN-03.
- **Keine** Standortberechtigung (BLE-Central braucht sie auf iOS nicht).
- Deployment Target: neueste iOS-Version. Signing: Personal Team (7-Tage-Profil).
- AccentColor: als Farbsatz in `Assets.xcassets` anlegen — Dunkel `#22D3EE`, Hell `#0E7490`.
