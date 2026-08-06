import Foundation

/// Monotone Aufzeichnungsuhr (rein, host-testbar). Leitet aus dem NICHT-monotonen
/// Geräte-Zeitstempel `timestamp_ms` eine springfreie Aufzeichnungszeit ab:
/// erkennt Resets (Rücklicht-Neustart → Zeitstempel springt auf 0) und kappt große
/// Zeitschritte (Funkabriss/Lücke), damit Dauer und Distanz nie aufgebläht werden.
/// Der BLE-Vertrag/Decoder bleibt unberührt — das ist reine Zeitlogik im App-Kern.
public struct RecordingClock: Sendable, Equatable {
    /// Zuletzt gesehener roher Geräte-Zeitstempel (ms). `nil` = noch kein Sample.
    public private(set) var lastRawMs: UInt32?
    /// Monotone Aufzeichnungszeit (s seit Fahrtbeginn) — steigt nie sprunghaft.
    public private(set) var elapsed: TimeInterval

    /// `elapsed` startet bei 0 (neue Fahrt) oder beim letzten `t` (Fortsetzen, AR-DATA-04).
    public init(elapsed: TimeInterval = 0) {
        self.lastRawMs = nil
        self.elapsed = elapsed
    }
}

/// Ergebnis eines Uhr-Schritts.
public struct RecordingClockStep: Sendable, Equatable {
    /// Neue monotone Zeit `t` für dieses Sample.
    public let time: TimeInterval
    /// Gekappter, nicht-negativer Zeitschritt für die Distanzintegration (Σ v·dt).
    public let dt: TimeInterval
    /// Geräte-Uhr-Reset erkannt (neuer Zeitstempel < letzter).
    public let didReset: Bool
    /// `false` → Sample verwerfen (dt ≤ 0, kein Fortschritt / Duplikat).
    public let accepted: Bool
}

public extension RecordingClock {
    /// Obergrenze eines einzelnen Zeitschritts. Kappt Lücken/Resets, sodass eine
    /// unbekannte Pause niemals Dauer oder Distanz aufbläht.
    static let maxSampleDt: TimeInterval = 1.5

    /// Reine Fortschreibung: aktueller Zustand + neuer roher Zeitstempel → neuer
    /// Zustand + Schritt. Reihenfolge der Fälle: erstes Sample (Anker) · Reset
    /// (rawMs < last) · Duplikat (dt = 0) · Normal/Backfill (dt = rawDt, gekappt).
    func advanced(to rawMs: UInt32) -> (RecordingClock, RecordingClockStep) {
        guard let last = lastRawMs else {
            var next = self
            next.lastRawMs = rawMs
            return (next, RecordingClockStep(time: elapsed, dt: 0, didReset: false, accepted: true))
        }

        // Reset: Geräte-Uhr zurückgesprungen. Reale Lücke unbekannt → dt kappen.
        if rawMs < last {
            let dt = Self.maxSampleDt
            var next = self
            next.lastRawMs = rawMs
            next.elapsed = elapsed + dt
            return (next, RecordingClockStep(time: next.elapsed, dt: dt, didReset: true, accepted: true))
        }

        let rawDt = TimeInterval(rawMs - last) / 1000.0   // rawMs ≥ last → kein Underflow
        if rawDt <= 0 {                                    // gleicher Zeitstempel → Duplikat
            var next = self
            next.lastRawMs = rawMs
            return (next, RecordingClockStep(time: elapsed, dt: 0, didReset: false, accepted: false))
        }

        // Normalfall & Backfill: kleine, fortlaufende Schritte bleiben exakt; nur eine
        // echte Lücke (rawDt groß) wird auf maxSampleDt gekappt.
        let dt = min(rawDt, Self.maxSampleDt)
        var next = self
        next.lastRawMs = rawMs
        next.elapsed = elapsed + dt
        return (next, RecordingClockStep(time: next.elapsed, dt: dt, didReset: false, accepted: true))
    }
}
