import Testing
@testable import SmartBikeCore

/// AP1: `TelemetryFrame` trägt die 13 v3-Felder als Optionals (ohne Verhaltensänderung).
struct TelemetryFrameTests {

    /// Konstruktion nur mit v2-Argumenten → alle 13 v3-Felder sind `nil`.
    @Test func v3FieldsDefaultToNil() {
        let f = TelemetryFrame(
            version: 2, timestampMs: 0,
            accelX: 0, accelY: 0, accelZ: 0, gyroX: 0, gyroY: 0, gyroZ: 0, brakeDecel: 0,
            pressurePa: 0, temperatureC: 0, lat: 0, lon: 0, speedKmph: 0, courseDeg: 0, altitudeM: 0,
            sats: 0, hdop: 0, utcYear: 0, utcMonth: 0, utcDay: 0, utcHour: 0, utcMinute: 0, utcSecond: 0,
            systemState: .run, initDegraded: false, imuHealth: .ok,
            baroValid: true, gnssFix: .fixOK, watchdogRecovered: false, brakeLightPct: 0)

        #expect(f.gnssAccelMs2 == nil)
        #expect(f.pitchRad == nil)
        #expect(f.gyroBiasRads == nil)
        #expect(f.normDeltaMin == nil)
        #expect(f.normDeltaMax == nil)
        #expect(f.jerkMax == nil)
        #expect(f.regimeStaticN == nil)
        #expect(f.regimeDynamicN == nil)
        #expect(f.regimeShockN == nil)
        #expect(f.biasCalibrated == nil)
        #expect(f.gnssAccelValid == nil)
        #expect(f.dtMaxMs == nil)
        #expect(f.loopMaxUs == nil)
    }

    /// Konstruktion mit gesetzten v3-Feldern → Werte werden unverändert getragen.
    @Test func v3FieldsCarryValues() {
        let f = TelemetryFrame(
            version: 3, timestampMs: 0,
            accelX: 0, accelY: 0, accelZ: 0, gyroX: 0, gyroY: 0, gyroZ: 0, brakeDecel: 0,
            pressurePa: 0, temperatureC: 0, lat: 0, lon: 0, speedKmph: 0, courseDeg: 0, altitudeM: 0,
            sats: 0, hdop: 0, utcYear: 0, utcMonth: 0, utcDay: 0, utcHour: 0, utcMinute: 0, utcSecond: 0,
            systemState: .run, initDegraded: false, imuHealth: .ok,
            baroValid: true, gnssFix: .fixOK, watchdogRecovered: false, brakeLightPct: 0,
            gnssAccelMs2: 3.5, pitchRad: 0.1, gyroBiasRads: -0.02,
            normDeltaMin: -1.5, normDeltaMax: 2.5, jerkMax: 4.0,
            regimeStaticN: 3, regimeDynamicN: 5, regimeShockN: 2,
            biasCalibrated: 1, gnssAccelValid: 1, dtMaxMs: 12, loopMaxUs: 1234)

        #expect(f.gnssAccelMs2 == 3.5)
        #expect(f.regimeStaticN == 3)
        #expect(f.regimeDynamicN == 5)
        #expect(f.regimeShockN == 2)
        #expect(f.biasCalibrated == 1)
        #expect(f.gnssAccelValid == 1)
        #expect(f.dtMaxMs == 12)
        #expect(f.loopMaxUs == 1234)
    }
}
