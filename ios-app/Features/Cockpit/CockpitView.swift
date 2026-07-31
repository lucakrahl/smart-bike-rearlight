import SwiftUI
import SmartBikeCore

/// Live-Cockpit (App Bible 6.4). Zonen: Statuszeile · Kacheln (Hero+3) · Start/Stopp.
/// TODO (Xcode/Claude Code): Kachel-Raster aus DashboardLayout, Hold-to-Stop-Button
/// mit Fortschrittsring (AR-UX-02), Stale-Abdimmung (AR-UX-05), Editor-Sheet.
struct CockpitView: View {
    @Environment(AppEnvironment.self) private var env
    var body: some View {
        // let vm = CockpitViewModel(store: env.telemetryStore, rides: env.rideManager)
        Text("Cockpit — TODO")
            .navigationTitle("Live")
    }
}
