import Testing
import Foundation
@testable import SmartBikeRearLight
import SmartBikeCore

/// AP8: read-only Diagnose-Ableitung. Getestet wird die reine `DiagnosticsReadout`-Logik
/// (Flags, Zahl-Formatierung, E-1-Defektindikator) sowie die Verdrahtung ViewModel ← Store.
struct DiagnosticsViewModelTests {

    @Test func derivesFlagsNumbersAndNoDefect() {
        let r = DiagnosticsReadout(biasCalibrated: true, gnssAccelValid: false,
                                   dtMaxMs: 12, loopMaxUs: 1234, truncatedV3FrameCount: 0)
        #expect(r.biasCalibrated == true)
        #expect(r.gnssAccelValid == false)
        #expect(r.dtMaxDisplay == "12 ms")
        #expect(r.loopMaxDisplay == "1234 µs")
        #expect(r.hasTruncatedDefect == false)
    }

    @Test func truncatedCountAboveZeroIsDefectAndMissingValuesDash() {
        let r = DiagnosticsReadout(biasCalibrated: false, gnssAccelValid: true,
                                   dtMaxMs: nil, loopMaxUs: nil, truncatedV3FrameCount: 3)
        #expect(r.hasTruncatedDefect == true)          // > 0 → Defekt (E-1)
        #expect(r.dtMaxDisplay == "—")                 // fehlender Wert
        #expect(r.loopMaxDisplay == "—")
    }

    @MainActor @Test func viewModelReflectsLastFrameAndCounter() {
        let store = TelemetryStore()
        let vm = DiagnosticsViewModel(store: store)

        // Ausgangszustand: keine Frames → alles false/nil/0, kein Defekt.
        #expect(vm.readout.dtMaxMs == nil)
        #expect(vm.readout.hasTruncatedDefect == false)

        // Vollständiges v3-Frame: Flags + Zeitbudget kommen durch.
        store.consume(.ok(frame(dtMax: 7, loopMax: 900, bias: 1, gnssValid: 1)))
        #expect(vm.readout.biasCalibrated == true)
        #expect(vm.readout.gnssAccelValid == true)
        #expect(vm.readout.dtMaxMs == 7)
        #expect(vm.readout.loopMaxUs == 900)
        #expect(vm.readout.truncatedV3FrameCount == 0)

        // Zu kurzes v3-Frame → E-1-Zähler steigt (Defekt-Indikator).
        store.consume(.truncatedV3(frame(dtMax: nil, loopMax: nil, bias: 0, gnssValid: 0)))
        #expect(vm.readout.truncatedV3FrameCount == 1)
        #expect(vm.readout.hasTruncatedDefect == true)
    }

    private func frame(dtMax: UInt8?, loopMax: UInt16?, bias: UInt8?, gnssValid: UInt8?) -> TelemetryFrame {
        TelemetryFrame(version: 3, timestampMs: 0,
                       accelX: 0, accelY: 0, accelZ: 9.81,
                       gyroX: 0, gyroY: 0, gyroZ: 0, brakeDecel: 0,
                       pressurePa: 100_000, temperatureC: 20,
                       lat: 51, lon: 6, speedKmph: 0, courseDeg: 0, altitudeM: 100,
                       sats: 9, hdop: 1,
                       utcYear: 2026, utcMonth: 8, utcDay: 7, utcHour: 12, utcMinute: 0, utcSecond: 0,
                       systemState: .run, initDegraded: false, imuHealth: .ok,
                       baroValid: true, gnssFix: .fixOK, watchdogRecovered: false,
                       biasCalibrated: bias, gnssAccelValid: gnssValid,
                       dtMaxMs: dtMax, loopMaxUs: loopMax)
    }
}
