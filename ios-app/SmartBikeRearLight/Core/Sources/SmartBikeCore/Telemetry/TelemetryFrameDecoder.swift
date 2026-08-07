import Foundation

/// Ergebnis der Frame-Dekodierung (rein, ohne Seiteneffekt/Zähler). Der Consumer/Store
/// wertet die Fälle aus und zählt Fehler bzw. zu kurze v3-Frames (E‑1).
public enum DecodeResult: Sendable, Equatable {
    /// Vollständig gelesenes Frame (v2, oder v3 bei voller Länge).
    case ok(TelemetryFrame)
    /// v3-Gerät, aber Frame zu kurz (81 ≤ Länge < 113): v2-Ebene gültig, v3-Felder `nil` (E‑1).
    case truncatedV3(TelemetryFrame)
    /// Verworfen (Länge < 81 oder version < 2) — beim Zählen als Fehler werten.
    case rejected
}

/// Dekodiert BLE-Telemetrie-Frames v2 **und** v3 — **zustandslos & rein** (App Bible Kap. 10,
/// Vertrag Schema v3). Host-testbar, kein SwiftUI/CoreBluetooth/SwiftData.
///
/// Versions-/Längenregel (genau in dieser Reihenfolge):
///  1. Länge < 81 → `.rejected`.
///  2. `version` < 2 → `.rejected` (deckt v1 ab).
///  3. sonst v2-Felder (Offsets 0–80) lesen.
///  4. `version >= 3 && Länge >= 113` → zusätzlich v3-Felder (81–112) lesen; sonst v3 = `nil`.
///  5. E‑1: `version >= 3 && 81 <= Länge < 113` → `.truncatedV3` (v3 = `nil`), kein Verwerfen.
///  6. Bytes jenseits der erwarteten Länge werden ignoriert (Vorwärtskompatibilität).
///
/// Zugriff zwingend über `withUnsafeBytes` + `loadUnaligned` (Little-Endian, keine
/// Pointer-Casts) — mehrere Offsets sind nicht typ-ausgerichtet.
public enum TelemetryFrameDecoder {
    private static let minLength = 81    // v2-Ebene / Mindestlänge
    private static let v3Length = 113    // vollständiges v3-Frame

    public static func decode(_ data: Data) -> DecodeResult {
        let length = data.count
        guard length >= minLength else { return .rejected }   // 1.

        return data.withUnsafeBytes { (raw: UnsafeRawBufferPointer) -> DecodeResult in
            func u8(_ o: Int) -> UInt8 { raw.load(fromByteOffset: o, as: UInt8.self) }
            func u16(_ o: Int) -> UInt16 { UInt16(littleEndian: raw.loadUnaligned(fromByteOffset: o, as: UInt16.self)) }
            func u32(_ o: Int) -> UInt32 { UInt32(littleEndian: raw.loadUnaligned(fromByteOffset: o, as: UInt32.self)) }
            func f32(_ o: Int) -> Float { Float(bitPattern: u32(o)) }

            let version = u16(0)
            guard version >= 2 else { return .rejected }       // 2.

            let hasV3 = version >= 3 && length >= v3Length      // 4.
            let isTruncatedV3 = version >= 3 && length < v3Length   // 5.

            let frame = TelemetryFrame(
                version: version,
                timestampMs: u32(2),
                accelX: f32(6), accelY: f32(10), accelZ: f32(14),
                gyroX: f32(18), gyroY: f32(22), gyroZ: f32(26),
                brakeDecel: f32(30),
                pressurePa: f32(34), temperatureC: f32(38),
                lat: f32(42), lon: f32(46),
                speedKmph: f32(50), courseDeg: f32(54), altitudeM: f32(58),
                sats: u8(62), hdop: f32(63),
                utcYear: u16(67),
                utcMonth: u8(69), utcDay: u8(70), utcHour: u8(71), utcMinute: u8(72), utcSecond: u8(73),
                systemState: SystemState(rawValue: u8(74)) ?? .initializing,
                initDegraded: u8(75) != 0,
                imuHealth: ImuHealthState(rawValue: u8(76)) ?? .ok,
                baroValid: u8(77) != 0,
                gnssFix: GnssFixStatus(rawValue: u8(78)) ?? .noData,
                watchdogRecovered: u8(79) != 0,
                brakeLightPct: u8(80),                                   // v2 (Offset 80)
                // v3 (Offsets 81–112) — nur bei vollständigem v3-Frame, sonst nil.
                gnssAccelMs2: hasV3 ? f32(81) : nil,
                pitchRad: hasV3 ? f32(85) : nil,
                gyroBiasRads: hasV3 ? f32(89) : nil,
                normDeltaMin: hasV3 ? f32(93) : nil,
                normDeltaMax: hasV3 ? f32(97) : nil,
                jerkMax: hasV3 ? f32(101) : nil,
                regimeStaticN: hasV3 ? u8(105) : nil,
                regimeDynamicN: hasV3 ? u8(106) : nil,
                regimeShockN: hasV3 ? u8(107) : nil,
                biasCalibrated: hasV3 ? u8(108) : nil,
                gnssAccelValid: hasV3 ? u8(109) : nil,
                dtMaxMs: hasV3 ? u8(110) : nil,
                loopMaxUs: hasV3 ? u16(111) : nil
            )
            return isTruncatedV3 ? .truncatedV3(frame) : .ok(frame)
        }
    }
}
