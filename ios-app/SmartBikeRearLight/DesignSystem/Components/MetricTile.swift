import SwiftUI
import SmartBikeCore

/// Wiederverwendbare Cockpit-Kachel. Zeigt Label + Wert (+ Einheit) glanceable.
/// `valueSize` steuert die Zifferngröße (Hero groß, Sekundärkacheln kompakt).
/// TODO: Größenvarianten (1x1/2x1/3x1/2x2), Stale-Abdimmung (AR-UX-05).
struct MetricTile: View {
    let display: MetricDisplay
    var isStale: Bool = false
    var valueSize: CGFloat = 40
    var body: some View {
        VStack(spacing: 4) {
            Text(display.label)
                .font(.caption).foregroundStyle(.secondary)
                .lineLimit(1).minimumScaleFactor(0.7)
            HStack(alignment: .firstTextBaseline, spacing: 4) {
                // Tabellarische Ziffern; bei Bedarf skalieren statt umbrechen (z. B. 00:00:00).
                Text(display.value)
                    .font(Theme.numeric(valueSize))
                    .lineLimit(1).minimumScaleFactor(0.5)
                if !display.unit.isEmpty {
                    Text(display.unit).font(.subheadline).foregroundStyle(.secondary)
                        .lineLimit(1)
                }
            }
        }
        .opacity(isStale ? 0.4 : 1.0)
        .padding()
        .frame(maxWidth: .infinity)
        .background(.thinMaterial, in: RoundedRectangle(cornerRadius: Theme.tileCornerRadius))
    }
}
