import Foundation
import Observation
import SmartBikeCore

/// Logik des Cockpit-Editors (AR-LIVE-08). Verschieben/Größe/Metrik je Kachel;
/// Constraints/Packing rein in `DashboardLayoutStore`/`GridPacker`. Edits sind
/// nicht-destruktiv: Änderungen, die das Raster sprengen, werden verworfen.
@MainActor @Observable
final class EditorViewModel {
    private let layoutStore = DashboardLayoutStore()
    private(set) var layout: DashboardLayout
    var selectedTileID: UUID?

    init(layout: DashboardLayout = .standard) {
        self.layout = layoutStore.sanitized(layout)
    }

    /// Platzierungen (Spalte/Zeile/Größe) fürs Rendern.
    var placements: [TilePlacement] { GridPacker.placements(for: layout.tiles) }
    var selectedTile: TileConfig? { layout.tiles.first { $0.id == selectedTileID } }

    /// Kapazität frei für eine weitere (1×1-)Kachel?
    var canAdd: Bool {
        guard layout.tiles.count < DashboardLayout.maxTiles else { return false }
        var candidate = layout
        candidate.tiles.append(TileConfig(metric: nextMetric(), size: .s1x1, position: layout.tiles.count))
        return layoutStore.fits(candidate)
    }

    func select(_ id: UUID?) { selectedTileID = id }

    /// Größe ändern; nur übernehmen, wenn das Layout weiterhin passt (kein Verwerfen).
    func setSize(_ size: TileSize, for id: UUID) {
        apply { if let i = $0.tiles.firstIndex(where: { $0.id == id }) { $0.tiles[i].size = size } }
    }
    func canSetSize(_ size: TileSize, for id: UUID) -> Bool {
        var candidate = layout
        guard let i = candidate.tiles.firstIndex(where: { $0.id == id }) else { return false }
        candidate.tiles[i].size = size
        return layoutStore.fits(candidate)
    }

    func setMetric(_ metric: MetricID, for id: UUID) {
        if let i = layout.tiles.firstIndex(where: { $0.id == id }) { layout.tiles[i].metric = metric }
    }

    func remove(_ id: UUID) {
        layout.tiles.removeAll { $0.id == id }
        if selectedTileID == id { selectedTileID = nil }
        renumber()
    }

    func add() {
        guard canAdd else { return }
        let tile = TileConfig(metric: nextMetric(), size: .s1x1, position: layout.tiles.count)
        layout.tiles.append(tile)
        selectedTileID = tile.id
    }

    /// Kachel in der Anordnung verschieben (Index in Positionsreihenfolge).
    func move(from source: Int, to destination: Int) {
        var ordered = layout.tiles.sorted { $0.position < $1.position }
        guard source >= 0, source < ordered.count else { return }
        let item = ordered.remove(at: source)
        ordered.insert(item, at: max(0, min(destination, ordered.count)))
        layout.tiles = ordered
        renumber()
    }

    func index(ofTileID uuidString: String) -> Int? {
        layout.tiles.sorted { $0.position < $1.position }.firstIndex { $0.id.uuidString == uuidString }
    }

    func resetToStandard() { layout = .standard; selectedTileID = nil }

    func commit() -> DashboardLayout { layoutStore.sanitized(layout) }

    // MARK: - Intern

    private func apply(_ mutate: (inout DashboardLayout) -> Void) {
        var candidate = layout
        mutate(&candidate)
        if layoutStore.fits(candidate) { layout = candidate }
    }
    private func renumber() { for i in layout.tiles.indices { layout.tiles[i].position = i } }
    private func nextMetric() -> MetricID {
        let used = Set(layout.tiles.map(\.metric))
        return MetricID.allCases.first { !used.contains($0) } ?? .speed
    }
}
