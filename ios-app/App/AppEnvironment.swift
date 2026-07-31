import SwiftUI
import Observation

/// Composition Root (App Bible 9.5): erzeugt die konkreten Implementierungen der
/// Protokolle und stellt die geteilten Stores bereit. Kein DI-Framework.
@MainActor @Observable
final class AppEnvironment {
    let telemetryStore: TelemetryStore
    let rideManager: RideManager
    // let source: TelemetrySource        // BLEConnectionService (live) / Mock (preview/test)
    // let repository: RideRepository      // SwiftDataStore (live) / InMemory (preview/test)

    init(telemetryStore: TelemetryStore, rideManager: RideManager) {
        self.telemetryStore = telemetryStore
        self.rideManager = rideManager
    }

    /// Reale Verdrahtung. TODO (Xcode/Claude Code): SwiftDataStore + BLEConnectionService
    /// erzeugen, Decode-Consumer-Task starten (BLE → Decoder → TelemetryStore).
    static func live() -> AppEnvironment {
        let store = TelemetryStore()
        // let repository = SwiftDataStore(modelContainer: …)
        // let source = BLEConnectionService()
        let manager = RideManager(repository: /* repository */ PreviewRideRepository())
        return AppEnvironment(telemetryStore: store, rideManager: manager)
    }
}
