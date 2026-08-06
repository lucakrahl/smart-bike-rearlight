import Testing
import Foundation
@testable import SmartBikeCore

/// Stale-/Gültig-Bewertung + barometrische Höhe (AR-UX-05, AR-LIVE-03).
struct LiveEvaluatorsTests {

    // MARK: LiveDataState

    @Test func liveDataFreshWhenConnectedAndRecent() {
        #expect(LiveDataEvaluator.liveDataState(frameAgeSeconds: 0.1, connection: .connected) == .fresh)
    }

    @Test func liveDataStaleWhenConnectedButOld() {
        #expect(LiveDataEvaluator.liveDataState(frameAgeSeconds: 3.0, connection: .connected) == .stale)
    }

    @Test func liveDataNoneWhenDisconnectedOrNoFrame() {
        #expect(LiveDataEvaluator.liveDataState(frameAgeSeconds: 0.1, connection: .disconnected) == .none)
        #expect(LiveDataEvaluator.liveDataState(frameAgeSeconds: 0.1, connection: .scanning) == .none)
        #expect(LiveDataEvaluator.liveDataState(frameAgeSeconds: nil, connection: .connected) == .none)
    }

    // MARK: GNSS-Gültigkeit

    @Test func gnssValidOnlyWithFixAndSats() {
        #expect(GnssValidity.isValid(fix: .fixOK, sats: 9))
        #expect(!GnssValidity.isValid(fix: .noFix, sats: 9))   // kein Fix
        #expect(!GnssValidity.isValid(fix: .noData, sats: 9))
        #expect(!GnssValidity.isValid(fix: .fixOK, sats: 0))   // 0 Sats
    }

    // MARK: Barometrische Höhe

    @Test func barometricAltitudeKnownValues() {
        // Meereshöhen-Referenzdruck → 0 m.
        #expect(abs(Barometer.altitudeMeters(pressurePa: 101_325)! - 0) < 0.001)
        // 100000 Pa → ~111 m (Standardatmosphäre).
        #expect(abs(Barometer.altitudeMeters(pressurePa: 100_000)! - 110.9) < 1.0)
        // Unplausibel → nil.
        #expect(Barometer.altitudeMeters(pressurePa: 0) == nil)
    }

    @Test func barometricAltitudeIsMonotonicallyDecreasingInPressure() {
        // Fällt der Druck, steigt die Höhe.
        let high = Barometer.altitudeMeters(pressurePa: 90_000)!
        let mid  = Barometer.altitudeMeters(pressurePa: 100_000)!
        let low  = Barometer.altitudeMeters(pressurePa: 101_325)!
        #expect(high > mid)
        #expect(mid > low)
    }

    // MARK: Höhen-Auflösung (Baro bevorzugt, GNSS-Fallback, sonst höhenlos)

    @Test func altitudeResolverPrefersBaroThenGnssThenNil() {
        // Baro gültig → barometrisch.
        #expect(AltitudeResolver.altitude(baroValid: true, pressurePa: 101_325,
                                          gnssValid: true, gnssAltitudeM: 250) == 0)
        // Baro ungültig, GNSS gültig → GNSS-Höhe.
        #expect(AltitudeResolver.altitude(baroValid: false, pressurePa: 101_325,
                                          gnssValid: true, gnssAltitudeM: 250) == 250)
        // Beides ungültig → höhenlos.
        #expect(AltitudeResolver.altitude(baroValid: false, pressurePa: nil,
                                          gnssValid: false, gnssAltitudeM: 250) == nil)
    }
}
