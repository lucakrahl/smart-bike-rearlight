import Foundation

/// Dekodiertes Telemetrie-Frame (App Bible Kap. 10). Reiner Werttyp. Trägt die
/// v2-Felder (Offsets 0–80, nicht-optional) sowie die v3-Felder (Offsets 81–112) als
/// Optionals — `nil`, wenn das Frame sie nicht enthält (v2-Gerät / zu kurzes v3-Frame).
/// Decoder/Versionsregel folgen in AP2; hier nur der Werttyp.
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

    // MARK: v3-Felder (Offsets 81–112, Schema v3). Optional: `nil` = im Frame nicht
    // vorhanden (v2-Gerät oder zu kurzes v3-Frame) — Datentyp-Seite der
    // Vorwärtskompatibilität (AR-CONN-04). Benennung/Typen exakt aus Vertrag Kap. 3.2.
    public var gnssAccelMs2: Float?     // Off 81 — m/s², nur gültig wenn gnssAccelValid==1
    public var pitchRad: Float?         // Off 85 — rad, interne Lageschätzung
    public var gyroBiasRads: Float?     // Off 89 — rad/s, geschätzter gyro_x-Nullpunktfehler
    public var normDeltaMin: Float?     // Off 93 — m/s², Min (‖a‖−g) im 100-ms-Fenster
    public var normDeltaMax: Float?     // Off 97 — m/s², Max (‖a‖−g) im Fenster
    public var jerkMax: Float?          // Off 101 — m/s²/10 ms, Max |Δ‖a‖| dt-normiert
    public var regimeStaticN: UInt8?    // Off 105 — Anzahl STATIC-Samples im Fenster
    public var regimeDynamicN: UInt8?   // Off 106 — Anzahl DYNAMIC-Samples
    public var regimeShockN: UInt8?     // Off 107 — Anzahl SHOCK-Samples
    public var biasCalibrated: UInt8?   // Off 108 — 0/1, Stufe-1-Bias-Kalibrierung fertig
    public var gnssAccelValid: UInt8?   // Off 109 — 0/1, Gültigkeit der GNSS-Referenz
    public var dtMaxMs: UInt8?          // Off 110 — ms, größtes dt im Fenster, sat. 255
    public var loopMaxUs: UInt16?       // Off 111 — µs, längste Schleifendauer, sat. 65535

    public init(version: UInt16, timestampMs: UInt32,
                accelX: Float, accelY: Float, accelZ: Float,
                gyroX: Float, gyroY: Float, gyroZ: Float, brakeDecel: Float,
                pressurePa: Float, temperatureC: Float,
                lat: Float, lon: Float, speedKmph: Float, courseDeg: Float, altitudeM: Float,
                sats: UInt8, hdop: Float,
                utcYear: UInt16, utcMonth: UInt8, utcDay: UInt8, utcHour: UInt8, utcMinute: UInt8, utcSecond: UInt8,
                systemState: SystemState, initDegraded: Bool, imuHealth: ImuHealthState,
                baroValid: Bool, gnssFix: GnssFixStatus, watchdogRecovered: Bool,
                brakeLightPct: UInt8 = 0,
                gnssAccelMs2: Float? = nil, pitchRad: Float? = nil, gyroBiasRads: Float? = nil,
                normDeltaMin: Float? = nil, normDeltaMax: Float? = nil, jerkMax: Float? = nil,
                regimeStaticN: UInt8? = nil, regimeDynamicN: UInt8? = nil, regimeShockN: UInt8? = nil,
                biasCalibrated: UInt8? = nil, gnssAccelValid: UInt8? = nil,
                dtMaxMs: UInt8? = nil, loopMaxUs: UInt16? = nil) {
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
        self.gnssAccelMs2 = gnssAccelMs2; self.pitchRad = pitchRad; self.gyroBiasRads = gyroBiasRads
        self.normDeltaMin = normDeltaMin; self.normDeltaMax = normDeltaMax; self.jerkMax = jerkMax
        self.regimeStaticN = regimeStaticN; self.regimeDynamicN = regimeDynamicN; self.regimeShockN = regimeShockN
        self.biasCalibrated = biasCalibrated; self.gnssAccelValid = gnssAccelValid
        self.dtMaxMs = dtMaxMs; self.loopMaxUs = loopMaxUs
    }
}
