import Foundation

/// Serialisiert ein `TelemetryFrame` in die Byte-Reihenfolge des Vertrags (Little-Endian,
/// gepackt). **Reiner Helfer** — kein BLE-Write zum Gerät (FR-SYS-04 unberührt): genutzt vom
/// `MockTelemetrySource` (simuliertes Gerät) und von den Round-Trip-Tests, damit es genau
/// **eine** Quelle für die Byte-Anordnung gibt (keine Divergenz zum Decoder).
///
/// `version >= 3` → 113 Byte (v2- + v3-Region); sonst 81 Byte (nur v2). `nil`-v3-Felder
/// werden als 0 geschrieben.
public enum TelemetryFrameEncoder {
    public static func encode(_ f: TelemetryFrame) -> Data {
        let length = f.version >= 3 ? 113 : 81
        var b = [UInt8](repeating: 0, count: length)

        func u16(_ v: UInt16, _ o: Int) { withUnsafeBytes(of: v.littleEndian) { for i in 0..<2 { b[o + i] = $0[i] } } }
        func u32(_ v: UInt32, _ o: Int) { withUnsafeBytes(of: v.littleEndian) { for i in 0..<4 { b[o + i] = $0[i] } } }
        func f32(_ v: Float, _ o: Int) { u32(v.bitPattern, o) }

        // v2-Region (0–80).
        u16(f.version, 0); u32(f.timestampMs, 2)
        f32(f.accelX, 6); f32(f.accelY, 10); f32(f.accelZ, 14)
        f32(f.gyroX, 18); f32(f.gyroY, 22); f32(f.gyroZ, 26)
        f32(f.brakeDecel, 30); f32(f.pressurePa, 34); f32(f.temperatureC, 38)
        f32(f.lat, 42); f32(f.lon, 46); f32(f.speedKmph, 50); f32(f.courseDeg, 54); f32(f.altitudeM, 58)
        b[62] = f.sats; f32(f.hdop, 63)
        u16(f.utcYear, 67)
        b[69] = f.utcMonth; b[70] = f.utcDay; b[71] = f.utcHour; b[72] = f.utcMinute; b[73] = f.utcSecond
        b[74] = f.systemState.rawValue; b[75] = f.initDegraded ? 1 : 0; b[76] = f.imuHealth.rawValue
        b[77] = f.baroValid ? 1 : 0; b[78] = f.gnssFix.rawValue; b[79] = f.watchdogRecovered ? 1 : 0
        b[80] = f.brakeLightPct

        // v3-Region (81–112) — nur bei version ≥ 3.
        if length >= 113 {
            f32(f.gnssAccelMs2 ?? 0, 81); f32(f.pitchRad ?? 0, 85); f32(f.gyroBiasRads ?? 0, 89)
            f32(f.normDeltaMin ?? 0, 93); f32(f.normDeltaMax ?? 0, 97); f32(f.jerkMax ?? 0, 101)
            b[105] = f.regimeStaticN ?? 0; b[106] = f.regimeDynamicN ?? 0; b[107] = f.regimeShockN ?? 0
            b[108] = f.biasCalibrated ?? 0; b[109] = f.gnssAccelValid ?? 0; b[110] = f.dtMaxMs ?? 0
            u16(f.loopMaxUs ?? 0, 111)
        }
        return Data(b)
    }
}
