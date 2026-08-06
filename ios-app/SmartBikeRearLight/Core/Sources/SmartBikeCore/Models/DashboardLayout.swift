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

public extension TileSize {
    /// Belegung im Raster (Spalten × Zeilen). Breite ≤ 3 (3-Spalten-Raster).
    var span: (columns: Int, rows: Int) {
        switch self {
        case .s1x1: return (1, 1)
        case .s2x1: return (2, 1)
        case .s3x1: return (3, 1)
        case .s2x2: return (2, 2)
        }
    }
}

/// Platzierung einer Kachel im Raster (aus dem Packing berechnet, nicht persistiert).
public struct TilePlacement: Sendable, Equatable {
    public var tile: TileConfig
    public var column: Int   // 0-basiert, links
    public var row: Int      // 0-basiert, oben
    public var width: Int    // Spalten
    public var height: Int   // Zeilen
    public init(tile: TileConfig, column: Int, row: Int, width: Int, height: Int) {
        self.tile = tile; self.column = column; self.row = row; self.width = width; self.height = height
    }
}

/// Cockpit-Layout (AR-LIVE-08). 3-Spalten-Raster, max ~6 Kacheln, kein Scrollen.
public struct DashboardLayout: Codable, Sendable, Equatable {
    public static let maxTiles = 6
    public static let columns = 3
    /// Höchste Zeilenzahl ohne Scrollen (AR-UX-01).
    public static let rows = 4
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
