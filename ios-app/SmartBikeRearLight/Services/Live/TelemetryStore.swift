import Foundation
import Observation
import SmartBikeCore

/// Schicht 3 — zentrale Live-Wahrheitsquelle (App Bible 9.1/9.2).
/// Einzige Quelle, aus der die Cockpit-ViewModels lesen.
@MainActor @Observable
final class TelemetryStore {
    private(set) var latestFrame: TelemetryFrame?
    private(set) var connection: ConnectionState = .disconnected
    private(set) var liveState: LiveDataState = .none          // AR-UX-05
    private(set) var snapshot: LiveSnapshot = .empty

    /// Wird vom Decode-Consumer aufgerufen (10 Hz). Schreibt die Live-Momentaufnahme
    /// fort; Aufzeichnungs-Aggregate (Distanz/Zeit/Ø/Max) folgen über den RideManager.
    func apply(_ frame: TelemetryFrame) {
        latestFrame = frame
        liveState = .fresh
        var s = snapshot
        s.speedKmph = Double(frame.speedKmph)
        s.altitudeM = Double(frame.altitudeM)
        s.courseDeg = Double(frame.courseDeg)
        s.sats = Int(frame.sats)
        s.hdop = Double(frame.hdop)
        s.isConnected = (connection == .connected)
        snapshot = s
    }
    func update(connection: ConnectionState) { self.connection = connection }
    func markStale() { if liveState == .fresh { liveState = .stale } }   // AR-CONN-06
}
