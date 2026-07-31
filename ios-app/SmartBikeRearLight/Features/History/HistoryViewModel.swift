import Foundation
import Observation
import SmartBikeCore

@MainActor @Observable
final class HistoryViewModel {
    private let repository: RideRepository
    private(set) var rides: [RideSummary] = []
    init(repository: RideRepository) { self.repository = repository }
    func load() async { rides = (try? await repository.allRides()) ?? [] }
    func delete(_ id: UUID) async { try? await repository.deleteRide(id); await load() }
    // Gesamtübersicht (AR-STAT-03): Summen über `rides` (TODO).
}
