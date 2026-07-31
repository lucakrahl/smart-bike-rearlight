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

    /// Wird vom Decode-Consumer aufgerufen (10 Hz). TODO: Snapshot fortschreiben.
    func apply(_ frame: TelemetryFrame) {
        latestFrame = frame
        liveState = .fresh
        // TODO: snapshot aus frame + laufender Aufzeichnung aktualisieren
    }
    func update(connection: ConnectionState) { self.connection = connection }
    func markStale() { if liveState == .fresh { liveState = .stale } }   // AR-CONN-06
}
