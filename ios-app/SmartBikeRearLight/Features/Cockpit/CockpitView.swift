import SwiftUI
import SmartBikeCore

/// Live-Cockpit (App Bible 6.4). Zwei Zustände (AR-LIVE-05):
/// Bereitschaft (Momentanwerte + „Fahrt starten") und Aufzeichnung (mitlaufende
/// Kennzahlen + „Zum Stoppen halten"). Ohne Scrollen, keine modalen Alerts (AR-UX-01).
struct CockpitView: View {
    @Environment(AppEnvironment.self) private var env

    var body: some View {
        // Leichter VM (hält nur Referenzen); liest die @Observable-Stores → Live-Updates.
        let vm = CockpitViewModel(store: env.telemetryStore, rides: env.rideManager)
        NavigationStack {
            VStack(spacing: Theme.Spacing.unit * 2) {
                StatusBar(connection: vm.connection, fix: vm.gnssFix, sats: vm.sats)
                    .frame(maxWidth: .infinity, alignment: .leading)

                // Hero-Kachel „Geschwindigkeit" — in beiden Zuständen sichtbar, glanceable.
                MetricTile(display: vm.display(.speed),
                           isStale: vm.liveState == .stale,
                           valueSize: 64)
                    .frame(maxWidth: .infinity, minHeight: 180)

                if vm.isRecording {
                    secondaryGrid(vm)
                    Spacer(minLength: 0)
                    HoldToStopButton { await vm.requestStop() }
                        .frame(maxWidth: .infinity)
                } else {
                    Spacer(minLength: 0)
                    startButton { vm.start() }
                }
            }
            .padding()
            .navigationTitle("Live")
            // Dezente Haptik bei Start/Stopp (AR-UX-04).
            .sensoryFeedback(trigger: vm.recording) { _, new in
                switch new {
                case .recording: return .impact(weight: .medium)
                case .idle:      return .success
                case .finishing: return nil
                }
            }
        }
    }

    /// 3-Spalten-Raster; kompakte Zifferngröße, damit z. B. 00:00:00 nicht umbricht.
    private func secondaryGrid(_ vm: CockpitViewModel) -> some View {
        HStack(spacing: Theme.Spacing.unit) {
            MetricTile(display: vm.display(.distance), isStale: vm.liveState == .stale, valueSize: 26)
            MetricTile(display: vm.display(.duration), isStale: vm.liveState == .stale, valueSize: 26)
            MetricTile(display: vm.display(.avgSpeed), isStale: vm.liveState == .stale, valueSize: 26)
        }
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
        .floatingGlass(interactive: true, in: .capsule)   // interaktives Glas-Control
        .gesture(
            // DragGesture(minimumDistance: 0) erkennt Druck (onChanged) und Loslassen (onEnded).
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
            // View verschwindet i. d. R. mit dem Zustandswechsel; Reset zur Sicherheit.
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
