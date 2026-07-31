import SwiftUI
import SmartBikeCore

/// Wiederverwendbare Cockpit-Kachel. Zeigt Label + Wert (+ Einheit) glanceable.
/// TODO: Größenvarianten (1x1/2x1/3x1/2x2), Stale-Abdimmung (AR-UX-05).
struct MetricTile: View {
    let display: MetricDisplay
    var isStale: Bool = false
    var body: some View {
        VStack(spacing: 4) {
            Text(display.label).font(.caption).foregroundStyle(.secondary)
            HStack(alignment: .firstTextBaseline, spacing: 4) {
                Text(display.value).font(Theme.numeric(40))
                if !display.unit.isEmpty { Text(display.unit).font(.subheadline).foregroundStyle(.secondary) }
            }
        }
        .opacity(isStale ? 0.4 : 1.0)
        .padding()
        .frame(maxWidth: .infinity)
        .background(.thinMaterial, in: RoundedRectangle(cornerRadius: Theme.tileCornerRadius))
    }
}
