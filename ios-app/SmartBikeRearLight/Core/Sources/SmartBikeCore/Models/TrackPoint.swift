import Foundation

/// Persistenter 1-Hz-Datenpunkt (App Bible Kap. 11). Reiner Werttyp; die
/// SwiftData-@Model-Klassen (App-Target) mappen von/zu diesem Typ.
public struct TrackPoint: Sendable, Equatable {
    public var t: TimeInterval          // Sekunden seit Fahrtbeginn (monotone Aufzeichnungszeit)
    public var lat: Double
    public var lon: Double
    /// Effektive Höhe (bevorzugt barometrisch, sonst gültige GNSS-Höhe). `nil` = höhenlos
    /// → aus Höhenprofil/Höhenmetern ausgeschlossen.
    public var altitudeM: Double?
    public var speedKmph: Double
    public var courseDeg: Double
    public var sats: Int
    public var hdop: Double
    public var gnssFix: GnssFixStatus
    /// Bremslicht-Validierung (v2): rohe Verzögerung (m/s²) und kommandierte Rücklicht-Duty (0…100 %).
    public var brakeDecelMs2: Double?
    public var brakeLightPct: Int?
    /// IMU-Gesundheit zum Zeitpunkt: bei ≠ .ok zeigt das Rücklicht Fail-Safe-Schlusslicht
    /// statt Kennlinienwert — für die Auswertung als solches zu kennzeichnen.
    public var imuHealth: ImuHealthState
    /// Rohwerte fürs Nachvollziehen/Referenz.
    public var pressurePa: Double?
    public var gnssAltitudeM: Double?
    public var temperatureC: Double?
    /// Weitere Rohfelder aus dem Frame (Validierung/Export). Optional, da für Altdaten leer.
    public var deviceTimestampMs: UInt32?
    public var baroValid: Bool?
    public var systemState: SystemState?
    public var initDegraded: Bool?
    public var watchdogRecovered: Bool?
    public var frameVersion: Int?

    public init(t: TimeInterval, lat: Double, lon: Double, altitudeM: Double?,
                speedKmph: Double, courseDeg: Double, sats: Int, hdop: Double,
                gnssFix: GnssFixStatus, brakeDecelMs2: Double? = nil, brakeLightPct: Int? = nil,
                imuHealth: ImuHealthState = .ok, pressurePa: Double? = nil,
                gnssAltitudeM: Double? = nil, temperatureC: Double? = nil,
                deviceTimestampMs: UInt32? = nil, baroValid: Bool? = nil,
                systemState: SystemState? = nil, initDegraded: Bool? = nil,
                watchdogRecovered: Bool? = nil, frameVersion: Int? = nil) {
        self.t = t; self.lat = lat; self.lon = lon; self.altitudeM = altitudeM
        self.speedKmph = speedKmph; self.courseDeg = courseDeg; self.sats = sats
        self.hdop = hdop; self.gnssFix = gnssFix
        self.brakeDecelMs2 = brakeDecelMs2; self.brakeLightPct = brakeLightPct; self.imuHealth = imuHealth
        self.pressurePa = pressurePa; self.gnssAltitudeM = gnssAltitudeM; self.temperatureC = temperatureC
        self.deviceTimestampMs = deviceTimestampMs; self.baroValid = baroValid
        self.systemState = systemState; self.initDegraded = initDegraded
        self.watchdogRecovered = watchdogRecovered; self.frameVersion = frameVersion
    }
}
