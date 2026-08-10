import Foundation
import Observation
import SmartBikeCore

/// Schicht 3 — zentrale Live-Wahrheitsquelle (App Bible 9.1/9.2).
/// Einzige Quelle, aus der die Cockpit-ViewModels lesen.
@MainActor @Observable
final class TelemetryStore {
    private(set) var latestFrame: TelemetryFrame?
    private(set) var connection: ConnectionState = .disconnected
    private(set) var liveState: LiveDataState = .none          // AR-UX-05
    private(set) var snapshot: LiveSnapshot = .empty
    /// GNSS-Fix des letzten Frames gültig? (Fix ok + Sats > 0). Für „kein Fix"-Anzeige.
    private(set) var isGnssValid: Bool = false
    /// Aus dem letzten Frame abgeleitete System-/Sensorwarnungen (AR-LIVE-03).
    private(set) var lastWarnings: [LiveWarning] = []

    // v3-Innensicht-Status (AP4). Genau zwei binäre Größen mit sofortiger Aussage; die
    // 11 Analyse-/Aggregatfelder werden bewusst NICHT gespiegelt (bleiben in `latestFrame`
    // und gehen nur in die Persistenz). Keine Cockpit-Warn-Chips daraus (AR-UX-01): kurz
    // nach Boot ist `bias_calibrated == 0` normal — kein Fehlalarm. Anzeige erst in AP8.
    /// Stufe-1-Bias-Kalibrierung abgeschlossen (v3-Feld; `false`, wenn Feld fehlt/0).
    private(set) var biasCalibrated: Bool = false
    /// GNSS-Referenzbeschleunigung aktuell gültig (v3-Feld; `false`, wenn Feld fehlt/0).
    private(set) var gnssAccelValid: Bool = false

    /// Warnungen greifen nur bei frischer Verbindung (Stale/kein Fix sind separat behandelt).
    var warnings: [LiveWarning] { liveState == .fresh ? lastWarnings : [] }

    /// Diagnosezähler (AP2/AP8): verworfene Frames bzw. zu kurze v3-Frames (E‑1).
    /// Der Decoder ist zustandslos; das Auswerten/Hochzählen liegt hier.
    private(set) var decodeErrorCount: Int = 0
    private(set) var truncatedV3FrameCount: Int = 0

    private var lastFrameAt: Date?

    /// Wertet ein Decode-Ergebnis aus: aktualisiert Live-Wahrheit + Diagnosezähler und gibt
    /// das Frame (falls vorhanden) an den Aufrufer zurück (für die Aufzeichnung).
    @discardableResult
    func consume(_ result: DecodeResult) -> TelemetryFrame? {
        switch result {
        case .ok(let frame):
            apply(frame); return frame
        case .truncatedV3(let frame):
            truncatedV3FrameCount += 1; apply(frame); return frame   // E‑1: kein Fehler
        case .rejected:
            decodeErrorCount += 1; return nil
        }
    }

    /// Wird vom Decode-Consumer aufgerufen (10 Hz). Schreibt die Live-Momentaufnahme
    /// fort; Aufzeichnungs-Aggregate (Distanz/Zeit/Ø/Max) folgen über den RideManager.
    func apply(_ frame: TelemetryFrame) {
        latestFrame = frame
        lastFrameAt = Date()
        isGnssValid = frame.isGnssValid
        biasCalibrated = (frame.biasCalibrated == 1)     // v3-Status; nil/0 → false
        gnssAccelValid = (frame.gnssAccelValid == 1)
        lastWarnings = SystemWarnings.derive(from: frame)
        var s = snapshot
        s.speedKmph = Double(frame.speedKmph)
        // Höhe barometrisch (Fallback GNSS bei gültigem Fix); für die Live-Anzeige 0, wenn höhenlos.
        s.altitudeM = AltitudeResolver.altitude(baroValid: frame.baroValid,
                                                pressurePa: Double(frame.pressurePa),
                                                gnssValid: frame.isGnssValid,
                                                gnssAltitudeM: Double(frame.altitudeM)) ?? 0
        s.courseDeg = Double(frame.courseDeg)
        s.sats = Int(frame.sats)
        s.hdop = Double(frame.hdop)
        s.isConnected = (connection == .connected)
        snapshot = s
        evaluateFreshness(now: Date())
    }

    func update(connection: ConnectionState) {
        self.connection = connection
        evaluateFreshness(now: Date())
    }

    /// Datenaktualität aus Frame-Alter + Verbindung neu bestimmen (AR-UX-05). Wird
    /// zusätzlich periodisch aufgerufen, damit „veraltet" auch ohne neue Frames greift.
    func evaluateFreshness(now: Date) {
        let age = lastFrameAt.map { now.timeIntervalSince($0) }
        liveState = LiveDataEvaluator.liveDataState(frameAgeSeconds: age, connection: connection)
    }
}
