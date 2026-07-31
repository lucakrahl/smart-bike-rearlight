import Foundation
import SmartBikeCore

/// Schicht 5 — Persistenz-Schnittstelle (App Bible 9.2, AR-DATA-02/03/04).
/// SwiftData-Implementierung: `SwiftDataStore`. Mock-Implementierung für Tests.
public protocol RideRepository: Sendable {
    func startRide(deviceId: UUID?) async throws -> UUID
    func append(_ point: TrackPoint, to rideId: UUID) async throws
    func finishRide(_ rideId: UUID, statistics: RideStatistics) async throws
    func allRides() async throws -> [RideSummary]
    func ride(_ id: UUID) async throws -> RideDetail?
    func deleteRide(_ id: UUID) async throws
    /// Wiederherstellung einer beim Absturz hängengebliebenen Aufzeichnung (AR-DATA-04).
    func recoverUnfinishedRide() async throws -> UUID?
}
