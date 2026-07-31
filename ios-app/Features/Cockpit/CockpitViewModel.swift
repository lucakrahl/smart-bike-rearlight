import Foundation
import Observation
import SmartBikeCore

/// Präsentationslogik des Live-Cockpits (Schicht 8). Liest den TelemetryStore,
/// steuert Start/Stopp über den RideManager.
@MainActor @Observable
final class CockpitViewModel {
    private let store: TelemetryStore
    private let rides: RideManager
    private let registry = MetricRegistry()

    init(store: TelemetryStore, rides: RideManager) { self.store = store; self.rides = rides }

    var recording: RecordingState { rides.state }
    var liveState: LiveDataState { store.liveState }
    var connection: ConnectionState { store.connection }

    func display(_ id: MetricID) -> MetricDisplay { registry.display(id, from: store.snapshot) }
    func start() { rides.start() }                 // AR-UX-02: Tap
    func requestStop() async { await rides.stop() } // AR-UX-02: nach Halten (~1 s)
}
