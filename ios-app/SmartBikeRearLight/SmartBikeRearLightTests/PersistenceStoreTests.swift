import Testing
import SwiftData
import Foundation
@testable import SmartBikeRearLight
import SmartBikeCore

/// Verifiziert die SwiftData-Persistenz (SwiftDataStore) ohne Simulator-UI:
/// Aufzeichnen/Abschließen, Idempotenz beim Backfill, Recovery, Löschen und
/// Dauerhaftigkeit über einen frischen Container hinweg („App-Neustart").
struct PersistenceStoreTests {

    private func makeStore(url: URL? = nil) throws -> SwiftDataStore {
        let schema = Schema([RideEntity.self, TrackSampleEntity.self, BLEDeviceEntity.self])
        let config = url.map { ModelConfiguration(schema: schema, url: $0) }
            ?? ModelConfiguration(schema: schema, isStoredInMemoryOnly: true)
        let container = try ModelContainer(for: schema, configurations: config)
        return SwiftDataStore(modelContainer: container)   // hält den Container am Leben
    }

    private var stats: RideStatistics {
        RideStatistics(duration: 4, distanceKm: 0.028, avgSpeedKmph: 20, maxSpeedKmph: 30,
                       ascentM: 0, descentM: 0, minAltitudeM: 200, maxAltitudeM: 200)
    }
    private func sample(t: Double, speed: Double) -> TrackPoint {
        TrackPoint(t: t, lat: 0, lon: 0, altitudeM: 200, speedKmph: speed, courseDeg: 0,
                   sats: 9, hdop: 1, gnssFix: .fixOK)
    }

    @Test func recordFinishAndList() async throws {
        let store = try makeStore()
        let id = try await store.startRide(deviceId: nil)
        for t in 0..<5 { try await store.append(sample(t: Double(t), speed: Double(10 + t * 5)), to: id) }
        // Doppelter Zeitstempel muss ignoriert werden (Idempotenz, AR-DATA-04).
        try await store.append(sample(t: 2, speed: 999), to: id)
        try await store.finishRide(id, statistics: stats)

        let all = try await store.allRides()
        #expect(all.count == 1)
        #expect(all.first?.id == id)
        #expect(abs((all.first?.statistics.distanceKm ?? 0) - stats.distanceKm) < 1e-9)

        let detail = try await store.ride(id)
        #expect(detail?.points.count == 5)                       // Duplikat verworfen
        #expect(detail?.points.map(\.t) == [0, 1, 2, 3, 4])      // nach t sortiert
    }

    @Test func recoverReturnsRecordingRide() async throws {
        let store = try makeStore()
        let id = try await store.startRide(deviceId: nil)        // Status "recording"
        let recovered = try await store.recoverUnfinishedRide()
        #expect(recovered == id)

        try await store.finishRide(id, statistics: stats)        // jetzt "finished"
        let none = try await store.recoverUnfinishedRide()
        #expect(none == nil)
    }

    @Test func deleteRemovesRide() async throws {
        let store = try makeStore()
        let id = try await store.startRide(deviceId: nil)
        try await store.finishRide(id, statistics: stats)
        let before = try await store.allRides().count
        #expect(before == 1)

        try await store.deleteRide(id)
        let after = try await store.allRides().count
        #expect(after == 0)
    }

    @Test func dataSurvivesFreshContainer() async throws {
        let url = URL(fileURLWithPath: NSTemporaryDirectory())
            .appendingPathComponent("sbrl_test_\(UUID().uuidString).store")
        defer { try? FileManager.default.removeItem(at: url) }

        let id: UUID
        do {
            let store = try makeStore(url: url)
            id = try await store.startRide(deviceId: nil)
            try await store.append(sample(t: 0, speed: 10), to: id)
            try await store.finishRide(id, statistics: stats)
        }   // Store + Container werden freigegeben → Datei geschlossen

        // Frischer Container auf derselben Datei = „App-Neustart".
        let store2 = try makeStore(url: url)
        let all = try await store2.allRides()
        #expect(all.count == 1)
        #expect(all.first?.id == id)
    }
}
