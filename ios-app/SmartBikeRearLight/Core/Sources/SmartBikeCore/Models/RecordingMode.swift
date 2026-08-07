import Foundation

/// Aufzeichnungsrate (AR-DATA-02, AP6). Reiner Werttyp, UI-frei.
/// `hz1` ist der Standard (1 Datenpunkt/s); `hz10` dient Validierungsfahrten mit
/// höherer Auflösung — und deutlich höherem Speicherbedarf. Der `rawValue` ist die
/// Frequenz in Hz, sodass Persistenz (UserDefaults-Int) und Ableitungen trivial sind.
public enum RecordingMode: Int, Sendable, CaseIterable, Codable {
    case hz1 = 1     // Standard — 1 Hz
    case hz10 = 10   // Validierung — 10 Hz

    /// Frequenz in Hz (== rawValue).
    public var hz: Int { rawValue }

    /// Soll-Abstand zweier Samples in Sekunden (1 Hz → 1,0 s; 10 Hz → 0,1 s).
    public var sampleInterval: TimeInterval { 1.0 / Double(rawValue) }

    /// Decimations-Bucket eines Zeitstempels: `floor(t · hz)`. Zwei Frames mit gleichem
    /// Bucket werden auf ein Sample verdichtet. Bei `hz1` identisch zur bisherigen
    /// Ganzsekunden-Logik `Int(t)` — Default-Verhalten bleibt unverändert.
    ///
    /// Das `+1e-9` kompensiert die Float-Akkumulation der Aufzeichnungsuhr
    /// (`elapsed = Σ 0,1 s ≈ n·0,1 − ε`): ohne diesen Nudge fiele ein Sample knapp unter
    /// einer Fenstergrenze (z. B. 0,7999…) in den Vorgänger-Bucket und ginge bei 10 Hz
    /// verloren (E-4). Der Nudge (1e-9 ≫ der Float-Fehler ~1e-16, aber unmerklich klein)
    /// lässt die floor-basierten Fenster unverändert.
    public func bucket(for t: TimeInterval) -> Int {
        Int(t * Double(rawValue) + 1e-9)
    }
}
