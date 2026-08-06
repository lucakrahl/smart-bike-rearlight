import SwiftUI
import Observation
import SwiftData
import SmartBikeCore

/// Composition Root (App Bible 9.5): erzeugt die konkreten Implementierungen der
/// Protokolle und stellt die geteilten Stores bereit. Kein DI-Framework.
@MainActor @Observable
final class AppEnvironment {
    let telemetryStore: TelemetryStore
    let rideManager: RideManager
    let repository: RideRepository        // SwiftDataStore (live) / InMemory (preview/test)
    /// Aktives Cockpit-Layout (AR-LIVE-08), persistiert in UserDefaults.
    private(set) var dashboardLayout: DashboardLayout
    private let source: TelemetrySource?
    private var telemetryTask: Task<Void, Never>?
    private static let layoutKey = "dashboardLayout"

    init(telemetryStore: TelemetryStore, rideManager: RideManager,
         repository: RideRepository, source: TelemetrySource? = nil) {
        self.telemetryStore = telemetryStore
        self.rideManager = rideManager
        self.repository = repository
        self.source = source
        self.dashboardLayout = Self.loadLayout()
    }

    /// Layout speichern (bereinigt) + persistieren.
    func updateLayout(_ layout: DashboardLayout) {
        let sane = DashboardLayoutStore().sanitized(layout)
        dashboardLayout = sane
        if let data = try? JSONEncoder().encode(sane) {
            UserDefaults.standard.set(data, forKey: Self.layoutKey)
        }
    }

    func resetLayout() { updateLayout(.standard) }

    private static func loadLayout() -> DashboardLayout {
        if let data = UserDefaults.standard.data(forKey: layoutKey),
           let decoded = try? JSONDecoder().decode(DashboardLayout.self, from: data) {
            return DashboardLayoutStore().sanitized(decoded)
        }
        return .standard
    }

    /// Reale Verdrahtung. Persistenz über SwiftData (Hintergrund-ModelActor), Live-
    /// Telemetrie über die gewählte `TelemetrySource` (echt vs. Mock — s. `makeSource()`).
    static func live() -> AppEnvironment {
        let store = TelemetryStore()
        let container = Self.makeModelContainer()
        let repository = SwiftDataStore(modelContainer: container)
        let manager = RideManager(repository: repository)
        let source = Self.makeSource()
        let env = AppEnvironment(telemetryStore: store, rideManager: manager,
                                 repository: repository, source: source)
        env.startTelemetry()
        Task { await manager.recoverIfNeeded() }   // hängengebliebene Aufzeichnung (AR-DATA-04)
        return env
    }

    /// Einzige Auswahlstelle der Datenquelle: echtes Gerät → `BLEConnectionService`;
    /// im Simulator (kein BLE) oder bei Launch-Flag `USE_MOCK_SOURCE` → `MockTelemetrySource`.
    /// Alles Nachgelagerte (Decoder, Store, RideManager) bleibt unverändert.
    private static func makeSource() -> TelemetrySource {
        #if targetEnvironment(simulator)
        return MockTelemetrySource()
        #else
        if ProcessInfo.processInfo.arguments.contains("USE_MOCK_SOURCE") {
            return MockTelemetrySource()
        }
        return BLEConnectionService()
        #endif
    }

    /// SwiftData-Container für die persistierten Modelle. Fällt bei Migrations-/
    /// Store-Fehlern auf einen In-Memory-Store zurück, damit die App startet.
    private static func makeModelContainer() -> ModelContainer {
        let types: [any PersistentModel.Type] = [RideEntity.self, TrackSampleEntity.self, BLEDeviceEntity.self]
        let schema = Schema(types)
        do {
            return try ModelContainer(for: schema)
        } catch {
            let config = ModelConfiguration(schema: schema, isStoredInMemoryOnly: true)
            return try! ModelContainer(for: schema, configurations: config)
        }
    }

    /// Startet den Decode-Consumer: Quelle → `TelemetryFrameDecoder` → `TelemetryStore`
    /// und (bei laufender Aufzeichnung) → `RideManager`. Läuft auf dem MainActor, da
    /// beide Senken MainActor-isoliert sind; die 80-Byte-Dekodierung bei 10 Hz ist
    /// vernachlässigbar.
    private func startTelemetry() {
        guard let source else { return }
        telemetryTask = Task { [telemetryStore, rideManager] in
            let stream = source.frames()
            await source.start()

            // Realen Verbindungszustand der Quelle in den Store spiegeln (AR-CONN-08,
            // AR-LIVE-02). Poll statt Push, damit die BLE-Schicht unverändert bleibt;
            // der Mock meldet weiterhin korrekt „Verbunden".
            let connectionTask = Task { [telemetryStore] in
                var last: ConnectionState?
                while !Task.isCancelled {
                    let state = await source.connectionState
                    if state != last { last = state; telemetryStore.update(connection: state) }
                    telemetryStore.evaluateFreshness(now: Date())   // „veraltet" auch ohne neue Frames
                    try? await Task.sleep(nanoseconds: 300_000_000)   // 300 ms
                }
            }
            defer { connectionTask.cancel() }

            for await data in stream {
                if let frame = TelemetryFrameDecoder.decode(data) {
                    telemetryStore.apply(frame)
                    rideManager.ingest(frame)   // no-op außerhalb der Aufzeichnung
                }
            }
        }
    }
}
