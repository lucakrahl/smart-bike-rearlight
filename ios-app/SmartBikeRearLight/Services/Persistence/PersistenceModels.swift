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
    var duration: Double        // bewegte Fahrzeit
    var totalDuration: Double?  // reine Gesamt-Aufzeichnungszeit (additiv)
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
        self.endedAt = nil; self.distanceKm = 0; self.duration = 0; self.totalDuration = nil
        self.avgSpeedKmph = 0; self.maxSpeedKmph = 0; self.ascentM = 0; self.descentM = 0
        self.minAltitudeM = 0; self.maxAltitudeM = 0; self.samples = []
    }
}

@Model final class TrackSampleEntity {
    var t: Double
    var lat: Double
    var lon: Double
    /// Effektive Höhe (baro bevorzugt); `nil` = höhenlos (kein Baro, kein gültiger Fix).
    var altitudeM: Double?
    var speedKmph: Double
    var courseDeg: Double
    var sats: Int
    var hdop: Double
    var gnssFix: Int
    /// Bremslicht-Validierung (v2) + IMU-Gesundheit (Fail-Safe-Kennzeichnung).
    var brakeDecelMs2: Double?
    var brakeLightPct: Int?
    var imuHealth: Int?
    /// Rohwerte fürs Nachvollziehen: Luftdruck (Baro-Quelle) + GNSS-Höhe als Referenz.
    var pressurePa: Double?
    var gnssAltitudeM: Double?
    /// Temperatur (°C). Bleibt persistiert (AP5); nur der CSV-Export lässt sie später fallen (AP7).
    var temperatureC: Double?
    /// Weitere Rohfelder (Validierung/Export). `deviceTimestampMs`/`systemState`/`frameVersion`
    /// als Int gespeichert (SwiftData-freundlich); Mapping in `SwiftDataStore`.
    var deviceTimestampMs: Int?
    var baroValid: Bool?
    var systemState: Int?
    var initDegraded: Bool?
    var watchdogRecovered: Bool?
    var frameVersion: Int?
    /// v3-Analyse-/Aggregatfelder (additiv, optional → leichtgewichtige Migration; Altdaten `nil`).
    var gnssAccelMs2: Double?
    var pitchRad: Double?
    var gyroBiasRads: Double?
    var normDeltaMin: Double?
    var normDeltaMax: Double?
    var jerkMax: Double?
    var regimeStaticN: Int?
    var regimeDynamicN: Int?
    var regimeShockN: Int?
    var biasCalibrated: Bool?
    var gnssAccelValid: Bool?
    var dtMaxMs: Int?
    var loopMaxUs: Int?
    var ride: RideEntity?

    init(t: Double, lat: Double, lon: Double, altitudeM: Double?, speedKmph: Double,
         courseDeg: Double, sats: Int, hdop: Double, gnssFix: Int,
         brakeDecelMs2: Double? = nil, brakeLightPct: Int? = nil, imuHealth: Int? = nil,
         pressurePa: Double? = nil, gnssAltitudeM: Double? = nil, temperatureC: Double? = nil,
         deviceTimestampMs: Int? = nil, baroValid: Bool? = nil, systemState: Int? = nil,
         initDegraded: Bool? = nil, watchdogRecovered: Bool? = nil, frameVersion: Int? = nil,
         gnssAccelMs2: Double? = nil, pitchRad: Double? = nil, gyroBiasRads: Double? = nil,
         normDeltaMin: Double? = nil, normDeltaMax: Double? = nil, jerkMax: Double? = nil,
         regimeStaticN: Int? = nil, regimeDynamicN: Int? = nil, regimeShockN: Int? = nil,
         biasCalibrated: Bool? = nil, gnssAccelValid: Bool? = nil,
         dtMaxMs: Int? = nil, loopMaxUs: Int? = nil) {
        self.t = t; self.lat = lat; self.lon = lon; self.altitudeM = altitudeM
        self.speedKmph = speedKmph; self.courseDeg = courseDeg; self.sats = sats
        self.hdop = hdop; self.gnssFix = gnssFix
        self.brakeDecelMs2 = brakeDecelMs2; self.brakeLightPct = brakeLightPct; self.imuHealth = imuHealth
        self.pressurePa = pressurePa; self.gnssAltitudeM = gnssAltitudeM; self.temperatureC = temperatureC
        self.deviceTimestampMs = deviceTimestampMs; self.baroValid = baroValid
        self.systemState = systemState; self.initDegraded = initDegraded
        self.watchdogRecovered = watchdogRecovered; self.frameVersion = frameVersion
        self.gnssAccelMs2 = gnssAccelMs2; self.pitchRad = pitchRad; self.gyroBiasRads = gyroBiasRads
        self.normDeltaMin = normDeltaMin; self.normDeltaMax = normDeltaMax; self.jerkMax = jerkMax
        self.regimeStaticN = regimeStaticN; self.regimeDynamicN = regimeDynamicN; self.regimeShockN = regimeShockN
        self.biasCalibrated = biasCalibrated; self.gnssAccelValid = gnssAccelValid
        self.dtMaxMs = dtMaxMs; self.loopMaxUs = loopMaxUs
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
