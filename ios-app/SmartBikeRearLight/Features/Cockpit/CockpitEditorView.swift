import SwiftUI
import SmartBikeCore

/// Cockpit-Editor als Sheet (App Bible 6.4, UX-D, AR-LIVE-08). 3-Spalten-Raster mit
/// Live-Werten, Tap → Größen-Chips + Metrik-Wähler, Halten & Ziehen zum Verschieben,
/// „+ Kachel" / „Auf Standard zurücksetzen" / „Fertig". Nicht-modal editiert.
struct CockpitEditorView: View {
    @Environment(AppEnvironment.self) private var env
    @Environment(\.dismiss) private var dismiss
    @State private var editor = EditorViewModel()
    @State private var didLoad = false
    private let registry = MetricRegistry()

    var body: some View {
        NavigationStack {
            VStack(spacing: Theme.Spacing.unit * 2) {
                // Live-Vorschau (echte Werte aus dem TelemetryStore).
                DashboardGridView(placements: editor.placements) { p in
                    editorTile(p)
                }
                .frame(height: 300)

                if let selected = editor.selectedTile {
                    tileControls(selected)
                } else {
                    Text("Kachel antippen zum Bearbeiten · halten & ziehen zum Verschieben")
                        .font(.caption).foregroundStyle(.secondary)
                        .frame(maxWidth: .infinity)
                }

                HStack {
                    Button { editor.add() } label: { Label("Kachel", systemImage: "plus") }
                        .disabled(!editor.canAdd)
                    Spacer()
                    Button { withAnimation { editor.resetToStandard() } } label: {
                        Label("Auf Standard zurücksetzen", systemImage: "arrow.counterclockwise")
                    }
                    .foregroundStyle(Theme.Semantic.warning)
                }
                .font(.subheadline)

                Spacer(minLength: 0)
            }
            .padding()
            .navigationTitle("Cockpit bearbeiten")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Abbrechen") { dismiss() }
                }
                ToolbarItem(placement: .confirmationAction) {
                    Button("Fertig") { env.updateLayout(editor.commit()); dismiss() }
                }
            }
            .task {
                if !didLoad { editor = EditorViewModel(layout: env.dashboardLayout); didLoad = true }
            }
        }
    }

    // MARK: - Kachel im Editor

    private func editorTile(_ p: TilePlacement) -> some View {
        let isSelected = editor.selectedTileID == p.tile.id
        return MetricTile(display: registry.display(p.tile.metric, from: env.telemetryStore.snapshot),
                          valueSize: CockpitView.valueSize(for: p))
            .overlay(alignment: .topTrailing) {
                if isSelected {
                    Button { withAnimation { editor.remove(p.tile.id) } } label: {
                        Image(systemName: "minus.circle.fill")
                            .font(.title3)
                            .foregroundStyle(Theme.Semantic.warning)
                            .padding(6)
                    }
                    .accessibilityLabel("Kachel entfernen")
                }
            }
            .overlay(
                RoundedRectangle(cornerRadius: Theme.tileCornerRadius)
                    .strokeBorder(isSelected ? Color.accentColor : .clear, lineWidth: 2)
            )
            .contentShape(Rectangle())
            .onTapGesture { editor.select(isSelected ? nil : p.tile.id) }
            .draggable(p.tile.id.uuidString)
            .dropDestination(for: String.self) { items, _ in
                guard let dragged = items.first,
                      let from = editor.index(ofTileID: dragged),
                      let to = editor.index(ofTileID: p.tile.id.uuidString) else { return false }
                withAnimation { editor.move(from: from, to: to) }
                return true
            }
    }

    // MARK: - Steuerung der ausgewählten Kachel

    private func tileControls(_ tile: TileConfig) -> some View {
        VStack(alignment: .leading, spacing: Theme.Spacing.unit) {
            HStack(spacing: Theme.Spacing.unit) {
                ForEach(TileSize.allCases, id: \.self) { size in
                    let isCurrent = tile.size == size
                    Button(size.rawValue.replacingOccurrences(of: "x", with: "×")) {
                        withAnimation { editor.setSize(size, for: tile.id) }
                    }
                    .buttonStyle(.bordered)
                    .tint(isCurrent ? .accentColor : .secondary)
                    .disabled(!isCurrent && !editor.canSetSize(size, for: tile.id))
                }
            }
            Picker("Metrik", selection: Binding(
                get: { tile.metric },
                set: { editor.setMetric($0, for: tile.id) }
            )) {
                ForEach(MetricID.allCases, id: \.self) { metric in
                    Text(registry.display(metric, from: .empty).label).tag(metric)
                }
            }
            .pickerStyle(.menu)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding()
        .background(.thinMaterial, in: RoundedRectangle(cornerRadius: Theme.tileCornerRadius))
    }
}
