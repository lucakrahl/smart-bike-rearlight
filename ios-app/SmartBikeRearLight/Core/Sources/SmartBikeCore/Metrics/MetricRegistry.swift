import Foundation

/// Momentaufnahme der Live-Werte, aus der die Kacheln gespeist werden.
/// Der TelemetryStore (App-Target) füllt diese Struktur (rein & testbar).
public struct LiveSnapshot: Sendable, Equatable {
    public var speedKmph, avgSpeedKmph, maxSpeedKmph: Double
    public var distanceKm: Double
    public var duration: TimeInterval
    public var altitudeM, ascentM, descentM, courseDeg: Double
    public var sats: Int
    public var hdop: Double
    public var isConnected: Bool
    public init(speedKmph: Double = 0, avgSpeedKmph: Double = 0, maxSpeedKmph: Double = 0,
                distanceKm: Double = 0, duration: TimeInterval = 0, altitudeM: Double = 0,
                ascentM: Double = 0, descentM: Double = 0, courseDeg: Double = 0,
                sats: Int = 0, hdop: Double = 0, isConnected: Bool = false) {
        self.speedKmph = speedKmph; self.avgSpeedKmph = avgSpeedKmph; self.maxSpeedKmph = maxSpeedKmph
        self.distanceKm = distanceKm; self.duration = duration; self.altitudeM = altitudeM
        self.ascentM = ascentM; self.descentM = descentM; self.courseDeg = courseDeg
        self.sats = sats; self.hdop = hdop; self.isConnected = isConnected
    }
    public static let empty = LiveSnapshot()
}

public struct MetricDisplay: Sendable, Equatable {
    public var label: String
    public var value: String
    public var unit: String
    public init(label: String, value: String, unit: String) {
        self.label = label; self.value = value; self.unit = unit
    }
}

/// Bildet `MetricID → Anzeige` (AR-LIVE-09). Erweiterbar ohne View-Änderung.
public struct MetricRegistry: Sendable {
    public init() {}

    public func display(_ id: MetricID, from s: LiveSnapshot) -> MetricDisplay {
        func f(_ v: Double, _ dec: Int) -> String { String(format: "%.\(dec)f", v) }
        switch id {
        case .speed:      return .init(label: "Geschwindigkeit", value: f(s.speedKmph, 1), unit: "km/h")
        case .avgSpeed:   return .init(label: "Ø-Geschwindigkeit", value: f(s.avgSpeedKmph, 1), unit: "km/h")
        case .maxSpeed:   return .init(label: "Max", value: f(s.maxSpeedKmph, 1), unit: "km/h")
        case .distance:   return .init(label: "Distanz", value: f(s.distanceKm, 2), unit: "km")
        case .duration:   return .init(label: "Fahrzeit", value: Self.hms(s.duration), unit: "")
        case .altitude:   return .init(label: "Höhe", value: f(s.altitudeM, 0), unit: "m")
        case .ascent:     return .init(label: "↑ Höhenmeter", value: f(s.ascentM, 0), unit: "m")
        case .descent:    return .init(label: "↓ Höhenmeter", value: f(s.descentM, 0), unit: "m")
        case .course:     return .init(label: "Kurs", value: f(s.courseDeg, 0), unit: "°")
        case .sats:       return .init(label: "Satelliten", value: "\(s.sats)", unit: "")
        case .hdop:       return .init(label: "HDOP", value: f(s.hdop, 1), unit: "")
        case .connection: return .init(label: "Verbindung", value: s.isConnected ? "Verbunden" : "Getrennt", unit: "")
        }
    }

    static func hms(_ t: TimeInterval) -> String {
        let s = Int(t); return String(format: "%02d:%02d:%02d", s / 3600, (s % 3600) / 60, s % 60)
    }
}
