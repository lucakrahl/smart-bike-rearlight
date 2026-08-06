import SwiftUI
import SmartBikeCore

/// Statuszeile (AR-LIVE-02/03, AR-UX-03): BLE · Fix · Sats; Warnung nur im Problemfall.
/// Verbindungsanzeige gestuft (Getrennt/Suchend/Verbindend/Verbunden/Bluetooth aus/
/// Keine Berechtigung) — Text/Stufe aus `ConnectionState.display` (SmartBikeCore).
struct StatusBar: View {
    let connection: ConnectionState
    let fix: GnssFixStatus
    let sats: Int
    /// System-/Sensorwarnungen (höchste Severity zuerst). Kompakt angezeigt (AR-LIVE-03).
    var warnings: [LiveWarning] = []
    var body: some View {
        let status = connection.display
        let gnssValid = GnssValidity.isValid(fix: fix, sats: sats)
        HStack(spacing: 12) {
            Label(status.label, systemImage: Self.symbol(for: status.severity))
                .foregroundStyle(Self.color(for: status.severity))
            // GNSS: „N Sats" (grün) bei gültigem Fix, sonst amber „Kein Fix" (AR-LIVE-03).
            Label(gnssValid ? "\(sats) Sats" : "Kein Fix", systemImage: "location.fill")
                .foregroundStyle(gnssValid ? Theme.Semantic.ok : Theme.Semantic.searching)
            // Höchste System-/Sensorwarnung kompakt (Rot=Warnung, Amber=Info).
            if let top = warnings.first {
                Label(top.text, systemImage: Self.warningSymbol(for: top.severity))
                    .foregroundStyle(top.severity == .warnung ? Theme.Semantic.warning : Theme.Semantic.searching)
                    .lineLimit(1)
            }
        }
        .font(.caption)
        .padding(.horizontal, 14)
        .padding(.vertical, 8)
        .floatingGlass(in: .capsule)   // schwebende Glas-Pille (Chrome), nicht interaktiv
    }

    private static func color(for severity: ConnectionSeverity) -> Color {
        switch severity {
        case .connected: return Theme.Semantic.ok
        case .searching: return Theme.Semantic.searching
        case .warning:   return Theme.Semantic.warning
        case .neutral:   return .secondary
        }
    }

    private static func symbol(for severity: ConnectionSeverity) -> String {
        severity == .warning ? "exclamationmark.triangle.fill" : "dot.radiowaves.left.and.right"
    }

    private static func warningSymbol(for severity: LiveWarning.Severity) -> String {
        severity == .warnung ? "exclamationmark.triangle.fill" : "info.circle"
    }
}
