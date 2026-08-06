import Foundation
import SwiftData
import SmartBikeCore

/// Schicht 5 — SwiftData-Implementierung von `RideRepository` als Hintergrund-
/// `ModelActor` (App Bible 9.3: Schreibvorgänge blockieren den Main-Thread nie).
/// Mapping Entity <-> Core-Typ ist in kleinen Hilfsfunktionen gekapselt.
@ModelActor
actor SwiftDataStore: RideRepository {

    // MARK: - Schreiben

    func startRide(deviceId: UUID?) async throws -> UUID {
        let ride = RideEntity(startedAt: Date(), status: "recording")
        modelContext.insert(ride)
        try modelContext.save()
        return ride.id
    }

    /// Hängt einen 1-Hz-Punkt an; idempotent gegen doppelte Zeitstempel, damit ein
    /// Backfill (AR-DATA-04) denselben Punkt nicht doppelt einträgt.
    func append(_ point: TrackPoint, to rideId: UUID) async throws {
        guard let ride = try fetchRide(rideId) else { return }
        if ride.samples.contains(where: { $0.t == point.t }) { return }   // idempotent
        let sample = makeSample(point)
        sample.ride = ride
        ride.samples.append(sample)
        modelContext.insert(sample)
        try modelContext.save()
    }

    func finishRide(_ rideId: UUID, statistics: RideStatistics) async throws {
        guard let ride = try fetchRide(rideId) else { return }
        ride.endedAt = Date()
        ride.status = "finished"
        apply(statistics, to: ride)
        try modelContext.save()
    }

    func deleteRide(_ id: UUID) async throws {
        guard let ride = try fetchRide(id) else { return }
        modelContext.delete(ride)          // Samples per cascade (siehe @Relationship)
        try modelContext.save()
    }

    // MARK: - Lesen

    /// Nur abgeschlossene Fahrten, neueste zuerst.
    func allRides() async throws -> [RideSummary] {
        let descriptor = FetchDescriptor<RideEntity>(
            predicate: #Predicate { $0.status == "finished" },
            sortBy: [SortDescriptor(\.startedAt, order: .reverse)]
        )
        return try modelContext.fetch(descriptor).map(makeSummary)
    }

    func ride(_ id: UUID) async throws -> RideDetail? {
        guard let ride = try fetchRide(id) else { return nil }
        let points = ride.samples.sorted { $0.t < $1.t }.map(makeTrackPoint)
        return RideDetail(id: ride.id, startedAt: ride.startedAt, endedAt: ride.endedAt,
                          statistics: makeStatistics(ride), points: points)
    }

    func recoverUnfinishedRide() async throws -> UUID? {
        var descriptor = FetchDescriptor<RideEntity>(
            predicate: #Predicate { $0.status == "recording" },
            sortBy: [SortDescriptor(\.startedAt, order: .reverse)]
        )
        descriptor.fetchLimit = 1
        return try modelContext.fetch(descriptor).first?.id
    }

    // MARK: - Helfer

    private func fetchRide(_ id: UUID) throws -> RideEntity? {
        var descriptor = FetchDescriptor<RideEntity>(predicate: #Predicate { $0.id == id })
        descriptor.fetchLimit = 1
        return try modelContext.fetch(descriptor).first
    }

    // MARK: Mapping Entity <-> Core-Typ

    private func makeSample(_ p: TrackPoint) -> TrackSampleEntity {
        TrackSampleEntity(t: p.t, lat: p.lat, lon: p.lon, altitudeM: p.altitudeM,
                          speedKmph: p.speedKmph, courseDeg: p.courseDeg,
                          sats: p.sats, hdop: p.hdop, gnssFix: Int(p.gnssFix.rawValue),
                          brakeDecelMs2: p.brakeDecelMs2, brakeLightPct: p.brakeLightPct,
                          imuHealth: Int(p.imuHealth.rawValue),
                          pressurePa: p.pressurePa, gnssAltitudeM: p.gnssAltitudeM,
                          deviceTimestampMs: p.deviceTimestampMs.map(Int.init),
                          baroValid: p.baroValid,
                          systemState: p.systemState.map { Int($0.rawValue) },
                          initDegraded: p.initDegraded,
                          watchdogRecovered: p.watchdogRecovered,
                          frameVersion: p.frameVersion)
    }

    private func makeTrackPoint(_ s: TrackSampleEntity) -> TrackPoint {
        TrackPoint(t: s.t, lat: s.lat, lon: s.lon, altitudeM: s.altitudeM,
                   speedKmph: s.speedKmph, courseDeg: s.courseDeg, sats: s.sats,
                   hdop: s.hdop, gnssFix: GnssFixStatus(rawValue: UInt8(clamping: s.gnssFix)) ?? .noData,
                   brakeDecelMs2: s.brakeDecelMs2, brakeLightPct: s.brakeLightPct,
                   imuHealth: ImuHealthState(rawValue: UInt8(clamping: s.imuHealth ?? 0)) ?? .ok,
                   pressurePa: s.pressurePa, gnssAltitudeM: s.gnssAltitudeM,
                   deviceTimestampMs: s.deviceTimestampMs.map { UInt32(clamping: $0) },
                   baroValid: s.baroValid,
                   systemState: s.systemState.flatMap { SystemState(rawValue: UInt8(clamping: $0)) },
                   initDegraded: s.initDegraded,
                   watchdogRecovered: s.watchdogRecovered,
                   frameVersion: s.frameVersion)
    }

    private func makeStatistics(_ r: RideEntity) -> RideStatistics {
        RideStatistics(duration: r.duration, totalDuration: r.totalDuration ?? r.duration,
                       distanceKm: r.distanceKm,
                       avgSpeedKmph: r.avgSpeedKmph, maxSpeedKmph: r.maxSpeedKmph,
                       ascentM: r.ascentM, descentM: r.descentM,
                       minAltitudeM: r.minAltitudeM, maxAltitudeM: r.maxAltitudeM)
    }

    private func apply(_ st: RideStatistics, to r: RideEntity) {
        r.duration = st.duration; r.totalDuration = st.totalDuration; r.distanceKm = st.distanceKm
        r.avgSpeedKmph = st.avgSpeedKmph; r.maxSpeedKmph = st.maxSpeedKmph
        r.ascentM = st.ascentM; r.descentM = st.descentM
        r.minAltitudeM = st.minAltitudeM; r.maxAltitudeM = st.maxAltitudeM
    }

    private func makeSummary(_ r: RideEntity) -> RideSummary {
        RideSummary(id: r.id, startedAt: r.startedAt, statistics: makeStatistics(r))
    }
}
