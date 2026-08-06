import Testing
import SwiftData
import Foundation
@testable import SmartBikeRearLight
import SmartBikeCore

/// AR-DATA-04: eine beim Absturz hängengebliebene „recording"-Fahrt wird beim Start
/// erkannt und kann abgeschlossen oder verworfen werden — ohne Simulator-UI.
@MainActor
struct RecoveryTests {

    private func makeStore() throws -> SwiftDataStore {
        let schema = Schema([RideEntity.self, TrackSampleEntity.self, BLEDeviceEntity.self])
        let config = ModelConfiguration(schema: schema, isStoredInMemoryOnly: true)
        let container = try ModelContainer(for: schema, configurations: config)
        return SwiftDataStore(modelContainer: container)
    }
    private func sample(t: Double, speed: Double) -> TrackPoint {
        TrackPoint(t: t, lat: 0, lon: 0, altitudeM: 200, speedKmph: speed, courseDeg: 0,
                   sats: 9, hdop: 1, gnssFix: .fixOK)
    }

    @Test func recoverAndFinish() async throws {
        let store = try makeStore()
        let id = try await store.startRide(deviceId: nil)              // Status "recording"
        for t in 0..<5 { try await store.append(sample(t: Double(t), speed: 36), to: id) }

        let manager = RideManager(repository: store)
        await manager.recoverIfNeeded()
        #expect(manager.pendingRecovery?.id == id)
        #expect(manager.pendingRecovery?.sampleCount == 5)

        await manager.finishRecovered()
        #expect(manager.pendingRecovery == nil)

        let all = try await store.allRides()
        #expect(all.count == 1)
        #expect(all.first?.id == id)
        #expect(all.first!.statistics.distanceKm > 0)                 // 36 km/h → plausibel
        // Kein hängender "recording"-Datensatz mehr.
        let stillPending = try await store.recoverUnfinishedRide()
        #expect(stillPending == nil)
    }

    @Test func recoverAndDiscard() async throws {
        let store = try makeStore()
        let id = try await store.startRide(deviceId: nil)
        try await store.append(sample(t: 0, speed: 10), to: id)

        let manager = RideManager(repository: store)
        await manager.recoverIfNeeded()
        #expect(manager.pendingRecovery != nil)

        await manager.discardRecovered()
        #expect(manager.pendingRecovery == nil)
        #expect(try await store.allRides().isEmpty)
        let stillPending = try await store.recoverUnfinishedRide()
        #expect(stillPending == nil)
    }

    @Test func recoverAndResume() async throws {
        let store = try makeStore()
        let id = try await store.startRide(deviceId: nil)
        for t in 0..<5 { try await store.append(sample(t: Double(t), speed: 36), to: id) }

        let manager = RideManager(repository: store)
        await manager.recoverIfNeeded()
        #expect(manager.pendingRecovery != nil)

        await manager.resumeRecovered()
        #expect(manager.pendingRecovery == nil)
        #expect(manager.state == .recording)
        // Fahrt bleibt „recording" (nicht abgeschlossen), an dieselbe ID gekoppelt.
        let stillUnfinished = try await store.recoverUnfinishedRide()
        #expect(stillUnfinished == id)

        // Fortsetzen und später beenden → landet als abgeschlossene Fahrt im Verlauf.
        await manager.stop()
        #expect(manager.state == .idle)
        let all = try await store.allRides()
        #expect(all.count == 1)
        #expect(all.first?.id == id)
    }

    @Test func noRecoveryWhenNothingUnfinished() async throws {
        let store = try makeStore()
        let manager = RideManager(repository: store)
        await manager.recoverIfNeeded()
        #expect(manager.pendingRecovery == nil)
    }
}
