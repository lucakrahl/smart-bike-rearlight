import Foundation
import Observation
import SmartBikeCore

/// Kurzinfo zu einer beim App-Start gefundenen, unterbrochenen Aufzeichnung (AR-DATA-04).
/// Wird dem Nutzer zur Wahl gestellt (abschließen/verwerfen) — nie automatisch fortgesetzt.
struct PendingRecovery: Equatable, Identifiable {
    let id: UUID
    let startedAt: Date
    let sampleCount: Int
    let duration: TimeInterval
}

/// Schicht 4 — Aufzeichnungs-Lebenszyklus (App Bible 9.2, AR-REC-01, AR-DATA-01/04).
/// Verdichtet Frames auf 1 Hz, hält die Punkte im RAM und berechnet die Live-
/// Kennzahlen über die `StatisticsEngine`. Persistenz folgt separat; das (aktuell
/// no-op) `RideRepository` wird bereits aufgerufen, ist für die Live-Werte aber egal.
@MainActor @Observable
final class RideManager {
    private(set) var state: RecordingState = .idle
    /// Live mitlaufende Kennzahlen (Distanz/Fahrzeit/Ø/Max) — Quelle fürs Cockpit (AR-LIVE-05/07).
    private(set) var statistics: RideStatistics = .zero
    /// Gesetzt, wenn beim Start eine unterbrochene Aufzeichnung gefunden wurde (AR-DATA-04).
    private(set) var pendingRecovery: PendingRecovery?

    private let repository: RideRepository
    private let engine = StatisticsEngine()
    private var currentRide: UUID?
    private var points: [TrackPoint] = []
    private var lastPersistedSecond: Int = -1
    /// Monotone Aufzeichnungszeit gegen Geräte-Uhr-Resets/Lücken (reine Logik in Core).
    /// Startet bei 0 (neue Fahrt) bzw. beim letzten `t` (Fortsetzen, AR-DATA-04).
    private var clock = RecordingClock()

    init(repository: RideRepository) { self.repository = repository }

    func start() {
        guard state == .idle else { return }
        points.removeAll(keepingCapacity: true)
        statistics = .zero
        lastPersistedSecond = -1
        clock = RecordingClock()
        state = .recording
        // Persistenz folgt separat; Rückgabe dient später als Aufzeichnungs-ID.
        Task { currentRide = try? await repository.startRide(deviceId: nil) }
    }

    /// Aufruf pro dekodiertem Frame (10 Hz). Verdichtet auf 1 Hz und schreibt die
    /// Live-Kennzahlen fort. No-op, solange nicht aufgezeichnet wird.
    func ingest(_ frame: TelemetryFrame) {
        guard state == .recording else { return }

        // Monotone Aufzeichnungszeit: fängt Geräte-Uhr-Resets/Lücken ab (Core).
        let (advanced, step) = clock.advanced(to: frame.timestampMs)
        clock = advanced
        guard step.accepted else { return }                // dt ≤ 0 (Duplikat) verwerfen
        let t = step.time

        let sec = Int(t)
        guard sec != lastPersistedSecond else { return }   // 1-Hz-Verdichtung (AR-DATA-02)
        lastPersistedSecond = sec

        // Höhe barometrisch (pressure_pa); Fallback GNSS-Höhe nur bei gültigem Fix,
        // sonst höhenlos. Rohwerte als Referenz mitspeichern.
        let pressurePa = Double(frame.pressurePa)
        let gnssAltitudeM = Double(frame.altitudeM)
        let altitude = AltitudeResolver.altitude(
            baroValid: frame.baroValid,
            pressurePa: pressurePa,
            gnssValid: frame.isGnssValid,
            gnssAltitudeM: gnssAltitudeM
        )
        let point = TrackPoint(
            t: t,
            lat: Double(frame.lat), lon: Double(frame.lon),
            altitudeM: altitude,
            speedKmph: Double(frame.speedKmph),
            courseDeg: Double(frame.courseDeg),
            sats: Int(frame.sats), hdop: Double(frame.hdop),
            gnssFix: frame.gnssFix,
            brakeDecelMs2: Double(frame.brakeDecel),        // Bremslicht-Validierung (v2)
            brakeLightPct: Int(frame.brakeLightPct),
            imuHealth: frame.imuHealth,
            pressurePa: pressurePa,
            gnssAltitudeM: gnssAltitudeM,
            temperatureC: Double(frame.temperatureC),
            deviceTimestampMs: frame.timestampMs,           // roher Geräte-Zeitstempel
            baroValid: frame.baroValid,
            systemState: frame.systemState,
            initDegraded: frame.initDegraded,
            watchdogRecovered: frame.watchdogRecovered,
            frameVersion: Int(frame.version),               // je Sample mitschreiben (CSV-Präambel AP7)
            // v3-Analyse-/Aggregatfelder (nil bei v2-Frames) — nur Persistenz/Analyse.
            gnssAccelMs2: frame.gnssAccelMs2.map(Double.init),
            pitchRad: frame.pitchRad.map(Double.init),
            gyroBiasRads: frame.gyroBiasRads.map(Double.init),
            normDeltaMin: frame.normDeltaMin.map(Double.init),
            normDeltaMax: frame.normDeltaMax.map(Double.init),
            jerkMax: frame.jerkMax.map(Double.init),
            regimeStaticN: frame.regimeStaticN.map(Int.init),
            regimeDynamicN: frame.regimeDynamicN.map(Int.init),
            regimeShockN: frame.regimeShockN.map(Int.init),
            biasCalibrated: frame.biasCalibrated.map { $0 == 1 },
            gnssAccelValid: frame.gnssAccelValid.map { $0 == 1 },
            dtMaxMs: frame.dtMaxMs.map(Int.init),
            loopMaxUs: frame.loopMaxUs.map(Int.init)
        )
        points.append(point)
        statistics = engine.computeStatistics(from: points)   // Live-Update (AR-LIVE-07)

        if let ride = currentRide {
            Task { try? await repository.append(point, to: ride) }
        }
    }

    func stop() async {
        guard state == .recording else { return }
        state = .finishing
        let finalStats = engine.computeStatistics(from: points)
        statistics = finalStats
        if let ride = currentRide {
            try? await repository.finishRide(ride, statistics: finalStats)
        }
        currentRide = nil
        lastPersistedSecond = -1
        clock = RecordingClock()
        state = .idle
    }

    /// Beim App-Start: hängengebliebene „recording"-Fahrt erkennen und dem Nutzer zur
    /// Wahl stellen (AR-DATA-04). NICHT automatisch fortsetzen. No-op im Fahrbetrieb.
    func recoverIfNeeded() async {
        guard state == .idle, pendingRecovery == nil else { return }
        guard let id = (try? await repository.recoverUnfinishedRide()) ?? nil else { return }
        let detail = try? await repository.ride(id)
        pendingRecovery = PendingRecovery(
            id: id,
            startedAt: detail?.startedAt ?? Date(),
            sampleCount: detail?.points.count ?? 0,
            duration: detail?.points.last?.t ?? 0
        )
    }

    /// Unterbrochene Fahrt abschließen: Statistik aus den Samples berechnen und
    /// als „finished" speichern → erscheint im Verlauf.
    func finishRecovered() async {
        guard let pending = pendingRecovery else { return }
        pendingRecovery = nil                                   // sofort räumen (Alert schließt)
        let detail = try? await repository.ride(pending.id)
        let stats = engine.computeStatistics(from: detail?.points ?? [])
        try? await repository.finishRide(pending.id, statistics: stats)
    }

    /// Unterbrochene Fahrt verwerfen: Datensatz löschen.
    func discardRecovered() async {
        guard let pending = pendingRecovery else { return }
        pendingRecovery = nil
        try? await repository.deleteRide(pending.id)
    }

    /// Unterbrochene Fahrt fortsetzen: bestehende Samples laden und die Aufzeichnung
    /// nahtlos wieder aufnehmen. Neue Punkte laufen über `tOffset` hinter den
    /// bestehenden weiter, sodass keine Zeitstempel kollidieren.
    func resumeRecovered() async {
        guard let pending = pendingRecovery, state == .idle else { return }
        pendingRecovery = nil
        let existing = (try? await repository.ride(pending.id))?.points ?? []
        currentRide = pending.id
        points = existing
        statistics = engine.computeStatistics(from: existing)
        // Uhr beim letzten `t` fortsetzen; erste neue Frame-Zeit wird zum Anker.
        clock = RecordingClock(elapsed: existing.last?.t ?? 0)
        lastPersistedSecond = Int(existing.last?.t ?? -1)
        state = .recording
    }
}
