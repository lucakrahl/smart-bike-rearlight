import SwiftUI
import SmartBikeCore

/// Einstellungen (App Bible Kap. 7): Gerät · Cockpit · Anzeige · Info (Daten lokal).
struct SettingsView: View {
    @Environment(AppEnvironment.self) private var env
    @State private var showEditor = false

    private var isRecording: Bool { env.rideManager.state != .idle }

    var body: some View {
        NavigationStack {
            Form {
                Section("Gerät") {
                    LabeledContent("Gerät", value: "SmartBikeRearLight")
                    LabeledContent("Verbindung", value: env.telemetryStore.connection.display.label)
                }

                Section {
                    Button {
                        showEditor = true
                    } label: {
                        Label("Cockpit bearbeiten", systemImage: "square.grid.2x2")
                    }
                    .disabled(isRecording)   // AR-LIVE-08: nur außerhalb der Aufzeichnung

                    Button(role: .destructive) {
                        env.resetLayout()
                    } label: {
                        Label("Auf Standard zurücksetzen", systemImage: "arrow.counterclockwise")
                    }
                    .disabled(isRecording)
                } header: {
                    Text("Cockpit")
                } footer: {
                    if isRecording { Text("Während der Aufzeichnung gesperrt.") }
                }

                Section {
                    Picker("Aufzeichnungsrate", selection: Binding(
                        get: { env.rideManager.mode },
                        set: { _ = env.setRecordingMode($0) }
                    )) {
                        Text("1 Hz · Standard").tag(RecordingMode.hz1)
                        Text("10 Hz · Validierung").tag(RecordingMode.hz10)
                    }
                    .pickerStyle(.segmented)
                    .disabled(isRecording)   // Umschalten nur außerhalb der Aufzeichnung (AP6)
                } header: {
                    Text("Aufzeichnung")
                } footer: {
                    // Nicht-modaler Hinweis (AR-UX-01): kein Alert, nur Fußtext.
                    if isRecording {
                        Text("Während der Aufzeichnung gesperrt.")
                    } else if env.rideManager.mode == .hz10 {
                        Text("10 Hz erhöht den Speicherbedarf deutlich (≈ 10× mehr Datenpunkte pro Fahrt).")
                    } else {
                        Text("10 Hz liefert eine höhere Auflösung für Validierungsfahrten – auf Kosten von mehr Speicher.")
                    }
                }

                Section("Anzeige") {
                    LabeledContent("Einheiten", value: "Metrisch")
                }

                Section("Info") {
                    LabeledContent("Version", value: SettingsViewModel.appVersion)
                    LabeledContent("Datenschutz", value: "Alle Daten lokal")
                }
            }
            .navigationTitle("Einstellungen")
            .sheet(isPresented: $showEditor) { CockpitEditorView() }
        }
    }
}
