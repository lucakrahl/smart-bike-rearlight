import Testing
@testable import SmartBikeCore

/// System-/Sensorwarnungen aus den Firmware-Statusfeldern (AR-LIVE-03).
struct SystemWarningsTests {

    /// „Gesundes" Frame; einzelne Statusfelder werden pro Test variiert.
    private func frame(system: SystemState = .run, initDegraded: Bool = false,
                       imu: ImuHealthState = .ok, baroValid: Bool = true,
                       watchdogRecovered: Bool = false) -> TelemetryFrame {
        TelemetryFrame(version: 2, timestampMs: 0,
                       accelX: 0, accelY: 0, accelZ: 0, gyroX: 0, gyroY: 0, gyroZ: 0, brakeDecel: 0,
                       pressurePa: 101_325, temperatureC: 20, lat: 0, lon: 0,
                       speedKmph: 0, courseDeg: 0, altitudeM: 0, sats: 9, hdop: 1,
                       utcYear: 2026, utcMonth: 1, utcDay: 1, utcHour: 0, utcMinute: 0, utcSecond: 0,
                       systemState: system, initDegraded: initDegraded, imuHealth: imu,
                       baroValid: baroValid, gnssFix: .fixOK, watchdogRecovered: watchdogRecovered)
    }

    @Test func allOkIsEmpty() {
        #expect(SystemWarnings.derive(from: frame()).isEmpty)
    }

    @Test func imuFailedIsWarnung() {
        let w = SystemWarnings.derive(from: frame(imu: .failed))
        #expect(w.count == 1)
        #expect(w[0].severity == .warnung)
        #expect(w[0].text == "IMU ausgefallen")
    }

    @Test func imuRecoveringIsInfo() {
        let w = SystemWarnings.derive(from: frame(imu: .recovering))
        #expect(w.map(\.severity) == [.info])
        #expect(w[0].text == "IMU stabilisiert sich")
    }

    @Test func statusFieldsMapToExpectedInfos() {
        #expect(SystemWarnings.derive(from: frame(initDegraded: true)).contains {
            $0.severity == .info && $0.text == "Start eingeschränkt" })
        #expect(SystemWarnings.derive(from: frame(watchdogRecovered: true)).contains {
            $0.text == "System neu gestartet" })
        #expect(SystemWarnings.derive(from: frame(system: .initializing)).contains {
            $0.text == "Rücklicht startet" })
        #expect(SystemWarnings.derive(from: frame(baroValid: false)).contains {
            $0.text == "Höhensensor ohne Werte" })
    }

    @Test func warnungSortedBeforeInfo() {
        // IMU ausgefallen (Warnung) + init_degraded (Info) → Warnung zuerst.
        let w = SystemWarnings.derive(from: frame(initDegraded: true, imu: .failed))
        #expect(w.count == 2)
        #expect(w[0].severity == .warnung)
        #expect(w[1].severity == .info)
    }
}
