import SwiftUI
import Charts
import MapKit
import SmartBikeCore

/// Fahrt-Detail (App Bible 6.5, AR-STAT-02/AR-VIS-*). Kennzahlen + Höhenprofil +
/// Geschwindigkeit (Swift Charts, X = Distanz) + Routenkarte (MapKit, Should).
struct RideDetailView: View {
    @Environment(AppEnvironment.self) private var env
    @Environment(\.dismiss) private var dismiss
    let rideId: UUID
    @State private var vm: RideDetailViewModel?

    /// Ab so vielen Fix-Punkten lohnt sich die Routenanzeige.
    private static let minRoutePoints = 2

    var body: some View {
        ScrollView {
            if let detail = vm?.detail {
                content(detail)
            } else {
                ProgressView().padding(.top, 80)
            }
        }
        .navigationTitle("Fahrt")
        .navigationBarTitleDisplayMode(.inline)
        .task {
            if vm == nil { vm = RideDetailViewModel(rideId: rideId, repository: env.repository) }
            await vm?.load()
        }
    }

    @ViewBuilder
    private func content(_ detail: RideDetail) -> some View {
        let plot = Self.plotPoints(for: detail.points)
        let maxDist = max(plot.last?.distanceKm ?? 0, 0.001)
        VStack(spacing: Theme.Spacing.unit * 2) {
            header(detail)
            statsGrid(detail.statistics)

            if plot.count >= 2 {
                ChartCard(title: "Höhenprofil", unit: "m · über Distanz") {
                    altitudeChart(plot, maxDist: maxDist)
                }
                ChartCard(title: "Geschwindigkeit", unit: "km/h · über Distanz") {
                    speedChart(plot, maxDist: maxDist)
                }
            }

            routeCard(detail.points)
            deleteButton
        }
        .padding()
    }

    // MARK: - Kopf & Kennzahlen

    private func header(_ detail: RideDetail) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(detail.startedAt, format: .dateTime.weekday(.wide).day().month(.wide).year())
                .font(.headline)
            Text(detail.startedAt, format: .dateTime.hour().minute())
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

    private func altitudeChart(_ plot: [PlotPoint], maxDist: Double) -> some View {
        let alts = plot.map(\.altitudeM)
        let lo = (alts.min() ?? 0) - 3
        let hi = (alts.max() ?? 0) + 3
        return Chart(plot) { p in
            AreaMark(x: .value("Distanz (km)", p.distanceKm),
                     y: .value("Höhe (m)", p.altitudeM))
                .foregroundStyle(Theme.Chart.altitude.opacity(0.22))
            LineMark(x: .value("Distanz (km)", p.distanceKm),
                     y: .value("Höhe (m)", p.altitudeM))
                .foregroundStyle(Theme.Chart.altitude)
        }
        .chartXScale(domain: 0...maxDist)
        .chartYScale(domain: lo...hi)
        .frame(height: 160)
        .clipped()   // Flächenfüllung endet exakt am unteren Diagrammrand
    }

    private func speedChart(_ plot: [PlotPoint], maxDist: Double) -> some View {
        Chart(plot) { p in
            LineMark(x: .value("Distanz (km)", p.distanceKm),
                     y: .value("km/h", p.speedKmph))
                .foregroundStyle(Color.accentColor)                 // Geschwindigkeit = Cyan
                .interpolationMethod(.catmullRom)
        }
        .chartXScale(domain: 0...maxDist)
        .frame(height: 160)
        .clipped()
    }

    // MARK: - Route

    @ViewBuilder
    private func routeCard(_ points: [TrackPoint]) -> some View {
        let coords = points
            .filter { $0.gnssFix == .fixOK }
            .map { CLLocationCoordinate2D(latitude: $0.lat, longitude: $0.lon) }
        if coords.count >= Self.minRoutePoints, let region = Self.region(for: coords) {
            VStack(alignment: .leading, spacing: Theme.Spacing.unit) {
                Text("Route").font(.headline)
                Map(initialPosition: .region(region)) {
                    MapPolyline(coordinates: coords)
                        .stroke(Color.accentColor, lineWidth: 4)
                }
                .frame(height: 240)
                .clipShape(RoundedRectangle(cornerRadius: Theme.tileCornerRadius))
                .allowsHitTesting(false)   // im Detail nur Vorschau, kein Panning im ScrollView
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

    struct PlotPoint: Identifiable {
        let id: Int
        let distanceKm: Double
        let altitudeM: Double
        let speedKmph: Double
    }

    /// Verknüpft jeden Punkt mit seiner kumulierten Distanz (SmartBikeCore).
    static func plotPoints(for points: [TrackPoint]) -> [PlotPoint] {
        let cum = StatisticsEngine().cumulativeDistanceKm(for: points)
        return points.indices.map { i in
            PlotPoint(id: i, distanceKm: cum[i],
                      altitudeM: points[i].altitudeM, speedKmph: points[i].speedKmph)
        }
    }

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
