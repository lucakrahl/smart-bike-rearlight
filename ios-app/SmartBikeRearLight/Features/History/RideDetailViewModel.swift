import Foundation
import Observation
import SmartBikeCore

@MainActor @Observable
final class RideDetailViewModel {
    private let repository: RideRepository
    let rideId: UUID
    private(set) var detail: RideDetail?
    init(rideId: UUID, repository: RideRepository) { self.rideId = rideId; self.repository = repository }
    func load() async { detail = try? await repository.ride(rideId) }
    func delete() async { try? await repository.deleteRide(rideId) }
}
