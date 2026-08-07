import Testing
@testable import SmartBikeRearLight
import SmartBikeCore

/// AP2: die Fehler-/Truncation-Zähler leben jetzt im `TelemetryStore` (Decoder ist rein).
@MainActor
struct TelemetryStoreDecodeTests {

    private func v2Frame(speed: Float = 10) -> TelemetryFrame {
        TelemetryFrame(
            version: 2, timestampMs: 0,
            accelX: 0, accelY: 0, accelZ: 0, gyroX: 0, gyroY: 0, gyroZ: 0, brakeDecel: 0,
            pressurePa: 0, temperatureC: 0, lat: 0, lon: 0, speedKmph: speed, courseDeg: 0, altitudeM: 0,
            sats: 9, hdop: 1, utcYear: 2026, utcMonth: 1, utcDay: 1, utcHour: 0, utcMinute: 0, utcSecond: 0,
            systemState: .run, initDegraded: false, imuHealth: .ok,
            baroValid: true, gnssFix: .fixOK, watchdogRecovered: false, brakeLightPct: 0)
    }

    @Test func rejectedIncrementsErrorOnly() {
        let store = TelemetryStore()
        #expect(store.consume(.rejected) == nil)
        #expect(store.decodeErrorCount == 1)
        #expect(store.truncatedV3FrameCount == 0)
    }

    @Test func truncatedV3IncrementsTruncatedAndAppliesFrame() {
        let store = TelemetryStore()
        let f = v2Frame(speed: 27)
        let returned = store.consume(.truncatedV3(f))
        #expect(returned == f)                       // Frame fürs Aufzeichnen zurückgegeben
        #expect(store.truncatedV3FrameCount == 1)     // E‑1
        #expect(store.decodeErrorCount == 0)          // kein Fehler
        #expect(store.latestFrame == f)               // apply() lief
    }

    @Test func okAppliesFrameWithoutCounting() {
        let store = TelemetryStore()
        let f = v2Frame(speed: 33)
        #expect(store.consume(.ok(f)) == f)
        #expect(store.decodeErrorCount == 0)
        #expect(store.truncatedV3FrameCount == 0)
        #expect(store.latestFrame == f)
    }
}
