import Foundation

/// Reines First-Fit-Packing variabel großer Kacheln im 3-Spalten-Raster (AR-LIVE-08).
/// Host-testbar. Reihenfolge der Kacheln = Anordnung; Positionen ergeben sich aus dem Packing.
public enum GridPacker {
    public static let columns = DashboardLayout.columns   // 3
    public static let maxRows = DashboardLayout.rows       // 4

    /// Platziert die Kacheln in Reihenfolge; überspringt, was nicht (mehr) passt
    /// (ungültige Breite oder Überkapazität) → Ergebnis ist überlappungsfrei.
    public static func placements(for tiles: [TileConfig]) -> [TilePlacement] {
        var occupied = Set<Cell>()
        var result: [TilePlacement] = []
        for tile in tiles {
            let (w, h) = tile.size.span
            guard w >= 1, w <= columns, h >= 1, h <= maxRows else { continue }
            guard let slot = firstFit(width: w, height: h, occupied: occupied) else { continue }
            for dc in 0..<w { for dr in 0..<h { occupied.insert(Cell(col: slot.col + dc, row: slot.row + dr)) } }
            result.append(TilePlacement(tile: tile, column: slot.col, row: slot.row, width: w, height: h))
        }
        return result
    }

    private struct Cell: Hashable { let col: Int; let row: Int }

    private static func firstFit(width w: Int, height h: Int,
                                 occupied: Set<Cell>) -> (col: Int, row: Int)? {
        guard h <= maxRows else { return nil }
        for row in 0...(maxRows - h) {
            for col in 0...(columns - w) {
                var free = true
                search: for dc in 0..<w {
                    for dr in 0..<h where occupied.contains(Cell(col: col + dc, row: row + dr)) {
                        free = false; break search
                    }
                }
                if free { return (col, row) }
            }
        }
        return nil
    }
}

/// Erzwingt die Cockpit-Constraints (AR-LIVE-08): Höchstzahl Kacheln, gültige Größen,
/// überlappungsfreies Packing, normalisierte Positionen. Rein & host-testbar.
public struct DashboardLayoutStore: Sendable {
    public init() {}

    /// Bereinigt ein (evtl. ungültiges) Layout zu einem gültigen: nach `position` sortiert,
    /// auf `maxTiles` gekappt, gepackt (Überlauf/ungültige Größe verworfen), Positionen 0…n.
    /// Leeres Ergebnis → Standard.
    public func sanitized(_ layout: DashboardLayout) -> DashboardLayout {
        let ordered = layout.tiles.sorted { $0.position < $1.position }
        let capped = Array(ordered.prefix(DashboardLayout.maxTiles))
        var kept = GridPacker.placements(for: capped).map(\.tile)
        if kept.isEmpty { kept = DashboardLayout.standard.tiles }
        for i in kept.indices { kept[i].position = i }
        return DashboardLayout(tiles: kept)
    }

    /// Passt das Layout unverändert (nichts verworfen)? Für nicht-destruktive Editor-Edits.
    public func fits(_ layout: DashboardLayout) -> Bool {
        guard layout.tiles.count <= DashboardLayout.maxTiles else { return false }
        return GridPacker.placements(for: layout.tiles).count == layout.tiles.count
    }
}
