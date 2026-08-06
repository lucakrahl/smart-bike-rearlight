import Foundation

// Reine, host-testbare Bewertungs-/Rechenlogik für Live-Werte (AR-UX-05, AR-LIVE-03).
// Kein SwiftUI/CoreBluetooth/SwiftData — nur Werte rein, Werte raus.

/// Bewertet die Datenaktualität der Live-Werte (AR-UX-05).
public enum LiveDataEvaluator {
    /// Ab diesem Alter des letzten Frames gilt die Anzeige als veraltet (10-Hz-Quelle;
    /// > 1 s ≈ 10 fehlende Frames).
    public static let staleAfter: TimeInterval = 1.0

    /// `.fresh` nur bei aktiver Verbindung UND jungem letzten Frame; sonst `.stale`
    /// (verbunden, aber Frames alt) bzw. `.none` (nicht verbunden / noch kein Frame).
    public static func liveDataState(frameAgeSeconds: TimeInterval?,
                                     connection: ConnectionState) -> LiveDataState {
        guard connection == .connected, let age = frameAgeSeconds else { return .none }
        return age <= staleAfter ? .fresh : .stale
    }
}

/// GNSS-Gültigkeit (AR-LIVE-03): nur ein echter Fix mit Satelliten liefert gültige
/// geschwindigkeits-/positionsabgeleitete Werte.
public enum GnssValidity {
    public static func isValid(fix: GnssFixStatus, sats: Int) -> Bool {
        fix == .fixOK && sats > 0
    }
}

public extension TelemetryFrame {
    var isGnssValid: Bool { GnssValidity.isValid(fix: gnssFix, sats: Int(sats)) }
}

public extension TrackPoint {
    var isGnssValid: Bool { GnssValidity.isValid(fix: gnssFix, sats: sats) }
}

/// Barometrische Höhe aus dem Luftdruck (Standardatmosphäre).
public enum Barometer {
    /// Referenzdruck auf Meereshöhe (Pa). Kalibrierung optional/Future.
    public static let seaLevelPressurePa: Double = 101_325

    /// h = 44330 · (1 − (p/p0)^(1/5.255)); `nil` bei unplausiblem Druck (p ≤ 0).
    /// Absolutwert witterungsabhängig; relative Höhenänderungen sind korrekt.
    public static func altitudeMeters(pressurePa: Double) -> Double? {
        guard pressurePa > 0 else { return nil }
        return 44_330.0 * (1.0 - pow(pressurePa / seaLevelPressurePa, 1.0 / 5.255))
    }
}

/// Bestimmt die effektive Höhe eines Samples: bevorzugt barometrisch, sonst gültige
/// GNSS-Höhe als Fallback, sonst höhenlos (`nil`) — dann aus dem Profil zu filtern.
public enum AltitudeResolver {
    public static func altitude(baroValid: Bool, pressurePa: Double?,
                                gnssValid: Bool, gnssAltitudeM: Double?) -> Double? {
        if baroValid, let p = pressurePa, let h = Barometer.altitudeMeters(pressurePa: p) {
            return h
        }
        if gnssValid, let g = gnssAltitudeM { return g }
        return nil
    }
}
