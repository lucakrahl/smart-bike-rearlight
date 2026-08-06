import Foundation

/// Dekodiertes Telemetrie-Frame, BLE-Schema v2 (81 Byte, App Bible Kap. 10). Reiner Werttyp.
public struct TelemetryFrame: Sendable, Equatable {
    /// Mindest-/Sollgröße des Frames. Überzählige Bytes werden vom Decoder ignoriert.
    public static let byteCount = 81
    public static let schemaVersion: UInt16 = 2

    public var version: UInt16
    public var timestampMs: UInt32
    public var accelX, accelY, accelZ: Float
    public var gyroX, gyroY, gyroZ: Float
    public var brakeDecel: Float
    public var pressurePa: Float
    public var temperatureC: Float
    public var lat: Float
    public var lon: Float
    public var speedKmph: Float
    public var courseDeg: Float
    public var altitudeM: Float
    public var sats: UInt8
    public var hdop: Float
    public var utcYear: UInt16
    public var utcMonth, utcDay, utcHour, utcMinute, utcSecond: UInt8
    public var systemState: SystemState
    public var initDegraded: Bool
    public var imuHealth: ImuHealthState
    public var baroValid: Bool
    public var gnssFix: GnssFixStatus
    public var watchdogRecovered: Bool
    /// v2 (Offset 80): tatsächlich kommandierte Rücklicht-Duty in Prozent (0…100),
    /// Gegenstück zum rohen `brakeDecel` (Offset 30).
    public var brakeLightPct: UInt8

    public init(version: UInt16, timestampMs: UInt32,
                accelX: Float, accelY: Float, accelZ: Float,
                gyroX: Float, gyroY: Float, gyroZ: Float, brakeDecel: Float,
                pressurePa: Float, temperatureC: Float,
                lat: Float, lon: Float, speedKmph: Float, courseDeg: Float, altitudeM: Float,
                sats: UInt8, hdop: Float,
                utcYear: UInt16, utcMonth: UInt8, utcDay: UInt8, utcHour: UInt8, utcMinute: UInt8, utcSecond: UInt8,
                systemState: SystemState, initDegraded: Bool, imuHealth: ImuHealthState,
                baroValid: Bool, gnssFix: GnssFixStatus, watchdogRecovered: Bool,
                brakeLightPct: UInt8 = 0) {
        self.version = version; self.timestampMs = timestampMs
        self.accelX = accelX; self.accelY = accelY; self.accelZ = accelZ
        self.gyroX = gyroX; self.gyroY = gyroY; self.gyroZ = gyroZ; self.brakeDecel = brakeDecel
        self.pressurePa = pressurePa; self.temperatureC = temperatureC
        self.lat = lat; self.lon = lon; self.speedKmph = speedKmph; self.courseDeg = courseDeg; self.altitudeM = altitudeM
        self.sats = sats; self.hdop = hdop
        self.utcYear = utcYear; self.utcMonth = utcMonth; self.utcDay = utcDay
        self.utcHour = utcHour; self.utcMinute = utcMinute; self.utcSecond = utcSecond
        self.systemState = systemState; self.initDegraded = initDegraded; self.imuHealth = imuHealth
        self.baroValid = baroValid; self.gnssFix = gnssFix; self.watchdogRecovered = watchdogRecovered
        self.brakeLightPct = brakeLightPct
    }
}
