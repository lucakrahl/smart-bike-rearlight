import Testing
import Foundation
@testable import SmartBikeCore

/// AP2b (E‑3): Kreuztest gegen den **Firmware-Golden-Vektor**. Liest die eingefrorene
/// Bytefolge `testdata/frame_v3_golden.hex`, dekodiert sie mit dem **Produktions-Decoder**
/// (AP2, nicht dem Test-Encoder) und prüft jedes Feld gegen `frame_v3_golden.md`.
/// Fehlt eine der beiden Dateien → klarer Testfehler (keine stille Übersprung-Logik).
struct FrameGoldenVectorTests {

    /// Repo-`testdata/` relativ zur Quelldatei (unabhängig vom Arbeitsverzeichnis).
    /// Pfad: …/ios-app/SmartBikeRearLight/Core/Tests/SmartBikeCoreTests/<diese Datei>.
    private static var testdataDir: URL {
        var url = URL(fileURLWithPath: #filePath)
        for _ in 0..<6 { url.deleteLastPathComponent() }   // Datei → … → Repo-Wurzel
        return url.appendingPathComponent("testdata")
    }

    @Test func goldenVectorDecodesToFirmwareValues() throws {
        let dir = Self.testdataDir
        let hexURL = dir.appendingPathComponent("frame_v3_golden.hex")
        let mdURL = dir.appendingPathComponent("frame_v3_golden.md")

        // Beide Dateien sind Pflicht — fehlt eine, schlägt der Test bewusst fehl.
        #expect(FileManager.default.fileExists(atPath: mdURL.path),
                "frame_v3_golden.md fehlt — Kreuzvalidierung nicht möglich: \(mdURL.path)")
        let hexString = try #require(try? String(contentsOf: hexURL, encoding: .utf8),
                                     "frame_v3_golden.hex fehlt/nicht lesbar: \(hexURL.path)")

        let bytes = Self.parseHex(hexString)
        #expect(bytes.count == 113, "Golden-Vektor muss 113 Byte sein, war \(bytes.count)")

        // Produktions-Decoder aus AP2 — ausdrücklich NICHT der Test-Encoder.
        let result = TelemetryFrameDecoder.decode(Data(bytes))
        guard case .ok(let f) = result else {
            Issue.record("Erwartet .ok(113 Byte, v3), war: \(result)")
            return
        }

        // Ganzzahl-/Enum-Felder exakt.
        #expect(f.version == 3)
        #expect(f.timestampMs == 305_419_896)
        #expect(f.sats == 11)
        #expect(f.utcYear == 2026)
        #expect(f.utcMonth == 8)
        #expect(f.utcDay == 7)
        #expect(f.utcHour == 15)
        #expect(f.utcMinute == 42)
        #expect(f.utcSecond == 33)
        #expect(f.systemState == .run)
        #expect(f.initDegraded == true)
        #expect(f.imuHealth == .failed)
        #expect(f.baroValid == true)
        #expect(f.gnssFix == .fixOK)
        #expect(f.watchdogRecovered == true)
        #expect(f.brakeLightPct == 88)
        #expect(f.regimeStaticN == 3)
        #expect(f.regimeDynamicN == 6)
        #expect(f.regimeShockN == 2)
        #expect(f.biasCalibrated == 1)
        #expect(f.gnssAccelValid == 1)
        #expect(f.dtMaxMs == 13)
        #expect(f.loopMaxUs == 4567)

        // Float-Felder mit kleiner Toleranz (float32-Rundung).
        expectClose(f.accelX, 1.1, "accel_x_ms2")
        expectClose(f.accelY, 2.2, "accel_y_ms2")
        expectClose(f.accelZ, 3.3, "accel_z_ms2")
        expectClose(f.gyroX, 4.4, "gyro_x_rads")
        expectClose(f.gyroY, 5.5, "gyro_y_rads")
        expectClose(f.gyroZ, 6.6, "gyro_z_rads")
        expectClose(f.brakeDecel, 7.7, "brake_decel_ms2")
        expectClose(f.pressurePa, 101_325.5, "pressure_pa")
        expectClose(f.temperatureC, 23.4, "temperature_c")
        expectClose(f.lat, 51.2277, "lat")
        expectClose(f.lon, 6.7735, "lon")
        expectClose(f.speedKmph, 25.5, "speed_kmph")
        expectClose(f.courseDeg, 123.4, "course_deg")
        expectClose(f.altitudeM, 45.6, "altitude_m")
        expectClose(f.hdop, 1.23, "hdop")
        expectClose(f.gnssAccelMs2, 8.8, "gnss_accel_ms2")
        expectClose(f.pitchRad, 0.1234, "pitch_rad")
        expectClose(f.gyroBiasRads, 0.005678, "gyro_bias_rads")
        expectClose(f.normDeltaMin, -1.11, "norm_delta_min")
        expectClose(f.normDeltaMax, 9.99, "norm_delta_max")
        expectClose(f.jerkMax, 1.357, "jerk_max")
    }

    // MARK: - Helpers

    private func expectClose(_ actual: Float?, _ expected: Float, _ name: String,
                             tolerance: Float = 1e-4) {
        guard let actual else { Issue.record("\(name): Wert fehlt (nil)"); return }
        #expect(abs(actual - expected) <= tolerance,
                "\(name): erwartet ~\(expected), war \(actual)")
    }

    /// Hex-String (mit beliebigem Whitespace/Zeilenumbruch) → Bytes.
    private static func parseHex(_ string: String) -> [UInt8] {
        let hex = string.filter { $0.isHexDigit }
        var out: [UInt8] = []
        out.reserveCapacity(hex.count / 2)
        var i = hex.startIndex
        while i < hex.endIndex {
            let j = hex.index(i, offsetBy: 2, limitedBy: hex.endIndex) ?? hex.endIndex
            if j > i, let byte = UInt8(hex[i..<j], radix: 16) { out.append(byte) }
            i = j
        }
        return out
    }
}
