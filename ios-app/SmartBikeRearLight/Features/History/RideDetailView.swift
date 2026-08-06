import SwiftUI
import Charts
import MapKit
import SmartBikeCore

/// Fahrt-Detail (App Bible 6.5, AR-STAT-02/AR-VIS-*). Kennzahlen + Höhenprofil +
/// Geschwindigkeit (Swift Charts, X = Distanz) + Routenkarte (MapKit). Serien/Route
/// werden off-main aufgebaut (flüssiges Öffnen); höhenlose Punkte sind gefiltert.
struct RideDetailView: View {
    @Environment(AppEnvironment.self) private var env
    @Environment(\.dismiss) private var dismiss
    let rideId: UUID
    @State private var vm: RideDetailViewModel?

    private static let minRoutePoints = 2

    var body: some View {
        ScrollView {
            if let presentation = vm?.presentation {
                content(presentation)
            } else {
                ProgressView("Lädt…").padding(.top, 80)
            }
        }
        .navigationTitle("Fahrt")
        .navigationBarTitleDisplayMode(.inline)
        .toolbar {
            if let url = vm?.exportURL {
                ToolbarItem(placement: .topBarTrailing) {
                    ShareLink(item: url) { Label("Exportieren", systemImage: "square.and.arrow.up") }
                }
            }
        }
        .task {
            if vm == nil { vm = RideDetailViewModel(rideId: rideId, repository: env.repository) }
            await vm?.load()
        }
    }

    @ViewBuilder
    private func content(_ p: RideDetailPresentation) -> some View {
        VStack(spacing: Theme.Spacing.unit * 2) {
            header(p)
            statsGrid(p.statistics)

            if p.chart.count >= 2 {
                ChartCard(title: "Höhenprofil", unit: "m · über Distanz") {
                    altitudeChart(p)
                }
                ChartCard(title: "Geschwindigkeit", unit: "km/h · über Distanz") {
                    speedChart(p)
                }
            }
            if p.brake.count >= 2 {
                ChartCard(title: "Bremslicht-Validierung", unit: "m/s² · % über Zeit") {
                    brakeChart(p)
                }
            }

            routeCard(p.route)
            deleteButton
        }
        .padding()
    }

    // MARK: - Kopf & Kennzahlen

    private func header(_ p: RideDetailPresentation) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(p.startedAt, format: .dateTime.weekday(.wide).day().month(.wide).year())
                .font(.headline)
            Text(p.startedAt, format: .dateTime.hour().minute())
                .font(.subheadline).foregroundStyle(.secondary)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private func statsGrid(_ s: RideStatistics) -> some View {
        let items: [MetricDisplay] = [
            .init(label: "Distanz", value: String(format: "%.2f", s.distanceKm), unit: "km"),
            .init(label: "Dauer", value: Self.hms(s.duration), unit: ""),
            .init(label: "Ø", value: String(format: "%.1f", s.avgSpeedKmph), unit: "km/h"),
            .init(label: "Max", value: String(format: "%.1f", s.maxSpeedKmph), unit: "km/h"),
            .init(label: "↑ Höhenmeter", value: String(format: "%.0f", s.ascentM), unit: "m"),
            .init(label: "↓ Höhenmeter", value: String(format: "%.0f", s.descentM), unit: "m"),
            .init(label: "Min Höhe", value: String(format: "%.0f", s.minAltitudeM), unit: "m"),
            .init(label: "Max Höhe", value: String(format: "%.0f", s.maxAltitudeM), unit: "m"),
        ]
        return LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())],
                         spacing: Theme.Spacing.unit) {
            ForEach(Array(items.enumerated()), id: \.offset) { _, display in
                MetricTile(display: display, valueSize: 22)
            }
        }
    }

    // MARK: - Diagramme (gleiche X-Achse = Distanz)

    private struct AltPoint: Identifiable { let id: Int; let distanceKm: Double; let altitudeM: Double }

    private func altitudeChart(_ p: RideDetailPresentation) -> some View {
        // Höhenlose Punkte filtern → keine Ausreißer-Balken.
        let alt = p.chart.compactMap { c in c.altitudeM.map { AltPoint(id: c.id, distanceKm: c.distanceKm, altitudeM: $0) } }
        let domain = p.altitudeDomain ?? 0...1
        return Chart(alt) { point in
            AreaMark(x: .value("Distanz (km)", point.distanceKm),
                     y: .value("Höhe (m)", point.altitudeM))
                .foregroundStyle(Theme.Chart.altitude.opacity(0.22))
            LineMark(x: .value("Distanz (km)", point.distanceKm),
                     y: .value("Höhe (m)", point.altitudeM))
                .foregroundStyle(Theme.Chart.altitude)
        }
        .chartXScale(domain: 0...p.maxDistanceKm)
        .chartYScale(domain: domain)                 // Y-Bereich an reale Höhen gebunden
        .frame(height: 160)
        .clipped()                                   // Fläche endet am unteren Diagrammrand
    }

    private func speedChart(_ p: RideDetailPresentation) -> some View {
        Chart(p.chart) { point in
            LineMark(x: .value("Distanz (km)", point.distanceKm),
                     y: .value("km/h", point.speedKmph))
                .foregroundStyle(Color.accentColor)   // Geschwindigkeit = Cyan
                .interpolationMethod(.catmullRom)
        }
        .chartXScale(domain: 0...p.maxDistanceKm)
        .frame(height: 160)
        .clipped()
    }

    // Doppelachse: Verzögerung (m/s², links) + Bremslicht (%, rechts) über die Zeit.
    // Erwartete Korrelation: Verzögerung ↑ ⇒ Lichtintensität ↑. IMU-Fehler (≠ .ok) sind
    // dezent markiert — dort zeigt das Rücklicht Fail-Safe-Schlusslicht, keine „keine Reaktion".
    private func brakeChart(_ p: RideDetailPresentation) -> some View {
        let maxDecel = p.maxDecel
        let decelName = "Verzögerung (m/s²)"
        let pctName = "Bremslicht (%)"
        let pctTicks: [Double] = [0, 25, 50, 75, 100]
        return Chart {
            ForEach(p.brake) { b in
                LineMark(x: .value("Zeit (s)", b.t), y: .value("Wert", b.decel),
                         series: .value("Serie", decelName))
                    .foregroundStyle(by: .value("Serie", decelName))
                // Bremslicht-% in den Verzögerungs-Wertebereich skaliert (gemeinsame Y-Achse).
                LineMark(x: .value("Zeit (s)", b.t), y: .value("Wert", b.pct / 100 * maxDecel),
                         series: .value("Serie", pctName))
                    .foregroundStyle(by: .value("Serie", pctName))
            }
            ForEach(p.brake.filter { !$0.imuHealthy }) { b in
                PointMark(x: .value("Zeit (s)", b.t), y: .value("Wert", b.decel))
                    .symbolSize(28)
                    .foregroundStyle(.secondary)
            }
        }
        .chartForegroundStyleScale([decelName: Theme.Semantic.warning, pctName: Color.accentColor])
        .chartYScale(domain: 0...maxDecel)
        .chartYAxis {
            AxisMarks(position: .leading)                       // m/s²
            AxisMarks(position: .trailing, values: pctTicks.map { $0 / 100 * maxDecel }) { mark in
                AxisGridLine()
                AxisTick()
                AxisValueLabel {
                    if let v = mark.as(Double.self) {
                        Text("\(Int((v / maxDecel * 100).rounded()))%")
                    }
                }
            }
        }
        .frame(height: 180)
        .clipped()
    }

    // MARK: - Route

    @ViewBuilder
    private func routeCard(_ route: [RoutePoint]) -> some View {
        let coords = route.map { CLLocationCoordinate2D(latitude: $0.lat, longitude: $0.lon) }
        if coords.count >= Self.minRoutePoints, let region = Self.region(for: coords) {
            VStack(alignment: .leading, spacing: Theme.Spacing.unit) {
                Text("Route").font(.headline)
                Map(initialPosition: .region(region)) {
                    MapPolyline(coordinates: coords)
                        .stroke(Color.accentColor, lineWidth: 4)
                }
                .frame(height: 240)
                .clipShape(RoundedRectangle(cornerRadius: Theme.tileCornerRadius))
                .allowsHitTesting(false)   // im Detail nur Vorschau
            }
        }
    }

    private var deleteButton: some View {
        Button(role: .destructive) {
            Task { await vm?.delete(); dismiss() }
        } label: {
            Label("Fahrt löschen", systemImage: "trash")
                .frame(maxWidth: .infinity, minHeight: Theme.minTapTarget)
        }
        .buttonStyle(.bordered)
        .tint(Theme.Semantic.warning)
        .padding(.top, Theme.Spacing.unit)
    }

    // MARK: - Reine Helfer

    static func region(for coords: [CLLocationCoordinate2D]) -> MKCoordinateRegion? {
        guard !coords.isEmpty else { return nil }
        let lats = coords.map(\.latitude), lons = coords.map(\.longitude)
        let minLat = lats.min()!, maxLat = lats.max()!
        let minLon = lons.min()!, maxLon = lons.max()!
        let center = CLLocationCoordinate2D(latitude: (minLat + maxLat) / 2,
                                            longitude: (minLon + maxLon) / 2)
        let span = MKCoordinateSpan(latitudeDelta: max((maxLat - minLat) * 1.4, 0.003),
                                    longitudeDelta: max((maxLon - minLon) * 1.4, 0.003))
        return MKCoordinateRegion(center: center, span: span)
    }

    static func hms(_ t: TimeInterval) -> String {
        let s = Int(t.rounded())
        return String(format: "%02d:%02d:%02d", s / 3600, (s % 3600) / 60, s % 60)
    }
}

/// Einheitlich gestaltete Diagramm-Karte (Titel + Einheit + Inhalt).
private struct ChartCard<Content: View>: View {
    let title: String
    let unit: String
    @ViewBuilder var content: Content
    var body: some View {
        VStack(alignment: .leading, spacing: Theme.Spacing.unit) {
            HStack {
                Text(title).font(.headline)
                Spacer()
                Text(unit).font(.caption).foregroundStyle(.secondary)
            }
            content
        }
        .padding()
        .background(.thinMaterial, in: RoundedRectangle(cornerRadius: Theme.tileCornerRadius))
        .clipShape(RoundedRectangle(cornerRadius: Theme.tileCornerRadius))
    }
}
