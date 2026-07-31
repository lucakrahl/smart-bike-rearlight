import Foundation

/// Vorberechnete Fahrt-Statistik (App Bible 6.5 / 11). MVP-Kennzahlen (Project Bible 2.4).
public struct RideStatistics: Sendable, Equatable {
    public var duration: TimeInterval
    public var distanceKm: Double
    public var avgSpeedKmph: Double
    public var maxSpeedKmph: Double
    public var ascentM: Double
    public var descentM: Double
    public var minAltitudeM: Double
    public var maxAltitudeM: Double

    public init(duration: TimeInterval, distanceKm: Double, avgSpeedKmph: Double, maxSpeedKmph: Double,
                ascentM: Double, descentM: Double, minAltitudeM: Double, maxAltitudeM: Double) {
        self.duration = duration; self.distanceKm = distanceKm
        self.avgSpeedKmph = avgSpeedKmph; self.maxSpeedKmph = maxSpeedKmph
        self.ascentM = ascentM; self.descentM = descentM
        self.minAltitudeM = minAltitudeM; self.maxAltitudeM = maxAltitudeM
    }
    public static let zero = RideStatistics(duration: 0, distanceKm: 0, avgSpeedKmph: 0,
                                            maxSpeedKmph: 0, ascentM: 0, descentM: 0,
                                            minAltitudeM: 0, maxAltitudeM: 0)
}
