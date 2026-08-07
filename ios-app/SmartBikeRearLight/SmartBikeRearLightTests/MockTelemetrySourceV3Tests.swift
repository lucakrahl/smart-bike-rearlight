import Testing
import Foundation
@testable import SmartBikeRearLight
import SmartBikeCore

/// AP3: der Mock erzeugt dekodierbare v3-Frames; über einen Lauf treten beide
/// Sonderfälle (`gnss_accel_valid == 0`, `bias_calibrated == 0`) auf.
struct MockTelemetrySourceV3Tests {

    @Test func producesDecodableV3FramesWithSpecialCases() {
        var allOK = true
        var sawInvalidGnss = false
        var sawUncalibrated = false

        for tick in 0..<300 {   // 30 s @ 10 Hz
            let data = MockTelemetrySource.makeFrame(timestampMs: UInt32(tick * 100), tick: Double(tick))
            #expect(data.count == 113)                      // volles v3-Frame

            guard case .ok(let f) = TelemetryFrameDecoder.decode(data), f.version == 3 else {
                allOK = false; continue
            }
            if f.gnssAccelValid == 0 {
                sawInvalidGnss = true
                #expect(f.gnssAccelMs2 == 0)                // ungültige Referenz → 0
            }
            if f.biasCalibrated == 0 { sawUncalibrated = true }
        }

        #expect(allOK, "alle Mock-Frames müssen als .ok(v3) dekodieren")
        #expect(sawInvalidGnss, "Sonderfall gnss_accel_valid == 0 trat nicht auf")
        #expect(sawUncalibrated, "Sonderfall bias_calibrated == 0 trat nicht auf")
    }
}
