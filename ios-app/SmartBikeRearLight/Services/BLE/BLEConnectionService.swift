import Foundation
import CoreBluetooth
import SmartBikeCore

/// Schicht 1 — kapselt Core Bluetooth als Actor (AR-CONN-01/02/03/07/08).
/// Unveränderlicher BLE-Vertrag (App Bible Kap. 10). Auto-Reconnect + Hintergrundbetrieb.
///
/// TODO (Xcode/Claude Code): CBCentralManagerDelegate implementieren:
///   - scanForPeripherals(withServices: [Self.serviceUUID])
///   - gespeicherten Peripheral-Identifier (BLEDevice) für Auto-Connect nutzen
///   - Characteristic abonnieren (setNotifyValue) und Bytes in die Continuation schreiben
///   - Hintergrundmodus „bluetooth-central" (siehe Resources/Info-Setup.md)
actor BLEConnectionService: TelemetrySource {
    static let serviceUUID = CBUUID(string: "587bb505-9f9d-4ae0-96fd-0b29adfc4b03")
    static let characteristicUUID = CBUUID(string: "8c604d09-743f-4850-9109-19604a17f358")
    static let deviceName = "SmartBikeRearLight"

    private(set) var connectionState: ConnectionState = .disconnected
    private var continuation: AsyncStream<Data>.Continuation?

    nonisolated func frames() -> AsyncStream<Data> {
        AsyncStream { continuation in
            Task { await self.setContinuation(continuation) }
        }
    }
    private func setContinuation(_ c: AsyncStream<Data>.Continuation) { self.continuation = c }

    func start() async { /* TODO: CBCentralManager erstellen und scannen/verbinden */ }
    func stop() async { continuation?.finish() }
}
