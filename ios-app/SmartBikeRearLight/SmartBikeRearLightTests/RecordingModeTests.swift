import Testing
import Foundation
@testable import SmartBikeRearLight
import SmartBikeCore

/// AP6: umschaltbarer Aufzeichnungsmodus 1 Hz (Default) / 10 Hz (Validierung).
/// Getestet gegen ein zählendes Mock-`RideRepository` (keine SwiftData-/UI-Abhängigkeit):
/// Sample-Rate je Modus, Batch-Schreiben (Saves ≪ Samples), Umschaltsperre während der
/// Aufzeichnung und eindeutige (nicht doppelte) Zeitstempel bei 10 Hz (E-4).
struct RecordingModeTests {

    /// Zählt Schreibvorgänge (ein `appendBatch` == ein `save()`) und sammelt alle Punkte.
    private actor CountingRepository: RideRepository {
        private(set) var saveCount = 0
        private(set) var points: [TrackPoint] = []
        private let rideID = UUID()

        func startRide(deviceId: UUID?) async throws -> UUID { rideID }
        func append(_ point: TrackPoint, to rideId: UUID) async throws {
            saveCount += 1; points.append(point)
        }
        func appendBatch(_ points: [TrackPoint], to rideId: UUID) async throws {
            saveCount += 1; self.points.append(contentsOf: points)
        }
        func finishRide(_ rideId: UUID, statistics: RideStatistics) async throws {}
        func allRides() async throws -> [RideSummary] { [] }
        func ride(_ id: UUID) async throws -> RideDetail? { nil }
        func deleteRide(_ id: UUID) async throws {}
        func recoverUnfinishedRide() async throws -> UUID? { nil }

        func snapshot() -> (saveCount: Int, points: [TrackPoint]) { (saveCount, points) }
    }

    /// Minimal gültiges v3-Frame mit gegebenem Geräte-Zeitstempel.
    private func frame(ms: UInt32) -> TelemetryFrame {
        TelemetryFrame(version: 3, timestampMs: ms,
                       accelX: 0, accelY: 0, accelZ: 9.81,
                       gyroX: 0, gyroY: 0, gyroZ: 0, brakeDecel: 0,
                       pressurePa: 100_000, temperatureC: 20,
                       lat: 51, lon: 6, speedKmph: 20, courseDeg: 0, altitudeM: 100,
                       sats: 9, hdop: 1,
                       utcYear: 2026, utcMonth: 8, utcDay: 7, utcHour: 12, utcMinute: 0, utcSecond: 0,
                       systemState: .run, initDegraded: false, imuHealth: .ok,
                       baroValid: true, gnssFix: .fixOK, watchdogRecovered: false)
    }

    /// Fahrt über 2 s mit 10-Hz-Eingang (21 Frames: 0…2000 ms, Schritt 100).
    @MainActor
    private func record(mode: RecordingMode) async -> (saveCount: Int, points: [TrackPoint]) {
        let repo = CountingRepository()
        let manager = RideManager(repository: repo)
        manager.setMode(mode)
        manager.start()
        await manager.recordingReady()
        for ms in stride(from: 0, through: 2000, by: 100) { manager.ingest(frame(ms: UInt32(ms))) }
        await manager.stop()
        return await repo.snapshot()
    }

    @MainActor @Test func sampleRatePerMode() async {
        let oneHz = await record(mode: .hz1)
        let tenHz = await record(mode: .hz10)
        // 1 Hz: ~1 Punkt/s über 2 s (Float-Bucket-Toleranz); 10 Hz: jedes Eingangs-Frame.
        #expect((2...4).contains(oneHz.points.count))
        #expect(tenHz.points.count >= 20)
        #expect(tenHz.points.count > oneHz.points.count * 3)
    }

    @MainActor @Test func batchesWritesAtTenHz() async {
        let snap = await record(mode: .hz10)
        // Batch-Schreiben: deutlich weniger Saves als Samples (≈ 1 Save/s statt 1/Sample).
        #expect(snap.points.count >= 20)
        #expect(snap.saveCount < snap.points.count)
        #expect(snap.saveCount <= 4)
    }

    @MainActor @Test func oneHzWritesEachSampleSingly() async {
        // Gegenprobe: bei 1 Hz bleibt es beim bisherigen Verhalten (1 Save je Sample).
        let snap = await record(mode: .hz1)
        #expect(snap.saveCount == snap.points.count)
    }

    @MainActor @Test func modeLockedWhileRecording() async {
        let repo = CountingRepository()
        let manager = RideManager(repository: repo)
        #expect(manager.setMode(.hz1))
        manager.start()                              // state == .recording (synchron)
        #expect(manager.setMode(.hz10) == false)     // Umschalten gesperrt
        #expect(manager.mode == .hz1)
        await manager.stop()
        #expect(manager.setMode(.hz10) == true)      // außerhalb der Aufzeichnung wieder erlaubt
        #expect(manager.mode == .hz10)
    }

    @MainActor @Test func timestampsUniqueAtTenHz() async {
        let snap = await record(mode: .hz10)
        let ts = snap.points.map(\.t)
        #expect(Set(ts).count == ts.count)                       // keine doppelten Zeitstempel (E-4)
        // Auch mit 2 Nachkommastellen (AP7-CSV, t_s ≥ 2 Dezimalen) eindeutig.
        let formatted = ts.map { String(format: "%.2f", $0) }
        #expect(Set(formatted).count == ts.count)
    }

    /// Skalen-Test (E-4): lange 10-Hz-Aufzeichnung (10 min ≈ 6000 Samples). Die
    /// erwartete Sample-Zahl muss EXAKT getroffen werden (kein Float-Drift, der Samples
    /// verschluckt oder doppelt), und alle Zeitstempel bleiben auf 2 Nachkommastellen
    /// eindeutig. Deckt die Akkumulation der Aufzeichnungsuhr über die volle Fahrtdauer ab.
    @MainActor @Test func exactSampleCountOverLongTenHzRide() async {
        let repo = CountingRepository()
        let manager = RideManager(repository: repo)
        manager.setMode(.hz10)
        manager.start()
        await manager.recordingReady()

        let expected = 6000                                       // 10 min @ 10 Hz
        for i in 0..<expected { manager.ingest(frame(ms: UInt32(i * 100))) }
        await manager.stop()

        let snap = await repo.snapshot()
        #expect(snap.points.count == expected)                    // exakt, kein Verlust/Doppelung
        let formatted = snap.points.map { String(format: "%.2f", $0.t) }
        #expect(Set(formatted).count == expected)                 // 2-Nachkommastellen eindeutig
    }
}
