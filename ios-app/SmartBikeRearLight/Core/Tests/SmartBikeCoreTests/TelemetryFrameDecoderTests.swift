import XCTest
@testable import SmartBikeCore

final class TelemetryFrameDecoderTests: XCTestCase {

    func testRejectsWrongLength() {
        XCTAssertNil(TelemetryFrameDecoder.decode(Data(count: 79)))
        XCTAssertNil(TelemetryFrameDecoder.decode(Data(count: 81)))
    }

    func testRejectsUnknownVersion() {
        var bytes = [UInt8](repeating: 0, count: 80)
        bytes[0] = 2   // version = 2 (LE)
        XCTAssertNil(TelemetryFrameDecoder.decode(Data(bytes)))
    }

    func testDecodesKnownFields() {
        var bytes = [UInt8](repeating: 0, count: 80)
        bytes[0] = 1; bytes[1] = 0                              // version = 1
        writeFloat(32.4, into: &bytes, at: 50)                 // speed_kmph
        writeFloat(214.0, into: &bytes, at: 58)                // altitude_m
        bytes[62] = 9                                          // sats
        bytes[78] = GnssFixStatus.fixOK.rawValue               // gnss_fix
        bytes[76] = ImuHealthState.recovering.rawValue         // imu_health

        let frame = TelemetryFrameDecoder.decode(Data(bytes))
        XCTAssertNotNil(frame)
        XCTAssertEqual(frame?.version, 1)
        XCTAssertEqual(frame?.sats, 9)
        XCTAssertEqual(frame?.gnssFix, .fixOK)
        XCTAssertEqual(frame?.imuHealth, .recovering)
        XCTAssertEqual(frame?.speedKmph ?? 0, 32.4, accuracy: 0.001)
        XCTAssertEqual(frame?.altitudeM ?? 0, 214.0, accuracy: 0.001)
    }

    private func writeFloat(_ v: Float, into bytes: inout [UInt8], at offset: Int) {
        let le = v.bitPattern.littleEndian
        withUnsafeBytes(of: le) { raw in
            for i in 0..<4 { bytes[offset + i] = raw[i] }
        }
    }
}
