import Foundation

/// Erzwingt die Cockpit-Constraints (AR-LIVE-08): Höchstzahl Kacheln, gültige
/// Positionen. Rein & host-testbar. Persistenz (Codable) übernimmt das App-Target.
public struct DashboardLayoutStore: Sendable {
    public init() {}

    public func sanitized(_ layout: DashboardLayout) -> DashboardLayout {
        var tiles = Array(layout.tiles.prefix(DashboardLayout.maxTiles))
        if tiles.isEmpty { tiles = DashboardLayout.standard.tiles }
        for i in tiles.indices { tiles[i].position = i }
        return DashboardLayout(tiles: tiles)
    }
}
