import XCTest
@testable import SmartBikeCore

final class StatisticsEngineTests: XCTestCase {

    func testDistanceViaSpeedIntegration() {
        // Konstant 36 km/h über 100 s -> 1,0 km (36 * 100/3600).
        let points = (0...100).map { i in
            TrackPoint(t: Double(i), lat: 0, lon: 0, altitudeM: 0, speedKmph: 36,
                       courseDeg: 0, sats: 9, hdop: 1, gnssFix: .fixOK)
        }
        let stats = StatisticsEngine().computeStatistics(from: points)
        XCTAssertEqual(stats.distanceKm, 1.0, accuracy: 0.001)
        XCTAssertEqual(stats.duration, 100, accuracy: 0.001)
        XCTAssertEqual(stats.avgSpeedKmph, 36, accuracy: 0.01)
        XCTAssertEqual(stats.maxSpeedKmph, 36, accuracy: 0.01)
    }

    func testAscentDescent() {
        let points = [
            TrackPoint(t: 0, lat: 0, lon: 0, altitudeM: 100, speedKmph: 0, courseDeg: 0, sats: 9, hdop: 1, gnssFix: .fixOK),
            TrackPoint(t: 1, lat: 0, lon: 0, altitudeM: 110, speedKmph: 0, courseDeg: 0, sats: 9, hdop: 1, gnssFix: .fixOK),
            TrackPoint(t: 2, lat: 0, lon: 0, altitudeM: 105, speedKmph: 0, courseDeg: 0, sats: 9, hdop: 1, gnssFix: .fixOK),
        ]
        let stats = StatisticsEngine().computeStatistics(from: points)
        XCTAssertEqual(stats.ascentM, 10, accuracy: 0.001)
        XCTAssertEqual(stats.descentM, 5, accuracy: 0.001)
        XCTAssertEqual(stats.minAltitudeM, 100, accuracy: 0.001)
        XCTAssertEqual(stats.maxAltitudeM, 110, accuracy: 0.001)
    }

    func testEmptyIsZero() {
        XCTAssertEqual(StatisticsEngine().computeStatistics(from: []).distanceKm, 0)
    }
}
