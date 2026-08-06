import Foundation
import CoreBluetooth
import SmartBikeCore

/// Schicht 1 — echte BLE-Anbindung über Core Bluetooth (AR-CONN-01/02/03/04/07/08).
/// Erfüllt denselben `TelemetrySource`-Vertrag wie der Mock. Unveränderlicher
/// BLE-Vertrag (App Bible Kap. 10): Scan per Service-UUID, eine NOTIFY-Characteristic,
/// rohe 80-Byte-Frames. Hier wird NICHT geparst — das macht der `TelemetryFrameDecoder`.
///
/// Concurrency: Der Actor ist eine dünne, Sendable-Fassade. Die eigentlichen
/// CoreBluetooth-Callbacks laufen alle auf einer dedizierten Dispatch-Queue im
/// `@unchecked Sendable`-Coordinator; die AsyncStream-Continuation wird ausschließlich
/// aus diesem einen Kontext (der BLE-Queue) bedient → keine Datenrennen.
actor BLEConnectionService: TelemetrySource {
    static let serviceUUID = CBUUID(string: "587bb505-9f9d-4ae0-96fd-0b29adfc4b03")
    static let characteristicUUID = CBUUID(string: "8c604d09-743f-4850-9109-19604a17f358")
    static let deviceName = "SmartBikeRearLight"

    private let coordinator: BLECoordinator
    private(set) var connectionState: ConnectionState = .disconnected

    init() {
        coordinator = BLECoordinator(serviceUUID: Self.serviceUUID,
                                     characteristicUUID: Self.characteristicUUID)
        // Zustandsänderungen aus der BLE-Queue in den Actor spiegeln.
        coordinator.onState = { [weak self] state in
            Task { await self?.updateState(state) }
        }
    }

    private func updateState(_ state: ConnectionState) { connectionState = state }

    nonisolated func frames() -> AsyncStream<Data> {
        AsyncStream { continuation in
            coordinator.setContinuation(continuation)
        }
    }

    func start() async { coordinator.start() }
    func stop() async { coordinator.stop() }
}

/// Bridged die CoreBluetooth-Delegate-Callbacks in Actor/Continuation. `@unchecked
/// Sendable`, weil der gesamte veränderliche Zustand ausschließlich auf `queue`
/// berührt wird (CBCentralManager-Delegate-Queue + eigene async-Hops dorthin).
private final class BLECoordinator: NSObject, @unchecked Sendable,
                                    CBCentralManagerDelegate, CBPeripheralDelegate {
    /// Aus der BLE-Queue aufgerufen; darf zu einem anderen Kontext hüpfen (Actor).
    var onState: (@Sendable (ConnectionState) -> Void)?

    private let serviceUUID: CBUUID
    private let characteristicUUID: CBUUID
    private let queue = DispatchQueue(label: "de.lucakrahl.SmartBikeRearLight.ble")

    private var central: CBCentralManager?
    private var peripheral: CBPeripheral?
    private var continuation: AsyncStream<Data>.Continuation?
    private var phase: BluetoothLinkPhase = .idle

    init(serviceUUID: CBUUID, characteristicUUID: CBUUID) {
        self.serviceUUID = serviceUUID
        self.characteristicUUID = characteristicUUID
        super.init()
    }

    // MARK: Steuerung (aus dem Actor; jeweils auf die BLE-Queue verlagert)

    func setContinuation(_ c: AsyncStream<Data>.Continuation) {
        queue.async { self.continuation = c }
    }

    func start() {
        queue.async {
            if self.central == nil {
                // Delegate-Queue = unsere BLE-Queue → alle Callbacks laufen hier.
                self.central = CBCentralManager(delegate: self, queue: self.queue,
                                                options: [CBCentralManagerOptionShowPowerAlertKey: true])
            } else {
                self.startScan()
            }
        }
    }

    func stop() {
        queue.async {
            self.central?.stopScan()
            if let p = self.peripheral { self.central?.cancelPeripheralConnection(p) }
            self.peripheral = nil
            self.phase = .idle
            self.publishState()
            self.continuation?.finish()
            self.continuation = nil
        }
    }

    // MARK: CBCentralManagerDelegate

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        publishState()
        if central.state == .poweredOn { startScan() }   // (erneut) suchen, sobald bereit
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        // Gefiltert wurde bereits per Service-UUID (nicht über den Namen — der liegt in
        // der Scan-Response). Erstes passendes Gerät nehmen, Scan aus (Energie, AR-CONN-01).
        self.peripheral = peripheral
        peripheral.delegate = self
        central.stopScan()
        phase = .connecting
        publishState()
        central.connect(peripheral, options: nil)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        peripheral.discoverServices([serviceUUID])   // Phase bleibt „verbindend" bis subscribed
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral,
                        error: Error?) {
        self.peripheral = nil
        startScan()   // Auto-Reconnect (AR-CONN-02)
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral,
                        error: Error?) {
        self.peripheral = nil
        phase = .idle
        publishState()
        startScan()   // Auto-Reconnect (AR-CONN-02); Backfill kommt über dieselbe Notify-Char.
    }

    // MARK: CBPeripheralDelegate

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let service = peripheral.services?.first(where: { $0.uuid == serviceUUID }) else { return }
        peripheral.discoverCharacteristics([characteristicUUID], for: service)
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService,
                    error: Error?) {
        guard let ch = service.characteristics?.first(where: { $0.uuid == characteristicUUID }) else { return }
        peripheral.setNotifyValue(true, for: ch)   // NOTIFY abonnieren (AR-CONN-04)
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateNotificationStateFor characteristic: CBCharacteristic,
                    error: Error?) {
        if characteristic.isNotifying {
            phase = .connected   // erst jetzt fließen Frames → „verbunden"
            publishState()
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic,
                    error: Error?) {
        guard let data = characteristic.value else { return }
        // Roh und ungeprüft weiterreichen — Länge/Version prüft der Decoder (BLE-Vertrag fix).
        continuation?.yield(data)
    }

    // MARK: Intern (immer auf `queue`)

    private func startScan() {
        guard let central, central.state == .poweredOn, peripheral == nil else { return }
        phase = .scanning
        publishState()
        central.scanForPeripherals(withServices: [serviceUUID], options: nil)
    }

    private func publishState() {
        let power = Self.power(from: central?.state ?? .unknown)
        onState?(ConnectionStateMapper.connectionState(power: power, phase: phase))
    }

    private static func power(from state: CBManagerState) -> BluetoothManagerState {
        switch state {
        case .poweredOn:    return .poweredOn
        case .poweredOff:   return .poweredOff
        case .unauthorized: return .unauthorized
        case .resetting:    return .resetting
        case .unsupported:  return .unsupported
        case .unknown:      return .unknown
        @unknown default:   return .unknown
        }
    }
}
