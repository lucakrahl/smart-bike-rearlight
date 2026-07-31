import SwiftUI
import SmartBikeCore

/// Statuszeile (AR-LIVE-02/03, AR-UX-03): BLE · Fix · Sats; Warnung nur im Problemfall.
struct StatusBar: View {
    let connection: ConnectionState
    let fix: GnssFixStatus
    let sats: Int
    var body: some View {
        HStack(spacing: 12) {
            Label(connection == .connected ? "Verbunden" : "Getrennt",
                  systemImage: "dot.radiowaves.left.and.right")
                .foregroundStyle(connection == .connected ? Theme.Semantic.ok : .secondary)
            Label("\(sats) Sats", systemImage: "location.fill")
                .foregroundStyle(fix == .fixOK ? Theme.Semantic.ok : Theme.Semantic.searching)
        }
        .font(.caption)
    }
}
