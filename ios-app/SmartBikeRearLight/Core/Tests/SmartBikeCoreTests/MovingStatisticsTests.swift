import Testing
import Foundation
@testable import SmartBikeCore

/// Fahrzeit/Ø nach „bewegt"-Definition (Stopps ausgeschlossen, Schwelle, ohne Fix nichts).
struct MovingStatisticsTests {

    private func point(t: Double, speed: Double, fix: GnssFixStatus = .fixOK) -> TrackPoint {
        TrackPoint(t: t, lat: 0, lon: 0, altitudeM: nil, speedKmph: speed, courseDeg: 0,
                   sats: fix == .fixOK ? 9 : 0, hdop: 1, gnssFix: fix)
    }

    @Test func movingTimeExcludesStop() {
        // 1 s-Takt; bei t=2 s steht das Rad (0 km/h).
        let points = [
            point(t: 0, speed: 10), point(t: 1, speed: 10), point(t: 2, speed: 0),
            point(t: 3, speed: 10), point(t: 4, speed: 10),
        ]
        let s = StatisticsEngine().computeStatistics(from: points)
        // Bewegte Intervalle (cur ≥ Schwelle): t0→1, t2→3, t3→4 = 3 s; Stopp-Intervall t1→2 fällt weg.
        #expect(abs(s.duration - 3.0) < 1e-9)
        #expect(abs(s.totalDuration - 4.0) < 1e-9)   // Gesamt bleibt erhalten
    }

    @Test func thresholdBoundary() {
        // cur = 1,0 zählt (≥ Schwelle), cur = 0,9 nicht.
        let points = [point(t: 0, speed: 2), point(t: 1, speed: 1.0), point(t: 2, speed: 0.9)]
        let s = StatisticsEngine().computeStatistics(from: points)
        #expect(abs(s.duration - 1.0) < 1e-9)         // nur t0→1
        #expect(abs(s.totalDuration - 2.0) < 1e-9)
    }

    @Test func noFixCountsNoMovement() {
        let points = [point(t: 0, speed: 10, fix: .noFix), point(t: 1, speed: 10, fix: .noFix)]
        let s = StatisticsEngine().computeStatistics(from: points)
        #expect(s.duration == 0)
        #expect(s.distanceKm == 0)
        #expect(s.avgSpeedKmph == 0)
    }

    @Test func movingAverageIsMeanOfMovingSamples() {
        // Drei Bewegt-Samples (36) + Stopp. Ø bewegt = 36 (unabhängig vom Stopp);
        // über die Gesamtzeit wäre der Schnitt niedriger.
        let points = [
            point(t: 0, speed: 36), point(t: 1, speed: 36), point(t: 2, speed: 36),
            point(t: 3, speed: 0), point(t: 4, speed: 0),
        ]
        let s = StatisticsEngine().computeStatistics(from: points)
        #expect(abs(s.duration - 2.0) < 1e-9)         // Bewegt-Intervalle t0→1, t1→2 (cur=36)
        #expect(abs(s.totalDuration - 4.0) < 1e-9)
        #expect(abs(s.avgSpeedKmph - 36.0) < 0.001)   // Mittel der Bewegt-Samples
    }
}
