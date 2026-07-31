import Foundation

/// Eine Cockpit-Kachel (AR-LIVE-08). Persistiert (App Bible Kap. 11).
public struct TileConfig: Codable, Sendable, Equatable, Identifiable {
    public var id: UUID
    public var metric: MetricID
    public var size: TileSize
    public var position: Int
    public init(id: UUID = UUID(), metric: MetricID, size: TileSize, position: Int) {
        self.id = id; self.metric = metric; self.size = size; self.position = position
    }
}

/// Cockpit-Layout (AR-LIVE-08). 3-Spalten-Raster, max ~6 Kacheln, kein Scrollen.
public struct DashboardLayout: Codable, Sendable, Equatable {
    public static let maxTiles = 6
    public static let columns = 3
    public var tiles: [TileConfig]
    public init(tiles: [TileConfig]) { self.tiles = tiles }

    /// Standard-Layout „Hero + 3" (AR-LIVE-01).
    public static let standard = DashboardLayout(tiles: [
        TileConfig(metric: .speed,    size: .s3x1, position: 0),
        TileConfig(metric: .distance, size: .s1x1, position: 1),
        TileConfig(metric: .duration, size: .s1x1, position: 2),
        TileConfig(metric: .avgSpeed, size: .s1x1, position: 3),
    ])
}
