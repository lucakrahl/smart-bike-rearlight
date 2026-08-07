import SwiftUI
import SmartBikeCore

/// Read-only Diagnose (AP8) unter Einstellungen: v3-Innensicht, Zeitbudget des letzten
/// Frames und E-1-Defektindikator. Bewusst NICHT im Cockpit, kein modaler Alert, kein
/// Scrollzwang im Cockpit (AR-UX-01). Reine Anzeige — kein Schreibpfad zum Gerät.
struct DiagnosticsView: View {
    @Environment(AppEnvironment.self) private var env

    var body: some View {
        // Ableitung im ViewModel; Zugriffe auf den @Observable-Store werden hier getrackt.
        let r = DiagnosticsViewModel(store: env.telemetryStore).readout
        Form {
            Section("v3-Status") {
                statusRow("bias_calibrated", ok: r.biasCalibrated)
                statusRow("gnss_accel_valid", ok: r.gnssAccelValid)
            }

            Section("Zeitbudget · letztes Frame") {
                LabeledContent("dt_max_ms", value: r.dtMaxDisplay)
                LabeledContent("loop_max_us", value: r.loopMaxDisplay)
            }

            Section {
                LabeledContent("truncatedV3FrameCount", value: "\(r.truncatedV3FrameCount)")
            } header: {
                Text("Defekt-Indikator (E-1)")
            } footer: {
                // Nicht-modaler Hinweis (AR-UX-01): Fußtext statt Alert.
                Label(r.hasTruncatedDefect
                      ? "Zu kurze v3-Frames empfangen — Firmware/Transport prüfen."
                      : "Keine zu kurzen v3-Frames.",
                      systemImage: r.hasTruncatedDefect ? "exclamationmark.triangle.fill" : "checkmark.circle")
                    .foregroundStyle(r.hasTruncatedDefect ? Theme.Semantic.warning : Theme.Semantic.ok)
            }
        }
        .navigationTitle("Diagnose")
    }

    private func statusRow(_ label: String, ok: Bool) -> some View {
        LabeledContent(label) {
            Label(ok ? "ja" : "nein", systemImage: ok ? "checkmark.circle.fill" : "circle")
                .labelStyle(.titleAndIcon)
                .foregroundStyle(ok ? Theme.Semantic.ok : .secondary)
        }
    }
}
