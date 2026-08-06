import Foundation

/// Herstellerneutrale Spiegelung von `CBManagerState` — hält SmartBikeCore frei von
/// CoreBluetooth (App Bible 9.7). Die On-Target-BLE-Schicht bildet `CBManagerState`
/// trivial hierauf ab und ruft dann die reine, host-testbare Abbildung unten auf.
public enum BluetoothManagerState: Sendable, Equatable {
    case unknown, resetting, unsupported, unauthorized, poweredOff, poweredOn
}

/// Phase der eigentlichen Verbindung (unabhängig vom Power-/Autorisierungszustand).
/// `connected` bedeutet: Notify-Characteristic abonniert, Frames fließen (AR-CONN-04).
public enum BluetoothLinkPhase: Sendable, Equatable {
    case idle, scanning, connecting, connected
}

/// Farb-/Dringlichkeitsstufe der Verbindungsanzeige (AR-UX-03) — UI-neutral, damit
/// SmartBikeCore SwiftUI-frei bleibt. Die View bildet die Stufe auf Theme-Farben ab.
public enum ConnectionSeverity: Sendable, Equatable {
    case connected   // verbunden — ok/grün
    case searching   // suchend/verbindend — amber
    case warning     // Bluetooth aus / keine Berechtigung — rot
    case neutral     // getrennt — dezent
}

/// Fertige, gestufte Statuszeilen-Anzeige (deutscher Text + Dringlichkeit).
public struct ConnectionDisplay: Sendable, Equatable {
    public let label: String
    public let severity: ConnectionSeverity
    public init(label: String, severity: ConnectionSeverity) {
        self.label = label; self.severity = severity
    }
}

public extension ConnectionState {
    /// Deutsche, gestufte Anzeige für die Statuszeile (AR-LIVE-02, AR-UX-03). Reine Abbildung.
    var display: ConnectionDisplay {
        switch self {
        case .connected:    return .init(label: "Verbunden", severity: .connected)
        case .scanning:     return .init(label: "Suchend", severity: .searching)
        case .connecting:   return .init(label: "Verbindend", severity: .searching)
        case .disconnected: return .init(label: "Getrennt", severity: .neutral)
        case .bluetoothOff: return .init(label: "Bluetooth aus", severity: .warning)
        case .unauthorized: return .init(label: "Keine Berechtigung", severity: .warning)
        }
    }
}

/// Reine Abbildung des BLE-Zustands auf das App-Verbindungsmodell (AR-CONN-01/07/08).
/// Host-testbar, ohne CoreBluetooth. Power-/Autorisierungszustand hat Vorrang; erst bei
/// `poweredOn` entscheidet die Verbindungsphase.
public enum ConnectionStateMapper {
    public static func connectionState(power: BluetoothManagerState,
                                       phase: BluetoothLinkPhase) -> ConnectionState {
        switch power {
        case .poweredOff:                          return .bluetoothOff       // Bluetooth aus
        case .unauthorized:                        return .unauthorized       // nicht autorisiert
        case .unknown, .resetting, .unsupported:   return .disconnected       // (noch) nicht nutzbar
        case .poweredOn:
            switch phase {
            case .idle:        return .disconnected
            case .scanning:    return .scanning     // suchend
            case .connecting:  return .connecting   // verbindend
            case .connected:   return .connected    // verbunden (subscribed)
            }
        }
    }
}
