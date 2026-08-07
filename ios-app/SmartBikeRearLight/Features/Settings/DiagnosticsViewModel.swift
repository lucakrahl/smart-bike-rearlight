import Foundation
import Observation
import SmartBikeCore

/// Read-only Diagnose-Ableitung (AP8). Reiner Werttyp → host-testbar ohne TelemetryStore.
/// Bündelt die v3-Innensicht (`bias_calibrated`, `gnss_accel_valid`), das Zeitbudget des
/// letzten Frames (`dt_max_ms`, `loop_max_us`) und den E-1-Defektzähler.
struct DiagnosticsReadout: Equatable {
    let biasCalibrated: Bool
    let gnssAccelValid: Bool
    /// Direkt aus dem letzten Frame; `nil` = kein (v3-)Frame bzw. Wert fehlt.
    let dtMaxMs: Int?
    let loopMaxUs: Int?
    /// Diagnosezähler aus dem TelemetryStore: zu kurze v3-Frames (E-1).
    let truncatedV3FrameCount: Int

    /// Defekt-Indikator: > 0 = zu kurze v3-Frames empfangen (E-1).
    var hasTruncatedDefect: Bool { truncatedV3FrameCount > 0 }

    var dtMaxDisplay: String { dtMaxMs.map { "\($0) ms" } ?? "—" }
    var loopMaxDisplay: String { loopMaxUs.map { "\($0) µs" } ?? "—" }
}

/// ViewModel der Diagnoseansicht (Schicht 8): leitet die read-only Diagnosegrößen aus dem
/// TelemetryStore ab. Keine Cockpit-Größe, kein Schreibpfad, kein Netzwerk (AR-UX-01).
@MainActor @Observable
final class DiagnosticsViewModel {
    private let store: TelemetryStore
    init(store: TelemetryStore) { self.store = store }

    var readout: DiagnosticsReadout {
        DiagnosticsReadout(
            biasCalibrated: store.biasCalibrated,
            gnssAccelValid: store.gnssAccelValid,
            dtMaxMs: store.latestFrame?.dtMaxMs.map(Int.init),
            loopMaxUs: store.latestFrame?.loopMaxUs.map(Int.init),
            truncatedV3FrameCount: store.truncatedV3FrameCount
        )
    }
}
