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
    var isRecording: Bool { rides.state != .idle }
    var liveState: LiveDataState { store.liveState }
    var connection: ConnectionState { store.connection }
    var sats: Int { store.snapshot.sats }
    var gnssFix: GnssFixStatus { store.latestFrame?.gnssFix ?? .noData }

    /// Momentanwerte aus dem Live-Store, angereichert um die mitlaufenden
    /// Aufzeichnungs-Kennzahlen aus dem `RideManager` (Distanz/Fahrzeit/Ø/Max).
    private var snapshot: LiveSnapshot {
        var s = store.snapshot
        let st = rides.statistics
        s.distanceKm = st.distanceKm
        s.duration = st.duration
        s.avgSpeedKmph = st.avgSpeedKmph
        s.maxSpeedKmph = st.maxSpeedKmph
        s.ascentM = st.ascentM
        s.descentM = st.descentM
        return s
    }

    func display(_ id: MetricID) -> MetricDisplay { registry.display(id, from: snapshot) }
    func start() { rides.start() }                 // AR-UX-02: Tap
    func requestStop() async { await rides.stop() } // AR-UX-02: nach Halten (~1 s)
}
