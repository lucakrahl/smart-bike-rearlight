import Foundation
import Observation
import SmartBikeCore

/// Schicht 4 — Aufzeichnungs-Lebenszyklus (App Bible 9.2, AR-REC-01, AR-DATA-01/04).
/// Verdichtet Frames auf 1 Hz, hält die Punkte im RAM und berechnet die Live-
/// Kennzahlen über die `StatisticsEngine`. Persistenz folgt separat; das (aktuell
/// no-op) `RideRepository` wird bereits aufgerufen, ist für die Live-Werte aber egal.
@MainActor @Observable
final class RideManager {
    private(set) var state: RecordingState = .idle
    /// Live mitlaufende Kennzahlen (Distanz/Fahrzeit/Ø/Max) — Quelle fürs Cockpit (AR-LIVE-05/07).
    private(set) var statistics: RideStatistics = .zero

    private let repository: RideRepository
    private let engine = StatisticsEngine()
    private var currentRide: UUID?
    private var points: [TrackPoint] = []
    private var startTimestampMs: UInt32?
    private var lastPersistedSecond: Int = -1

    init(repository: RideRepository) { self.repository = repository }

    func start() {
        guard state == .idle else { return }
        points.removeAll(keepingCapacity: true)
        statistics = .zero
        startTimestampMs = nil
        lastPersistedSecond = -1
        state = .recording
        // Persistenz folgt separat; Rückgabe dient später als Aufzeichnungs-ID.
        Task { currentRide = try? await repository.startRide(deviceId: nil) }
    }

    /// Aufruf pro dekodiertem Frame (10 Hz). Verdichtet auf 1 Hz und schreibt die
    /// Live-Kennzahlen fort. No-op, solange nicht aufgezeichnet wird.
    func ingest(_ frame: TelemetryFrame) {
        guard state == .recording else { return }

        // t relativ zur Startzeit; an die Geräte-Uhr des ersten Frames gekoppelt (monoton).
        if startTimestampMs == nil { startTimestampMs = frame.timestampMs }
        let base = startTimestampMs ?? frame.timestampMs
        let t = TimeInterval(frame.timestampMs &- base) / 1000.0

        let sec = Int(t)
        guard sec != lastPersistedSecond else { return }   // 1-Hz-Verdichtung (AR-DATA-02)
        lastPersistedSecond = sec

        let point = TrackPoint(
            t: t,
            lat: Double(frame.lat), lon: Double(frame.lon),
            altitudeM: Double(frame.altitudeM),
            speedKmph: Double(frame.speedKmph),
            courseDeg: Double(frame.courseDeg),
            sats: Int(frame.sats), hdop: Double(frame.hdop),
            gnssFix: frame.gnssFix
        )
        points.append(point)
        statistics = engine.computeStatistics(from: points)   // Live-Update (AR-LIVE-07)

        if let ride = currentRide {
            Task { try? await repository.append(point, to: ride) }
        }
    }

    func stop() async {
        guard state == .recording else { return }
        state = .finishing
        let finalStats = engine.computeStatistics(from: points)
        statistics = finalStats
        if let ride = currentRide {
            try? await repository.finishRide(ride, statistics: finalStats)
        }
        currentRide = nil
        lastPersistedSecond = -1
        state = .idle
    }

    /// Beim App-Start: hängengebliebene Aufzeichnung anbieten (AR-DATA-04).
    func recoverIfNeeded() async {
        currentRide = try? await repository.recoverUnfinishedRide()
    }
}
