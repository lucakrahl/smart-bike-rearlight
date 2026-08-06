import Testing
@testable import SmartBikeCore

/// Packing/Constraints des Cockpit-Layouts (AR-LIVE-08).
struct DashboardLayoutTests {
    private let store = DashboardLayoutStore()

    private func tiles(_ specs: [(MetricID, TileSize)]) -> DashboardLayout {
        DashboardLayout(tiles: specs.enumerated().map { i, s in
            TileConfig(metric: s.0, size: s.1, position: i)
        })
    }

    @Test func standardLayoutIsValidAndUnchanged() {
        let s = store.sanitized(.standard)
        #expect(s == .standard)
        #expect(store.fits(.standard))
        let places = GridPacker.placements(for: DashboardLayout.standard.tiles)
        #expect(places.count == 4)
        // Hero 3×1 oben, drei 1×1 in der zweiten Zeile.
        #expect(places[0] == TilePlacement(tile: places[0].tile, column: 0, row: 0, width: 3, height: 1))
        #expect(places[1].row == 1 && places[1].column == 0)
        #expect(places[3].row == 1 && places[3].column == 2)
    }

    @Test func addUntilCapacityThenReject() {
        // 6 × 1×1 passen (2 Zeilen); Nr. 7 wird abgelehnt.
        let six = tiles(Array(repeating: (MetricID.sats, .s1x1), count: 6))
        #expect(store.fits(six))
        #expect(store.sanitized(six).tiles.count == 6)

        let seven = tiles(Array(repeating: (MetricID.sats, .s1x1), count: 7))
        #expect(!store.fits(seven))                     // Überkapazität
        #expect(store.sanitized(seven).tiles.count == 6) // auf Maximum gekappt
    }

    @Test func resizeRepacksWithoutOverlap() {
        // Standard, aber Hero als 2×2 statt 3×1 → muss überlappungsfrei umpacken.
        let layout = tiles([(.speed, .s2x2), (.distance, .s1x1), (.duration, .s1x1), (.avgSpeed, .s1x1)])
        #expect(store.fits(layout))
        let p = GridPacker.placements(for: layout.tiles)
        #expect(p.count == 4)
        #expect(p[0].column == 0 && p[0].row == 0 && p[0].width == 2 && p[0].height == 2)
        // keine zwei Kacheln teilen sich eine Zelle
        var seen = Set<String>()
        for pl in p {
            for c in pl.column..<(pl.column + pl.width) {
                for r in pl.row..<(pl.row + pl.height) {
                    #expect(seen.insert("\(c),\(r)").inserted)
                }
            }
        }
    }

    @Test func tooWideTileIsRejected() {
        // 2×2 an Spalte, die nur mit 3×1 danebenpasst — hier: fünf 2×2 sprengen die 4 Zeilen.
        let many = tiles(Array(repeating: (MetricID.speed, .s2x2), count: 5))
        #expect(!store.fits(many))
    }

    @Test func resetRestoresStandard() {
        let custom = tiles([(.maxSpeed, .s2x1), (.hdop, .s1x1)])
        _ = store.sanitized(custom)
        #expect(store.sanitized(.standard) == .standard)
    }

    @Test func movePreservesTilesAndNormalizesPositions() {
        // Reihenfolge geändert (avgSpeed nach vorn) → Positionen 0…n neu vergeben, nichts verloren.
        let moved = DashboardLayout(tiles: [
            TileConfig(metric: .avgSpeed, size: .s1x1, position: 0),
            TileConfig(metric: .speed,    size: .s3x1, position: 1),
            TileConfig(metric: .distance, size: .s1x1, position: 2),
            TileConfig(metric: .duration, size: .s1x1, position: 3),
        ])
        let s = store.sanitized(moved)
        #expect(s.tiles.count == 4)
        #expect(s.tiles.map(\.position) == [0, 1, 2, 3])
        #expect(s.tiles.first?.metric == .avgSpeed)
    }
}
