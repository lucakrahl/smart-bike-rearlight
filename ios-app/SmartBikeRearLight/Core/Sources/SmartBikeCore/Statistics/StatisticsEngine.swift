import Foundation

/// Reine Statistik-Berechnung (AR-LIVE-07, AR-STAT/VIS). Host-testbar.
/// Distanz = Integration von speed_kmph über die Zeit (robust gegen float32-
/// Quantisierung von lat/lon — App Bible Decision Log).
public struct StatisticsEngine: Sendable {
    public init() {}

    public func computeStatistics(from points: [TrackPoint]) -> RideStatistics {
        guard points.count >= 2, let first = points.first, let last = points.last else {
            let a = points.first?.altitudeM ?? 0
            return RideStatistics(duration: 0, distanceKm: 0, avgSpeedKmph: 0, maxSpeedKmph: 0,
                                  ascentM: 0, descentM: 0, minAltitudeM: a, maxAltitudeM: a)
        }
        var distanceKm = 0.0, maxSpeed = 0.0, ascent = 0.0, descent = 0.0
        var minAlt = first.altitudeM, maxAlt = first.altitudeM
        for i in 1..<points.count {
            let prev = points[i - 1], cur = points[i]
            let dt = cur.t - prev.t                              // s
            let vAvg = (prev.speedKmph + cur.speedKmph) / 2.0    // km/h
            distanceKm += vAvg * (dt / 3600.0)
            maxSpeed = max(maxSpeed, cur.speedKmph)
            let dAlt = cur.altitudeM - prev.altitudeM
            if dAlt > 0 { ascent += dAlt } else { descent += -dAlt }   // TODO: Glättung (Rausch-Schwelle)
            minAlt = min(minAlt, cur.altitudeM)
            maxAlt = max(maxAlt, cur.altitudeM)
        }
        let duration = last.t - first.t
        let avg = duration > 0 ? distanceKm / (duration / 3600.0) : 0
        return RideStatistics(duration: duration, distanceKm: distanceKm, avgSpeedKmph: avg,
                              maxSpeedKmph: maxSpeed, ascentM: ascent, descentM: descent,
                              minAltitudeM: minAlt, maxAltitudeM: maxAlt)
    }
}
