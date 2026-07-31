import Foundation

/// Persistenter 1-Hz-Datenpunkt (App Bible Kap. 11). Reiner Werttyp; die
/// SwiftData-@Model-Klassen (App-Target) mappen von/zu diesem Typ.
public struct TrackPoint: Sendable, Equatable {
    public var t: TimeInterval          // Sekunden seit Fahrtbeginn (Geräte-Uhr)
    public var lat: Double
    public var lon: Double
    public var altitudeM: Double
    public var speedKmph: Double
    public var courseDeg: Double
    public var sats: Int
    public var hdop: Double
    public var gnssFix: GnssFixStatus
    public var pressurePa: Double?
    public var temperatureC: Double?

    public init(t: TimeInterval, lat: Double, lon: Double, altitudeM: Double,
                speedKmph: Double, courseDeg: Double, sats: Int, hdop: Double,
                gnssFix: GnssFixStatus, pressurePa: Double? = nil, temperatureC: Double? = nil) {
        self.t = t; self.lat = lat; self.lon = lon; self.altitudeM = altitudeM
        self.speedKmph = speedKmph; self.courseDeg = courseDeg; self.sats = sats
        self.hdop = hdop; self.gnssFix = gnssFix; self.pressurePa = pressurePa; self.temperatureC = temperatureC
    }
}
