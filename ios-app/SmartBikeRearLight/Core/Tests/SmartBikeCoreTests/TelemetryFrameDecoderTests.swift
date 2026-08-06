import Testing
import Foundation
@testable import SmartBikeCore

/// Dekodierung des BLE-Schema-v2-Frames (81 Byte, App Bible Kap. 10). Firmware ist
/// autoritativ (FR-SYS-04) — die App liest nur, prüft Länge/Version, ist vorwärtskompatibel.
struct TelemetryFrameDecoderTests {

    @Test func frameSizeAndSchemaVersion() {
        #expect(TelemetryFrame.byteCount == 81)
        #expect(TelemetryFrame.schemaVersion == 2)
    }

    @Test func rejectsTooShort() {
        #expect(TelemetryFrameDecoder.decode(Data(count: 80)) == nil)   // v1-Länge → nil
        #expect(TelemetryFrameDecoder.decode(Data(count: 79)) == nil)
    }

    @Test func rejectsWrongVersion() {
        var bytes = [UInt8](repeating: 0, count: 81)
        bytes[0] = 1   // version = 1 (LE) bei korrekter Länge → trotzdem nil
        #expect(TelemetryFrameDecoder.decode(Data(bytes)) == nil)
    }

    @Test func decodesKnownFields() {
        var bytes = validV2Frame()
        writeFloat(32.4, into: &bytes, at: 50)                 // speed_kmph
        writeFloat(214.0, into: &bytes, at: 58)                // altitude_m
        bytes[62] = 9                                          // sats
        bytes[78] = GnssFixStatus.fixOK.rawValue               // gnss_fix
        bytes[76] = ImuHealthState.recovering.rawValue         // imu_health

        let frame = TelemetryFrameDecoder.decode(Data(bytes))
        #expect(frame != nil)
        #expect(frame?.version == 2)
        #expect(frame?.sats == 9)
        #expect(frame?.gnssFix == .fixOK)
        #expect(frame?.imuHealth == .recovering)
        #expect(abs((frame?.speedKmph ?? 0) - 32.4) < 0.001)
        #expect(abs((frame?.altitudeM ?? 0) - 214.0) < 0.001)
    }

    @Test func readsBrakeLightPctAtOffset80() {
        var bytes = validV2Frame()
        bytes[80] = 73                                         // brake_light_pct
        #expect(TelemetryFrameDecoder.decode(Data(bytes))?.brakeLightPct == 73)
    }

    @Test func acceptsOversizedFrameAndIgnoresExtraBytes() {
        // ≥ 81 Byte: überzählige Bytes dürfen nicht stören (Vorwärtskompatibilität AR-CONN-04).
        var bytes = validV2Frame()
        bytes[80] = 55
        bytes.append(contentsOf: [0xAA, 0xBB, 0xCC, 0xDD, 0xEE])   // 86 Byte
        let frame = TelemetryFrameDecoder.decode(Data(bytes))
        #expect(frame != nil)
        #expect(frame?.brakeLightPct == 55)
    }

    // MARK: Helpers

    private func validV2Frame() -> [UInt8] {
        var bytes = [UInt8](repeating: 0, count: 81)
        bytes[0] = 2; bytes[1] = 0   // version = 2 (LE)
        return bytes
    }

    private func writeFloat(_ v: Float, into bytes: inout [UInt8], at offset: Int) {
        let le = v.bitPattern.littleEndian
        withUnsafeBytes(of: le) { raw in
            for i in 0..<4 { bytes[offset + i] = raw[i] }
        }
    }
}
