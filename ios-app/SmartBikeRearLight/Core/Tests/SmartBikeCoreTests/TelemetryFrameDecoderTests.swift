import Testing
import Foundation
@testable import SmartBikeCore

/// AP2: zustandsloser v3-Decoder → `DecodeResult` (.ok / .truncatedV3 / .rejected).
/// Zugriff über withUnsafeBytes + loadUnaligned; Little-Endian; keine Pointer-Casts.
struct TelemetryFrameDecoderTests {

    // MARK: - Konstanten / v2

    @Test func frameSizeAndSchemaVersion() {
        #expect(TelemetryFrame.byteCount == 81)
        #expect(TelemetryFrame.schemaVersion == 2)
    }

    @Test func decodesV2KnownFieldsAsOkWithoutV3() {
        var bytes = zeroFrame(length: 81, version: 2)
        writeFloat(32.4, into: &bytes, at: 50)
        writeFloat(214.0, into: &bytes, at: 58)
        bytes[62] = 9
        bytes[78] = GnssFixStatus.fixOK.rawValue
        bytes[80] = 55

        let result = TelemetryFrameDecoder.decode(Data(bytes))
        #expect(isOK(result))
        let f = frame(result)
        #expect(f?.version == 2)
        #expect(f?.sats == 9)
        #expect(f?.gnssFix == .fixOK)
        #expect(f?.brakeLightPct == 55)
        #expect(abs((f?.speedKmph ?? 0) - 32.4) < 0.001)
        #expect(abs((f?.altitudeM ?? 0) - 214.0) < 0.001)
        #expect(f?.gnssAccelMs2 == nil)     // v2 → keine v3-Felder
    }

    // MARK: - Round-Trip (Test-Encoder ↔ Produktions-Decoder)

    @Test func roundTripAllFieldsIsOk() {
        // Gemeinsamer Encoder (dieselbe Byte-Quelle wie der Mock, AP3) → keine Divergenz.
        let original = sampleV3Frame()
        #expect(TelemetryFrameDecoder.decode(TelemetryFrameEncoder.encode(original)) == .ok(original))
    }

    // MARK: - Offset-Prüfung je neuem v3-Feld (13)

    @Test func eachV3FieldReadAtCorrectOffset() {
        #expect(decodeV3 { writeFloat(1.5, into: &$0, at: 81) }?.gnssAccelMs2 == 1.5)
        #expect(decodeV3 { writeFloat(2.5, into: &$0, at: 85) }?.pitchRad == 2.5)
        #expect(decodeV3 { writeFloat(3.5, into: &$0, at: 89) }?.gyroBiasRads == 3.5)
        #expect(decodeV3 { writeFloat(4.5, into: &$0, at: 93) }?.normDeltaMin == 4.5)
        #expect(decodeV3 { writeFloat(5.5, into: &$0, at: 97) }?.normDeltaMax == 5.5)
        #expect(decodeV3 { writeFloat(6.5, into: &$0, at: 101) }?.jerkMax == 6.5)
        #expect(decodeV3 { $0[105] = 3 }?.regimeStaticN == 3)
        #expect(decodeV3 { $0[106] = 5 }?.regimeDynamicN == 5)
        #expect(decodeV3 { $0[107] = 2 }?.regimeShockN == 2)
        #expect(decodeV3 { $0[108] = 1 }?.biasCalibrated == 1)
        #expect(decodeV3 { $0[109] = 1 }?.gnssAccelValid == 1)
        #expect(decodeV3 { $0[110] = 250 }?.dtMaxMs == 250)
        #expect(decodeV3 { writeU16(65000, into: &$0, at: 111) }?.loopMaxUs == 65000)
    }

    // MARK: - Nicht-ausgerichtete Felder (Off 81 float, Off 111 uint16)

    @Test func unalignedFieldsDecodeCorrectly() {
        let f = decodeV3 {
            writeFloat(12.5, into: &$0, at: 81)
            writeU16(40000, into: &$0, at: 111)
        }
        #expect(f?.gnssAccelMs2 == 12.5)
        #expect(f?.loopMaxUs == 40000)
    }

    // MARK: - Grenzfall-Matrix: Länge {80,81,112,113,200} × version {1,2,3,4}

    @Test func versionLengthMatrix() {
        for length in [80, 81, 112, 113, 200] {
            for version in [UInt16(1), 2, 3, 4] {
                let result = TelemetryFrameDecoder.decode(Data(zeroFrame(length: length, version: version)))

                let rejected = length < 81 || version < 2
                let truncatedV3 = !rejected && version >= 3 && length < 113
                let hasV3 = version >= 3 && length >= 113
                let ctx = "L=\(length) v=\(version)"

                if rejected {
                    #expect(isRejected(result), "\(ctx): sollte .rejected sein")
                } else if truncatedV3 {
                    #expect(isTruncatedV3(result), "\(ctx): sollte .truncatedV3 sein")
                } else {
                    #expect(isOK(result), "\(ctx): sollte .ok sein")
                }
                // v3-Felder nur bei vollständigem v3-Frame vorhanden.
                #expect((frame(result)?.gnssAccelMs2 != nil) == hasV3, "\(ctx): v3-Präsenz falsch")
            }
        }
    }

    // MARK: - E‑1: kurzes v3-Frame (Länge 112, version 3) → .truncatedV3

    @Test func shortV3FrameReturnsTruncatedV3() {
        var bytes = zeroFrame(length: 112, version: 3)   // 1 Byte zu kurz für volles v3
        writeFloat(27.0, into: &bytes, at: 50)           // v2-Feld muss lesbar bleiben

        let result = TelemetryFrameDecoder.decode(Data(bytes))
        #expect(isTruncatedV3(result))
        let f = frame(result)
        #expect(abs((f?.speedKmph ?? 0) - 27.0) < 0.001)  // v2-Felder gelesen
        #expect(f?.gnssAccelMs2 == nil)                   // v3-Felder nil
        #expect(f?.loopMaxUs == nil)
    }

    // MARK: - Verwerfen

    @Test func tooShortOrOldVersionIsRejected() {
        #expect(TelemetryFrameDecoder.decode(Data(count: 80)) == .rejected)                     // < 81
        #expect(TelemetryFrameDecoder.decode(Data(zeroFrame(length: 113, version: 1))) == .rejected)  // version 1
    }

    @Test func oversizedFrameIgnoresExtraBytes() {
        var bytes = zeroFrame(length: 113, version: 3)
        writeFloat(9.0, into: &bytes, at: 81)
        bytes.append(contentsOf: [0xAA, 0xBB, 0xCC])      // 116 Byte
        let result = TelemetryFrameDecoder.decode(Data(bytes))
        #expect(isOK(result))
        #expect(frame(result)?.gnssAccelMs2 == 9.0)
    }

    // MARK: - Helpers (Ergebnis)

    private func frame(_ r: DecodeResult) -> TelemetryFrame? {
        switch r {
        case .ok(let f), .truncatedV3(let f): return f
        case .rejected: return nil
        }
    }
    private func isOK(_ r: DecodeResult) -> Bool { if case .ok = r { return true }; return false }
    private func isTruncatedV3(_ r: DecodeResult) -> Bool { if case .truncatedV3 = r { return true }; return false }
    private func isRejected(_ r: DecodeResult) -> Bool { if case .rejected = r { return true }; return false }

    /// Volles v3-Frame (113 B, version 3), `mutate` anwenden, dekodieren, Frame zurückgeben.
    private func decodeV3(_ mutate: (inout [UInt8]) -> Void) -> TelemetryFrame? {
        var b = zeroFrame(length: 113, version: 3)
        mutate(&b)
        return frame(TelemetryFrameDecoder.decode(Data(b)))
    }

    // MARK: - Helpers (Bytes)

    private func zeroFrame(length: Int, version: UInt16) -> [UInt8] {
        var b = [UInt8](repeating: 0, count: length)
        if length >= 2 { writeU16(version, into: &b, at: 0) }
        return b
    }
    private func writeFloat(_ v: Float, into bytes: inout [UInt8], at offset: Int) {
        withUnsafeBytes(of: v.bitPattern.littleEndian) { raw in for i in 0..<4 { bytes[offset + i] = raw[i] } }
    }
    private func writeU16(_ v: UInt16, into bytes: inout [UInt8], at offset: Int) {
        withUnsafeBytes(of: v.littleEndian) { raw in for i in 0..<2 { bytes[offset + i] = raw[i] } }
    }

    private func sampleV3Frame() -> TelemetryFrame {
        TelemetryFrame(
            version: 3, timestampMs: 123_456,
            accelX: 0.1, accelY: 0.2, accelZ: 9.8, gyroX: 0.01, gyroY: -0.02, gyroZ: 0.03,
            brakeDecel: 3.2, pressurePa: 98_950, temperatureC: 21.5,
            lat: 51.2277, lon: 6.7735, speedKmph: 28.4, courseDeg: 123, altitudeM: 205,
            sats: 9, hdop: 0.8, utcYear: 2026, utcMonth: 8, utcDay: 7, utcHour: 14, utcMinute: 30, utcSecond: 15,
            systemState: .run, initDegraded: false, imuHealth: .recovering,
            baroValid: true, gnssFix: .fixOK, watchdogRecovered: true, brakeLightPct: 80,
            gnssAccelMs2: 3.1, pitchRad: 0.12, gyroBiasRads: -0.015,
            normDeltaMin: -1.2, normDeltaMax: 2.7, jerkMax: 4.4,
            regimeStaticN: 3, regimeDynamicN: 5, regimeShockN: 2,
            biasCalibrated: 1, gnssAccelValid: 1, dtMaxMs: 12, loopMaxUs: 1234)
    }
}
