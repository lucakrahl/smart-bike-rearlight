import Testing
import SwiftData
import Foundation
@testable import SmartBikeRearLight
import SmartBikeCore

/// AP5: die 13 v3-Felder werden additiv persistiert; Mapping Core-Werttyp ↔ @Model
/// (hin/zurück, inkl. nil); `temperature_c` bleibt im Modell.
struct PersistenceV3FieldsTests {

    private func makeStore(url: URL? = nil) throws -> SwiftDataStore {
        let schema = Schema([RideEntity.self, TrackSampleEntity.self, BLEDeviceEntity.self])
        let config = url.map { ModelConfiguration(schema: schema, url: $0) }
            ?? ModelConfiguration(schema: schema, isStoredInMemoryOnly: true)
        return SwiftDataStore(modelContainer: try ModelContainer(for: schema, configurations: config))
    }

    /// Punkt mit allen 13 v3-Feldern gesetzt.
    private func fullV3Point(t: Double) -> TrackPoint {
        TrackPoint(t: t, lat: 51.1, lon: 6.7, altitudeM: 200, speedKmph: 20, courseDeg: 10,
                   sats: 9, hdop: 1, gnssFix: .fixOK, brakeDecelMs2: 2.5, brakeLightPct: 80,
                   imuHealth: .ok, pressurePa: 98_950, gnssAltitudeM: 205, temperatureC: 21.5,
                   deviceTimestampMs: 12_345, baroValid: true, systemState: .run,
                   initDegraded: false, watchdogRecovered: false, frameVersion: 3,
                   gnssAccelMs2: 3.1, pitchRad: 0.12, gyroBiasRads: -0.015,
                   normDeltaMin: -1.2, normDeltaMax: 2.7, jerkMax: 4.4,
                   regimeStaticN: 3, regimeDynamicN: 5, regimeShockN: 2,
                   biasCalibrated: true, gnssAccelValid: true, dtMaxMs: 12, loopMaxUs: 1234)
    }

    /// v2-Punkt ohne v3-Felder (Simulation eines Alt-/v2-Samples).
    private func v2Point(t: Double) -> TrackPoint {
        TrackPoint(t: t, lat: 51.1, lon: 6.7, altitudeM: 200, speedKmph: 20, courseDeg: 10,
                   sats: 9, hdop: 1, gnssFix: .fixOK, temperatureC: 21.5, frameVersion: 2)
    }

    private func assertFull(_ p: TrackPoint) {
        #expect(p.gnssAccelMs2 == 3.1)
        #expect(p.pitchRad == 0.12)
        #expect(p.gyroBiasRads == -0.015)
        #expect(p.normDeltaMin == -1.2)
        #expect(p.normDeltaMax == 2.7)
        #expect(p.jerkMax == 4.4)
        #expect(p.regimeStaticN == 3)
        #expect(p.regimeDynamicN == 5)
        #expect(p.regimeShockN == 2)
        #expect(p.biasCalibrated == true)
        #expect(p.gnssAccelValid == true)
        #expect(p.dtMaxMs == 12)
        #expect(p.loopMaxUs == 1234)
        #expect(p.temperatureC == 21.5)   // temperature_c bleibt im Modell (AP5/AP7-Abgrenzung)
        #expect(p.frameVersion == 3)
    }

    private func assertV3Nil(_ p: TrackPoint) {
        #expect(p.gnssAccelMs2 == nil)
        #expect(p.pitchRad == nil)
        #expect(p.gyroBiasRads == nil)
        #expect(p.normDeltaMin == nil)
        #expect(p.normDeltaMax == nil)
        #expect(p.jerkMax == nil)
        #expect(p.regimeStaticN == nil)
        #expect(p.regimeDynamicN == nil)
        #expect(p.regimeShockN == nil)
        #expect(p.biasCalibrated == nil)
        #expect(p.gnssAccelValid == nil)
        #expect(p.dtMaxMs == nil)
        #expect(p.loopMaxUs == nil)
        #expect(p.temperatureC == 21.5)   // v2-Feld bleibt erhalten
    }

    /// Mapping Core ↔ @Model über den echten Schreib-/Lesepfad (in-memory).
    @Test func v3FieldsRoundTripThroughPersistence() async throws {
        let store = try makeStore()
        let id = try await store.startRide(deviceId: nil)
        try await store.append(fullV3Point(t: 0), to: id)
        try await store.append(v2Point(t: 1), to: id)
        try await store.finishRide(id, statistics: .zero)

        let pts = (try await store.ride(id)?.points ?? []).sorted { $0.t < $1.t }
        #expect(pts.count == 2)
        assertFull(pts[0])
        assertV3Nil(pts[1])
    }

    /// Neue Felder überstehen einen frischen Container auf derselben Datei (Öffnen/Laden +
    /// nil-Semantik). Hinweis: **kein** echter Alt-Schema-Migrationstest (s. Bericht).
    @Test func v3FieldsSurviveFreshContainer() async throws {
        let url = URL(fileURLWithPath: NSTemporaryDirectory())
            .appendingPathComponent("sbrl_v3_\(UUID().uuidString).store")
        defer { try? FileManager.default.removeItem(at: url) }

        let id: UUID
        do {
            let store = try makeStore(url: url)
            id = try await store.startRide(deviceId: nil)
            try await store.append(fullV3Point(t: 0), to: id)
            try await store.append(v2Point(t: 1), to: id)
            try await store.finishRide(id, statistics: .zero)
        }
        // Frischer Container auf derselben Datei = „App-Neustart".
        let store2 = try makeStore(url: url)
        let pts = (try await store2.ride(id)?.points ?? []).sorted { $0.t < $1.t }
        #expect(pts.count == 2)
        assertFull(pts[0])
        assertV3Nil(pts[1])
    }
}
