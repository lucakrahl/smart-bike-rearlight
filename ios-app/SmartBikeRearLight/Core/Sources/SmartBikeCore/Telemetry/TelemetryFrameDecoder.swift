import Foundation

/// Dekodiert das 81-Byte-Frame (BLE-Schema v2, Little-Endian, feste Offsets — App Bible
/// Kap. 10). Rein & host-testbar (AR-CONN-04, AR-NFR-TST-01). Akzeptiert Länge ≥ 81 und
/// ignoriert überzählige Bytes (Vorwärtskompatibilität, FR-TEL-06); gibt nil zurück bei
/// zu kurzer Länge oder unbekannter Schema-Version.
public enum TelemetryFrameDecoder {
    public static func decode(_ data: Data) -> TelemetryFrame? {
        guard data.count >= TelemetryFrame.byteCount else { return nil }
        return data.withUnsafeBytes { (raw: UnsafeRawBufferPointer) -> TelemetryFrame? in
            func u8(_ o: Int) -> UInt8 { raw.load(fromByteOffset: o, as: UInt8.self) }
            func u16(_ o: Int) -> UInt16 { UInt16(littleEndian: raw.loadUnaligned(fromByteOffset: o, as: UInt16.self)) }
            func u32(_ o: Int) -> UInt32 { UInt32(littleEndian: raw.loadUnaligned(fromByteOffset: o, as: UInt32.self)) }
            func f32(_ o: Int) -> Float { Float(bitPattern: u32(o)) }

            let version = u16(0)
            guard version == TelemetryFrame.schemaVersion else { return nil }

            return TelemetryFrame(
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
                brakeLightPct: u8(80)                       // v2 (Offset 80)
            )
        }
    }
}
