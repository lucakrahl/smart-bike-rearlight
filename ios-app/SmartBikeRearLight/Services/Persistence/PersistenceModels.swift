import Foundation
import SwiftData

/// SwiftData-@Model-Klassen (App-Target). Werden von/zu den reinen Core-Typen
/// gemappt (App Bible Kap. 11). NICHT im Core-Package (SwiftData-abhängig).
///
/// TODO (Xcode/Claude Code): Felder finalisieren + Mapping zu TrackPoint/RideStatistics.
@Model final class RideEntity {
    var id: UUID
    var startedAt: Date
    var endedAt: Date?
    var status: String            // "recording" | "finished"
    // eingebettete Statistik
    var distanceKm: Double
    var duration: Double
    var avgSpeedKmph: Double
    var maxSpeedKmph: Double
    var ascentM: Double
    var descentM: Double
    var minAltitudeM: Double
    var maxAltitudeM: Double
    @Relationship(deleteRule: .cascade, inverse: \TrackSampleEntity.ride)
    var samples: [TrackSampleEntity]

    init(id: UUID = UUID(), startedAt: Date, status: String = "recording") {
        self.id = id; self.startedAt = startedAt; self.status = status
        self.endedAt = nil; self.distanceKm = 0; self.duration = 0
        self.avgSpeedKmph = 0; self.maxSpeedKmph = 0; self.ascentM = 0; self.descentM = 0
        self.minAltitudeM = 0; self.maxAltitudeM = 0; self.samples = []
    }
}

@Model final class TrackSampleEntity {
    var t: Double
    var lat: Double
    var lon: Double
    var altitudeM: Double
    var speedKmph: Double
    var courseDeg: Double
    var sats: Int
    var hdop: Double
    var gnssFix: Int
    var ride: RideEntity?

    init(t: Double, lat: Double, lon: Double, altitudeM: Double, speedKmph: Double,
         courseDeg: Double, sats: Int, hdop: Double, gnssFix: Int) {
        self.t = t; self.lat = lat; self.lon = lon; self.altitudeM = altitudeM
        self.speedKmph = speedKmph; self.courseDeg = courseDeg; self.sats = sats
        self.hdop = hdop; self.gnssFix = gnssFix
    }
}

@Model final class BLEDeviceEntity {
    var peripheralID: String
    var name: String
    var lastConnected: Date?
    init(peripheralID: String, name: String, lastConnected: Date? = nil) {
        self.peripheralID = peripheralID; self.name = name; self.lastConnected = lastConnected
    }
}
