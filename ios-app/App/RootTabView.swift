import SwiftUI

/// Navigations-Wurzel: TabBar mit drei Tabs (App Bible Kap. 7).
struct RootTabView: View {
    @Environment(AppEnvironment.self) private var env
    var body: some View {
        TabView {
            CockpitView()
                .tabItem { Label("Live", systemImage: "gauge.with.dots.needle.bottom.50percent") }
            HistoryView()
                .tabItem { Label("Verlauf", systemImage: "clock.arrow.circlepath") }
            SettingsView()
                .tabItem { Label("Einstellungen", systemImage: "gearshape") }
        }
    }
}
