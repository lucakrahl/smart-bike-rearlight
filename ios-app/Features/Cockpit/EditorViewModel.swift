import Foundation
import Observation
import SmartBikeCore

/// Logik des Cockpit-Editors (AR-LIVE-08). Verschieben/Größe/Metrik je Kachel,
/// Constraints via DashboardLayoutStore.
@MainActor @Observable
final class EditorViewModel {
    private let layoutStore = DashboardLayoutStore()
    var layout: DashboardLayout

    init(layout: DashboardLayout = .standard) { self.layout = layout }

    func resetToStandard() { layout = .standard }
    func commit() -> DashboardLayout { layoutStore.sanitized(layout) }
    // TODO: move(tile:to:), setSize(_:for:), setMetric(_:for:), add(), remove(_:)
}
