import Testing
import Foundation
@testable import SmartBikeRearLight
import SmartBikeCore

/// AP4: der Store leitet genau zwei v3-Live-Status ab (`biasCalibrated`, `gnssAccelValid`);
/// die 11 Analyse-/Aggregatfelder werden NICHT als Live-Property gespiegelt.
@MainActor
struct TelemetryStoreV3StatusTests {

    private func v3Frame(biasCalibrated: UInt8?, gnssAccelValid: UInt8?) -> TelemetryFrame {
        TelemetryFrame(
            version: 3, timestampMs: 0,
            accelX: 0, accelY: 0, accelZ: 9.81, gyroX: 0, gyroY: 0, gyroZ: 0, brakeDecel: 0,
            pressurePa: 101_325, temperatureC: 20, lat: 0, lon: 0, speedKmph: 0, courseDeg: 0, altitudeM: 0,
            sats: 9, hdop: 1, utcYear: 2026, utcMonth: 1, utcDay: 1, utcHour: 0, utcMinute: 0, utcSecond: 0,
            systemState: .run, initDegraded: false, imuHealth: .ok,
            baroValid: true, gnssFix: .fixOK, watchdogRecovered: false, brakeLightPct: 0,
            gnssAccelMs2: 3.0, pitchRad: 0.1, gyroBiasRads: -0.01,
            normDeltaMin: -1.0, normDeltaMax: 2.0, jerkMax: 3.0,
            regimeStaticN: 4, regimeDynamicN: 5, regimeShockN: 1,
            biasCalibrated: biasCalibrated, gnssAccelValid: gnssAccelValid,
            dtMaxMs: 11, loopMaxUs: 1200)
    }

    @Test func statusesTrueWhenFieldsAreOne() {
        let store = TelemetryStore()
        store.consume(.ok(v3Frame(biasCalibrated: 1, gnssAccelValid: 1)))
        #expect(store.biasCalibrated == true)
        #expect(store.gnssAccelValid == true)
    }

    @Test func statusesFalseWhenFieldsAreZero() {
        let store = TelemetryStore()
        store.consume(.ok(v3Frame(biasCalibrated: 0, gnssAccelValid: 0)))
        #expect(store.biasCalibrated == false)
        #expect(store.gnssAccelValid == false)
    }

    @Test func statusesFalseWhenFieldsAbsent() {
        // v2-Frame (v3-Felder nil) → beide Status false, kein Absturz.
        let store = TelemetryStore()
        store.consume(.ok(v3Frame(biasCalibrated: nil, gnssAccelValid: nil)))
        #expect(store.biasCalibrated == false)
        #expect(store.gnssAccelValid == false)
    }

    /// Die 11 Analysefelder erscheinen nicht als eigene Live-Property des Stores
    /// (sie leben nur in `latestFrame`). Prüfung über die gespiegelten Stored-Properties.
    @Test func analysisFieldsAreNotMirroredAsLiveProperties() {
        let store = TelemetryStore()
        store.consume(.ok(v3Frame(biasCalibrated: 1, gnssAccelValid: 1)))

        let analysisFields: Set<String> = [
            "gnssAccelMs2", "pitchRad", "gyroBiasRads", "norm_delta_min".camelStub,
            "normDeltaMin", "normDeltaMax", "jerkMax",
            "regimeStaticN", "regimeDynamicN", "regimeShockN",
            "dtMaxMs", "loopMaxUs",
        ]
        // Gespiegelte Stored-Property-Namen (@Observable-Backing hat führenden „_").
        let propertyNames = Set(Mirror(reflecting: store).children.compactMap { child -> String? in
            guard let label = child.label else { return nil }
            return label.hasPrefix("_") ? String(label.dropFirst()) : label
        })
        for field in analysisFields {
            #expect(!propertyNames.contains(field),
                    "Analysefeld \(field) darf keine Live-Property des Stores sein")
        }
        // Gegenprobe: die zwei erlaubten Status sind vorhanden.
        #expect(propertyNames.contains("biasCalibrated"))
        #expect(propertyNames.contains("gnssAccelValid"))
        // Das ganze Frame bleibt weiterhin verfügbar (dort liegen die Analysefelder).
        #expect(store.latestFrame?.jerkMax == 3.0)
    }
}

private extension String {
    /// Dummy, damit versehentliche Snake-Case-Namen im Set nicht zufällig matchen.
    var camelStub: String { self }
}
