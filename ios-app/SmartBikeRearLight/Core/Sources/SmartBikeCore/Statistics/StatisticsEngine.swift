import Foundation

/// Reine Statistik-Berechnung (AR-LIVE-07, AR-STAT/VIS). Host-testbar.
/// Distanz = Integration von speed_kmph über die Zeit (robust gegen float32-
/// Quantisierung von lat/lon — App Bible Decision Log).
public struct StatisticsEngine: Sendable {
    /// Totzone gegen Rausch-Höhenmeter beim Zählen von Auf-/Abstieg (baro-Rauschen ~1 m).
    public static let altitudeDeadbandM: Double = 1.0
    /// Ab dieser Geschwindigkeit (km/h) gilt ein Sample als „bewegt" (Fahrzeit/Ø).
    public static let movingSpeedThresholdKmh: Double = 1.0

    public init() {}

    public func computeStatistics(from points: [TrackPoint]) -> RideStatistics {
        guard points.count >= 2, let first = points.first, let last = points.last else {
            let a = points.compactMap(\.altitudeM).first ?? 0
            return RideStatistics(duration: 0, distanceKm: 0, avgSpeedKmph: 0, maxSpeedKmph: 0,
                                  ascentM: 0, descentM: 0, minAltitudeM: a, maxAltitudeM: a)
        }
        var distanceKm = 0.0, maxSpeed = 0.0, ascent = 0.0, descent = 0.0, movingTime = 0.0
        for i in 1..<points.count {
            let prev = points[i - 1], cur = points[i]
            let dt = cur.t - prev.t                              // s
            // Distanz/Geschwindigkeit nur bei gültigem Fix werten (AR-LIVE-03) — ohne Fix
            // wächst die Strecke nicht.
            if prev.isGnssValid && cur.isGnssValid {
                let vAvg = (prev.speedKmph + cur.speedKmph) / 2.0
                distanceKm += vAvg * (dt / 3600.0)
                maxSpeed = max(maxSpeed, cur.speedKmph)
            }
            // Bewegte Fahrzeit: nur Intervalle mit gültigem Fix und Speed ≥ Schwelle
            // (Stopps/Ampeln zählen nicht).
            if cur.isGnssValid && cur.speedKmph >= Self.movingSpeedThresholdKmh {
                movingTime += dt
            }
            // Höhenmeter nur zwischen Punkten mit gültiger Höhe, mit Totzone.
            if let a0 = prev.altitudeM, let a1 = cur.altitudeM {
                let dAlt = a1 - a0
                if dAlt > Self.altitudeDeadbandM { ascent += dAlt }
                else if dAlt < -Self.altitudeDeadbandM { descent += -dAlt }
            }
        }
        let alts = points.compactMap(\.altitudeM)
        let minAlt = alts.min() ?? 0
        let maxAlt = alts.max() ?? 0
        let totalDuration = last.t - first.t
        // Bewegter Schnitt = Mittel der Geschwindigkeiten der Bewegt-Samples (robust gegen
        // Brems-/Beschleunigungssegmente, die die Schwelle kreuzen).
        let movingSamples = points.filter { $0.isGnssValid && $0.speedKmph >= Self.movingSpeedThresholdKmh }
        let avg = movingSamples.isEmpty ? 0 : movingSamples.map(\.speedKmph).reduce(0, +) / Double(movingSamples.count)
        return RideStatistics(duration: movingTime, totalDuration: totalDuration,
                              distanceKm: distanceKm, avgSpeedKmph: avg,
                              maxSpeedKmph: maxSpeed, ascentM: ascent, descentM: descent,
                              minAltitudeM: minAlt, maxAltitudeM: maxAlt)
    }

    /// Kumulierte Distanz (km) bis zu jedem Punkt — gleiche Geschwindigkeits-
    /// Integration wie `computeStatistics` (robust gegen float32-Quantisierung von
    /// lat/lon). Ergebnis hat dieselbe Länge wie `points`; das erste Element ist 0.
    /// X-Achse für die Detaildiagramme (AR-VIS-*).
    public func cumulativeDistanceKm(for points: [TrackPoint]) -> [Double] {
        guard !points.isEmpty else { return [] }
        var result = [Double](repeating: 0, count: points.count)
        var acc = 0.0
        for i in 1..<points.count {
            let prev = points[i - 1], cur = points[i]
            if prev.isGnssValid && cur.isGnssValid {          // ohne Fix kein Zuwachs (bleibt flach)
                let dt = cur.t - prev.t
                let vAvg = (prev.speedKmph + cur.speedKmph) / 2.0
                acc += vAvg * (dt / 3600.0)
            }
            result[i] = acc
        }
        return result
    }
}
