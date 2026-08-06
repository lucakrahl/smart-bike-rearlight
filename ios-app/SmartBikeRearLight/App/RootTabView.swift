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
        // Unterbrochene Fahrt beim App-Start (AR-DATA-04). Alert nur im idle-Zustand,
        // nie während der Fahrt (AR-UX-01) — recoverIfNeeded() setzt pendingRecovery
        // ausschließlich beim Start.
        .alert("Unterbrochene Fahrt gefunden",
               isPresented: recoveryPresented,
               presenting: env.rideManager.pendingRecovery) { pending in
            Button("Abschließen") { Task { await env.rideManager.finishRecovered() } }
            Button("Verwerfen", role: .destructive) { Task { await env.rideManager.discardRecovered() } }
            // „Weiter fahren" trägt die .cancel-Rolle (sicheres Fortsetzen) und verhindert,
            // dass SwiftUI zusätzlich einen eigenen „Cancel"-Button einfügt.
            Button("Weiter fahren", role: .cancel) { Task { await env.rideManager.resumeRecovered() } }
        } message: { pending in
            // App ist durchgängig deutsch → Datum unabhängig vom Geräte-Locale in de_DE.
            let stamp = pending.startedAt.formatted(
                Date.FormatStyle(date: .abbreviated, time: .shortened).locale(Locale(identifier: "de_DE")))
            Text("Vom \(stamp) · \(pending.sampleCount) Punkte. Abschließen und in den Verlauf übernehmen, verwerfen oder weiter aufzeichnen?")
        }
    }

    /// Präsentation an `pendingRecovery` gekoppelt; die Aktionen räumen den Zustand selbst.
    private var recoveryPresented: Binding<Bool> {
        Binding(get: { env.rideManager.pendingRecovery != nil }, set: { _ in })
    }
}
