import Foundation
import Observation
import SmartBikeCore

/// Schicht 4 — Aufzeichnungs-Lebenszyklus (App Bible 9.2, AR-REC-01, AR-DATA-01/04).
/// Verdichtet Frames auf 1 Hz, schreibt inkrementell, ordnet Backfill idempotent ein.
@MainActor @Observable
final class RideManager {
    private(set) var state: RecordingState = .idle
    private let repository: RideRepository
    private let statistics = StatisticsEngine()
    private var currentRide: UUID?
    private var lastPersistedSecond: Int = -1

    init(repository: RideRepository) { self.repository = repository }

    func start() {
        state = .recording
        // TODO: repository.startRide(...) + currentRide setzen
    }

    /// Aufruf pro dekodiertem Frame (nur bei state == .recording).
    func ingest(_ point: TrackPoint) {
        guard state == .recording, let ride = currentRide else { return }
        let sec = Int(point.t)
        guard sec != lastPersistedSecond else { return }   // 1-Hz-Verdichtung (AR-DATA-02)
        lastPersistedSecond = sec
        Task { try? await repository.append(point, to: ride) }
    }

    func stop() async {
        guard let ride = currentRide else { state = .idle; return }
        state = .finishing
        // TODO: Punkte laden, Statistik berechnen, finishRide(...)
        currentRide = nil; lastPersistedSecond = -1; state = .idle
    }

    /// Beim App-Start: hängengebliebene Aufzeichnung anbieten (AR-DATA-04).
    func recoverIfNeeded() async {
        currentRide = try? await repository.recoverUnfinishedRide()
    }
}
