import Foundation

/// System-/Sensorwarnung aus den Firmware-Statusfeldern (AR-LIVE-03). Rein & host-testbar.
public struct LiveWarning: Sendable, Equatable, Identifiable {
    /// Dringlichkeit; `Comparable` nach Deklarationsreihenfolge → `.warnung > .info`.
    public enum Severity: Sendable, Equatable, Comparable { case info, warnung }

    public let id: String
    public let severity: Severity
    public let text: String

    public init(id: String, severity: Severity, text: String) {
        self.id = id; self.severity = severity; self.text = text
    }
}

/// Leitet die aktuellen Warnungen aus einem Frame ab (nur Firmware-Statusfelder).
public enum SystemWarnings {
    /// Ergebnis ist nach Dringlichkeit sortiert (Warnung vor Info); „alles OK" → leer.
    public static func derive(from frame: TelemetryFrame) -> [LiveWarning] {
        var warnings: [LiveWarning] = []

        switch frame.imuHealth {
        case .failed:
            warnings.append(.init(id: "imu_failed", severity: .warnung, text: "IMU ausgefallen"))
        case .recovering:
            warnings.append(.init(id: "imu_recovering", severity: .info, text: "IMU stabilisiert sich"))
        case .ok:
            break
        }
        if frame.initDegraded {
            warnings.append(.init(id: "init_degraded", severity: .info, text: "Start eingeschränkt"))
        }
        if frame.watchdogRecovered {
            warnings.append(.init(id: "watchdog_recovered", severity: .info, text: "System neu gestartet"))
        }
        if frame.systemState == .initializing {
            warnings.append(.init(id: "starting", severity: .info, text: "Rücklicht startet"))
        }
        if !frame.baroValid {
            warnings.append(.init(id: "baro_invalid", severity: .info, text: "Höhensensor ohne Werte"))
        }

        // Warnung vor Info (höchste Severity zuerst); Reihenfolge innerhalb gleicher Stufe bleibt.
        return warnings.sorted { $0.severity > $1.severity }
    }
}
