import SwiftUI
import SmartBikeCore

/// Live-Cockpit (App Bible 6.4). Statuszeile · anpassbares Kachel-Raster (AR-LIVE-08) ·
/// Start/Stopp. Ohne Scrollen, keine modalen Alerts im Fahrbetrieb (AR-UX-01).
struct CockpitView: View {
    @Environment(AppEnvironment.self) private var env
    @State private var showWarnings = false
    @State private var showEditor = false

    var body: some View {
        let vm = CockpitViewModel(store: env.telemetryStore, rides: env.rideManager)
        let placements = GridPacker.placements(for: env.dashboardLayout.tiles)
        NavigationStack {
            VStack(spacing: Theme.Spacing.unit * 2) {
                VStack(alignment: .leading, spacing: Theme.Spacing.unit) {
                    StatusBar(connection: vm.connection, fix: vm.gnssFix, sats: vm.sats,
                              warnings: vm.warnings)
                        .onTapGesture {
                            guard !vm.warnings.isEmpty else { return }
                            withAnimation { showWarnings.toggle() }
                        }
                    if showWarnings, !vm.warnings.isEmpty { warningList(vm.warnings) }
                }
                .frame(maxWidth: .infinity, alignment: .leading)

                // Anpassbares Kachel-Raster mit Live-Werten (kein Scrollen).
                DashboardGridView(placements: placements) { p in
                    MetricTile(display: vm.tileDisplay(p.tile.metric),
                               isStale: !vm.isFresh,
                               valueSize: Self.valueSize(for: p))
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)

                if vm.isRecording {
                    Text("Bearbeiten während der Aufzeichnung gesperrt")
                        .font(.caption2).foregroundStyle(.secondary)
                    HoldToStopButton { await vm.requestStop() }
                        .frame(maxWidth: .infinity)
                } else {
                    startButton { vm.start() }
                }
            }
            .padding()
            .navigationTitle("Live")
            .toolbar {
                ToolbarItem(placement: .topBarTrailing) {
                    Button { showEditor = true } label: {
                        Label("Bearbeiten", systemImage: "square.grid.2x2")
                    }
                    .disabled(vm.isRecording)   // nur außerhalb der Aufzeichnung (AR-LIVE-08)
                }
            }
            .sheet(isPresented: $showEditor) { CockpitEditorView() }
            .sensoryFeedback(trigger: vm.recording) { _, new in
                switch new {
                case .recording: return .impact(weight: .medium)
                case .idle:      return .success
                case .finishing: return nil
                }
            }
        }
    }

    /// Zifferngröße abhängig von der Kachelgröße (Hero groß, 1×1 kompakt).
    static func valueSize(for p: TilePlacement) -> CGFloat {
        if p.width >= 3 || p.height >= 2 { return 56 }
        if p.width == 2 { return 40 }
        return 26
    }

    private func warningList(_ warnings: [LiveWarning]) -> some View {
        VStack(alignment: .leading, spacing: 4) {
            ForEach(warnings) { w in
                Label(w.text, systemImage: w.severity == .warnung ? "exclamationmark.triangle.fill" : "info.circle")
                    .font(.caption)
                    .foregroundStyle(w.severity == .warnung ? Theme.Semantic.warning : Theme.Semantic.searching)
            }
        }
        .padding(.horizontal, 14)
        .padding(.vertical, 8)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(.thinMaterial, in: RoundedRectangle(cornerRadius: Theme.tileCornerRadius))
    }

    /// „Fahrt starten" — einfacher Tap (AR-UX-02), prominentes Liquid-Glass-Control.
    @ViewBuilder
    private func startButton(_ action: @escaping () -> Void) -> some View {
        let button = Button(action: action) {
            Label("Fahrt starten", systemImage: "record.circle")
                .font(.headline)
                .frame(maxWidth: .infinity, minHeight: Theme.minTapTarget + 16)
        }
        .controlSize(.large)

        if #available(iOS 26.0, *) {
            button.buttonStyle(.glassProminent).tint(.accentColor)
        } else {
            button.buttonStyle(.borderedProminent)
        }
    }
}

/// Positioniert Kacheln gemäß Packing im 3-Spalten-Raster (geteilt von Cockpit & Editor).
struct DashboardGridView<TileContent: View>: View {
    let placements: [TilePlacement]
    var spacing: CGFloat = Theme.Spacing.unit
    @ViewBuilder var content: (TilePlacement) -> TileContent

    var body: some View {
        GeometryReader { geo in
            let cols = CGFloat(DashboardLayout.columns)
            let usedRows = CGFloat(max(1, placements.map { $0.row + $0.height }.max() ?? 1))
            let colW = (geo.size.width - spacing * (cols - 1)) / cols
            let rowH = (geo.size.height - spacing * (usedRows - 1)) / usedRows
            ForEach(placements, id: \.tile.id) { p in
                content(p)
                    .frame(width: colW * CGFloat(p.width) + spacing * CGFloat(p.width - 1),
                           height: rowH * CGFloat(p.height) + spacing * CGFloat(p.height - 1),
                           alignment: .topLeading)
                    .offset(x: (colW + spacing) * CGFloat(p.column),
                            y: (rowH + spacing) * CGFloat(p.row))
            }
        }
    }
}

/// „Zum Stoppen halten" mit Fortschrittsring (AR-UX-02). Löst erst nach ~1 s Halten
/// aus; frühes Loslassen bricht ab. Kein modaler Dialog im Fahrbetrieb (AR-UX-01).
private struct HoldToStopButton: View {
    var onStop: () async -> Void
    private let holdDuration: Double = 1.0

    @State private var progress: CGFloat = 0
    @State private var holdTask: Task<Void, Never>?

    var body: some View {
        VStack(spacing: Theme.Spacing.unit) {
            ZStack {
                Circle().stroke(Color.secondary.opacity(0.25), lineWidth: 6)
                Circle()
                    .trim(from: 0, to: progress)
                    .stroke(Theme.Semantic.warning,
                            style: StrokeStyle(lineWidth: 6, lineCap: .round))
                    .rotationEffect(.degrees(-90))
                Image(systemName: "stop.fill")
                    .font(.system(size: 34, weight: .bold))
                    .foregroundStyle(Theme.Semantic.warning)
            }
            .frame(width: 96, height: 96)
            .contentShape(Circle())
            Text("Zum Stoppen halten")
                .font(.subheadline).foregroundStyle(.secondary)
        }
        .padding(.horizontal, 24)
        .padding(.vertical, 16)
        .floatingGlass(interactive: true, in: .capsule)
        .gesture(
            DragGesture(minimumDistance: 0)
                .onChanged { _ in if holdTask == nil { beginHold() } }
                .onEnded { _ in cancelHold() }
        )
        .accessibilityLabel("Zum Stoppen halten")
        .accessibilityHint("Eine Sekunde gedrückt halten, um die Aufzeichnung zu beenden.")
    }

    private func beginHold() {
        withAnimation(.linear(duration: holdDuration)) { progress = 1 }
        holdTask = Task {
            try? await Task.sleep(nanoseconds: UInt64(holdDuration * 1_000_000_000))
            if Task.isCancelled { return }
            await onStop()
            holdTask = nil
            progress = 0
        }
    }

    private func cancelHold() {
        guard let task = holdTask else { return }
        task.cancel()
        holdTask = nil
        withAnimation(.easeOut(duration: 0.2)) { progress = 0 }
    }
}
