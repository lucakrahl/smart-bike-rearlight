import Foundation
import SmartBikeCore

/// Leichtgewichtige In-Memory-Implementierung für Previews/Bootstrap.
/// Ersetzt durch `SwiftDataStore`, sobald implementiert.
actor PreviewRideRepository: RideRepository {
    func startRide(deviceId: UUID?) async throws -> UUID { UUID() }
    func append(_ point: TrackPoint, to rideId: UUID) async throws {}
    func appendBatch(_ points: [TrackPoint], to rideId: UUID) async throws {}
    func finishRide(_ rideId: UUID, statistics: RideStatistics) async throws {}
    func allRides() async throws -> [RideSummary] { [] }
    func ride(_ id: UUID) async throws -> RideDetail? { nil }
    func deleteRide(_ id: UUID) async throws {}
    func recoverUnfinishedRide() async throws -> UUID? { nil }
}
