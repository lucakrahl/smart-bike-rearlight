import SwiftUI
import SmartBikeCore

/// Verlauf/Fahrtenliste (App Bible 6.5): chronologische Liste gespeicherter Fahrten
/// mit Datum, Distanz, Dauer und Ø-Geschwindigkeit; Wisch-Löschen.
/// Gesamtübersicht und Detailansicht folgen separat.
struct HistoryView: View {
    @Environment(AppEnvironment.self) private var env
    @State private var vm: HistoryViewModel?

    var body: some View {
        NavigationStack {
            Group {
                if let vm {
                    List {
                        ForEach(vm.rides) { ride in
                            NavigationLink {
                                RideDetailView(rideId: ride.id)
                            } label: {
                                RideRow(ride: ride)
                            }
                        }
                        .onDelete { offsets in
                            let ids = offsets.map { vm.rides[$0].id }
                            Task { for id in ids { await vm.delete(id) } }
                        }
                    }
                    .overlay {
                        if vm.rides.isEmpty {
                            ContentUnavailableView("Keine Fahrten", systemImage: "bicycle",
                                                   description: Text("Zeichne im Live-Tab eine Fahrt auf."))
                        }
                    }
                    .refreshable { await vm.load() }
                } else {
                    ProgressView()
                }
            }
            .navigationTitle("Verlauf")
            .task {
                if vm == nil { vm = HistoryViewModel(repository: env.repository) }
                await vm?.load()   // erneut bei jedem Erscheinen (frische Fahrten übernehmen)
            }
        }
    }
}

/// Eine Zeile der Fahrtenliste. Zahlen tabellarisch für ruhiges Layout.
private struct RideRow: View {
    let ride: RideSummary

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(ride.startedAt, format: .dateTime.weekday().day().month().hour().minute())
                .font(.headline)
            HStack(spacing: Theme.Spacing.unit * 2) {
                metric(String(format: "%.2f", ride.statistics.distanceKm), "km")
                metric(Self.hms(ride.statistics.duration), "")
                metric(String(format: "%.1f", ride.statistics.avgSpeedKmph), "km/h Ø")
            }
            .font(.subheadline)
            .foregroundStyle(.secondary)
            .monospacedDigit()
        }
        .padding(.vertical, 4)
    }

    private func metric(_ value: String, _ unit: String) -> some View {
        HStack(spacing: 3) {
            Text(value)
            if !unit.isEmpty { Text(unit).foregroundStyle(.tertiary) }
        }
    }

    /// hh:mm:ss ohne Lokalisierungsabhängigkeit (tabellarische Ziffern).
    static func hms(_ t: TimeInterval) -> String {
        let s = Int(t.rounded())
        return String(format: "%02d:%02d:%02d", s / 3600, (s % 3600) / 60, s % 60)
    }
}
