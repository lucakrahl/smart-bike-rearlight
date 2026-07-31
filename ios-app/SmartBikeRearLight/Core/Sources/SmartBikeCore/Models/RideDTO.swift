import Foundation

/// Kompakte Fahrt-Zusammenfassung für die Fahrtenliste (App Bible 6.5).
public struct RideSummary: Sendable, Equatable, Identifiable {
    public var id: UUID
    public var startedAt: Date
    public var statistics: RideStatistics
    public init(id: UUID, startedAt: Date, statistics: RideStatistics) {
        self.id = id; self.startedAt = startedAt; self.statistics = statistics
    }
}

/// Vollständige Fahrt inkl. Track (für Detailansicht/Diagramme/Route).
public struct RideDetail: Sendable, Equatable, Identifiable {
    public var id: UUID
    public var startedAt: Date
    public var endedAt: Date?
    public var statistics: RideStatistics
    public var points: [TrackPoint]
    public init(id: UUID, startedAt: Date, endedAt: Date?, statistics: RideStatistics, points: [TrackPoint]) {
        self.id = id; self.startedAt = startedAt; self.endedAt = endedAt
        self.statistics = statistics; self.points = points
    }
}
