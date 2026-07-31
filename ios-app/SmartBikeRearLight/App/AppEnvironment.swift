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
    private let source: TelemetrySource?
    private var telemetryTask: Task<Void, Never>?

    init(telemetryStore: TelemetryStore, rideManager: RideManager,
         repository: RideRepository, source: TelemetrySource? = nil) {
        self.telemetryStore = telemetryStore
        self.rideManager = rideManager
        self.repository = repository
        self.source = source
    }

    /// Reale Verdrahtung. Persistenz über SwiftData (Hintergrund-ModelActor), Live-
    /// Telemetrie vorläufig über die simulierte Quelle (BLE-Vertrag unverändert).
    /// TODO: BLEConnectionService statt Mock, sobald verfügbar.
    static func live() -> AppEnvironment {
        let store = TelemetryStore()
        let container = Self.makeModelContainer()
        let repository = SwiftDataStore(modelContainer: container)
        let manager = RideManager(repository: repository)
        let source = MockTelemetrySource()
        let env = AppEnvironment(telemetryStore: store, rideManager: manager,
                                 repository: repository, source: source)
        env.startTelemetry()
        Task { await manager.recoverIfNeeded() }   // hängengebliebene Aufzeichnung (AR-DATA-04)
        return env
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
            telemetryStore.update(connection: .connected)
            for await data in stream {
                if let frame = TelemetryFrameDecoder.decode(data) {
                    telemetryStore.apply(frame)
                    rideManager.ingest(frame)   // no-op außerhalb der Aufzeichnung
                }
            }
        }
    }
}
