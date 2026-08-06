import Foundation
import Observation
import SmartBikeCore

/// Ein Punkt der Detaildiagramme (X = kumulierte Distanz). Höhe optional → höhenlose
/// Punkte werden im Höhenprofil gefiltert. Sendable, damit off-main aufbaubar.
struct RideChartPoint: Identifiable, Sendable {
    let id: Int
    let distanceKm: Double
    let altitudeM: Double?
    let speedKmph: Double
}

/// Ein Punkt der Bremslicht-Validierung (X = Aufzeichnungszeit t).
struct BrakePoint: Identifiable, Sendable {
    let id: Int
    let t: Double
    let decel: Double        // brake_decel_ms2
    let pct: Double          // brake_light_pct (0…100)
    let imuHealthy: Bool     // false → Fail-Safe (Schlusslicht statt Kennlinie) kennzeichnen
}

/// Roher Routenpunkt (Sendable); die View macht daraus `CLLocationCoordinate2D`.
struct RoutePoint: Sendable { let lat: Double; let lon: Double }

/// Fertig aufbereitete Detaildarstellung — komplett abseits des Main-Threads gebaut.
struct RideDetailPresentation: Sendable {
    let startedAt: Date
    let endedAt: Date?
    let statistics: RideStatistics
    let chart: [RideChartPoint]
    let brake: [BrakePoint]
    let maxDecel: Double
    let route: [RoutePoint]
    let maxDistanceKm: Double
    let altitudeDomain: ClosedRange<Double>?
    let csvText: String
}

@MainActor @Observable
final class RideDetailViewModel {
    private let repository: RideRepository
    let rideId: UUID
    private(set) var presentation: RideDetailPresentation?
    private(set) var isLoading = false
    /// Fertige CSV-Datei fürs Share-Sheet (nil, solange nicht geladen/geschrieben).
    private(set) var exportURL: URL?

    init(rideId: UUID, repository: RideRepository) { self.rideId = rideId; self.repository = repository }

    func load() async {
        isLoading = true
        defer { isLoading = false }
        // Fetch läuft im Hintergrund-ModelActor; die Aufbereitung zusätzlich off-main.
        guard let detail = try? await repository.ride(rideId) else { presentation = nil; return }
        let prepared = await Task.detached { Self.buildPresentation(from: detail) }.value
        presentation = prepared
        exportURL = await Task.detached { Self.writeCSV(prepared.csvText, startedAt: prepared.startedAt) }.value
    }

    func delete() async { try? await repository.deleteRide(rideId) }

    /// Reiner Aufbau von Diagramm-Serien + Routenlinie + CSV (nonisolated → Hintergrund).
    nonisolated static func buildPresentation(from detail: RideDetail) -> RideDetailPresentation {
        let cum = StatisticsEngine().cumulativeDistanceKm(for: detail.points)
        let chart = detail.points.indices.map { i in
            RideChartPoint(id: i, distanceKm: cum[i],
                           altitudeM: detail.points[i].altitudeM,
                           speedKmph: detail.points[i].speedKmph)
        }
        let brake: [BrakePoint] = detail.points.enumerated().compactMap { i, p in
            guard let decel = p.brakeDecelMs2, let pct = p.brakeLightPct else { return nil }
            return BrakePoint(id: i, t: p.t, decel: decel, pct: Double(pct),
                              imuHealthy: p.imuHealth == .ok)
        }
        let maxDecel = max(brake.map(\.decel).max() ?? 0, 0.5)   // gültige Achse auch ohne Bremsen
        let route = detail.points
            .filter { $0.isGnssValid }
            .map { RoutePoint(lat: $0.lat, lon: $0.lon) }
        let maxDist = max(cum.last ?? 0, 0.001)
        let alts = detail.points.compactMap(\.altitudeM)
        let domain: ClosedRange<Double>? = alts.isEmpty ? nil : ((alts.min()! - 3)...(alts.max()! + 3))
        let appVersion = (Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String) ?? "?"
        let csv = RideCSVExporter.export(points: detail.points, startedAt: detail.startedAt,
                                         endedAt: detail.endedAt, appVersion: appVersion)
        return RideDetailPresentation(startedAt: detail.startedAt, endedAt: detail.endedAt,
                                      statistics: detail.statistics, chart: chart, brake: brake,
                                      maxDecel: maxDecel, route: route, maxDistanceKm: maxDist,
                                      altitudeDomain: domain, csvText: csv)
    }

    /// Schreibt die CSV mit UTF-8-BOM (Excel-Umlaute) in eine Temp-Datei fürs Share-Sheet.
    nonisolated static func writeCSV(_ text: String, startedAt: Date) -> URL? {
        let df = DateFormatter()
        df.locale = Locale(identifier: "de_DE")
        df.dateFormat = "yyyy-MM-dd-HHmm"
        let name = "SmartBikeRearLight-Fahrt-\(df.string(from: startedAt)).csv"
        let url = FileManager.default.temporaryDirectory.appendingPathComponent(name)
        let data = Data([0xEF, 0xBB, 0xBF]) + Data(text.utf8)   // BOM + Inhalt
        return (try? data.write(to: url)) == nil ? nil : url
    }
}
