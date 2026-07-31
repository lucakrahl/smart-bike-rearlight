import Foundation
import SwiftData
import SmartBikeCore

/// Schicht 5 — SwiftData-Implementierung von `RideRepository` als Hintergrund-
/// `ModelActor` (App Bible 9.3: Schreibvorgänge blockieren den Main-Thread nie).
///
/// TODO (Xcode/Claude Code): CRUD implementieren + Mapping Entity <-> Core-Typ,
/// inkrementelles Batch-Schreiben, Recovery unfertiger Fahrten (AR-DATA-04).
@ModelActor
actor SwiftDataStore: RideRepository {
    func startRide(deviceId: UUID?) async throws -> UUID { fatalError("TODO") }
    func append(_ point: TrackPoint, to rideId: UUID) async throws { /* TODO */ }
    func finishRide(_ rideId: UUID, statistics: RideStatistics) async throws { /* TODO */ }
    func allRides() async throws -> [RideSummary] { [] }        // TODO
    func ride(_ id: UUID) async throws -> RideDetail? { nil }   // TODO
    func deleteRide(_ id: UUID) async throws { /* TODO */ }
    func recoverUnfinishedRide() async throws -> UUID? { nil }  // TODO
}
