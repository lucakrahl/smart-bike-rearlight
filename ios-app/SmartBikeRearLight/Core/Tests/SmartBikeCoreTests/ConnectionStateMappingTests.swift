import Testing
@testable import SmartBikeCore

/// Deckt die reine Abbildung BLE-Zustand → `ConnectionState` ab (AR-CONN-01/07/08).
/// Alle `CBManagerState`-Entsprechungen + Verbindungsphasen bei poweredOn.
struct ConnectionStateMappingTests {

    @Test func powerStatesTakePrecedence() {
        // Power-/Autorisierungszustand dominiert — Phase ist dann irrelevant.
        #expect(ConnectionStateMapper.connectionState(power: .poweredOff, phase: .connected) == .bluetoothOff)
        #expect(ConnectionStateMapper.connectionState(power: .unauthorized, phase: .scanning) == .unauthorized)
        #expect(ConnectionStateMapper.connectionState(power: .unknown, phase: .connecting) == .disconnected)
        #expect(ConnectionStateMapper.connectionState(power: .resetting, phase: .connected) == .disconnected)
        #expect(ConnectionStateMapper.connectionState(power: .unsupported, phase: .scanning) == .disconnected)
    }

    @Test func phaseDecidesWhenPoweredOn() {
        #expect(ConnectionStateMapper.connectionState(power: .poweredOn, phase: .idle) == .disconnected)
        #expect(ConnectionStateMapper.connectionState(power: .poweredOn, phase: .scanning) == .scanning)
        #expect(ConnectionStateMapper.connectionState(power: .poweredOn, phase: .connecting) == .connecting)
        #expect(ConnectionStateMapper.connectionState(power: .poweredOn, phase: .connected) == .connected)
    }

    @Test func displayTextAndSeverityForEachState() {
        #expect(ConnectionState.disconnected.display == .init(label: "Getrennt", severity: .neutral))
        #expect(ConnectionState.scanning.display == .init(label: "Suchend", severity: .searching))
        #expect(ConnectionState.connecting.display == .init(label: "Verbindend", severity: .searching))
        #expect(ConnectionState.connected.display == .init(label: "Verbunden", severity: .connected))
        #expect(ConnectionState.bluetoothOff.display == .init(label: "Bluetooth aus", severity: .warning))
        #expect(ConnectionState.unauthorized.display == .init(label: "Keine Berechtigung", severity: .warning))
    }
}
