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
    /// Aufzeichnungsrate (AP6). Umschalten nur außerhalb einer laufenden Aufzeichnung
    /// (`setMode`); der Wert für eine Fahrt wird bei `start()` festgeschrieben.
    private(set) var mode: RecordingMode = .hz1

    private let repository: RideRepository
    private let engine = StatisticsEngine()
    private var currentRide: UUID?
    private var points: [TrackPoint] = []
    /// Zuletzt persistierter Decimations-Bucket (mode-abhängig); −1 = noch keiner.
    private var lastPersistedBucket: Int = -1
    /// Noch nicht geschriebene Punkte (Batch-Puffer, AP6) und die laufenden Schreib-Tasks.
    private var pendingPersist: [TrackPoint] = []
    private var persistTasks: [Task<Void, Never>] = []
    /// Monotone Aufzeichnungszeit gegen Geräte-Uhr-Resets/Lücken (reine Logik in Core).
    /// Startet bei 0 (neue Fahrt) bzw. beim letzten `t` (Fortsetzen, AR-DATA-04).
    private var clock = RecordingClock()

    /// ~1 Schreibvorgang pro Sekunde: 1 Hz → jedes Sample, 10 Hz → alle 10 Samples.
    private var flushBatchSize: Int { mode.hz }

    init(repository: RideRepository) { self.repository = repository }

    /// Aufzeichnungsrate umschalten. Nur außerhalb einer laufenden Aufzeichnung erlaubt
    /// (AP6); während `recording`/`finishing` No-op → `false`.
    @discardableResult
    func setMode(_ newMode: RecordingMode) -> Bool {
        guard state == .idle else { return false }
        mode = newMode
        return true
    }

    func start() {
        guard state == .idle else { return }
        points.removeAll(keepingCapacity: true)
        pendingPersist.removeAll(keepingCapacity: true)
        statistics = .zero
        lastPersistedBucket = -1
        clock = RecordingClock()
        state = .recording
        // Persistenz folgt separat; Rückgabe dient später als Aufzeichnungs-ID.
        Task { currentRide = try? await repository.startRide(deviceId: nil) }
    }

    /// Wartet, bis die vom Repository vergebene Aufzeichnungs-ID vorliegt (start() setzt
    /// sie asynchron). Für deterministische Tests; im Betrieb sonst nicht nötig.
    func recordingReady() async {
        var spins = 0
        while state == .recording, currentRide == nil, spins < 1_000 {
            await Task.yield(); spins += 1
        }
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

        // Rate-Verdichtung je Modus (AP6): 1 Hz → Ganzsekunden-Bucket (unverändert),
        // 10 Hz → 0,1-s-Bucket. `t` (Double) behält volle Sub-Sekunden-Auflösung, sodass
        // die gespeicherten Zeitstempel bei 10 Hz eindeutig bleiben (E-4).
        let bucket = mode.bucket(for: t)
        guard bucket != lastPersistedBucket else { return }   // (AR-DATA-02)
        lastPersistedBucket = bucket

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

        // Persistenz gepuffert und gebatcht (AP6): ~1 Save/s statt 1 Save/Sample.
        // Live-Kennzahlen hängen NICHT an der Persistenz → Main-Thread nie blockiert.
        pendingPersist.append(point)
        if pendingPersist.count >= flushBatchSize { scheduleFlush() }
    }

    /// Reiht den aktuellen Puffer als EINEN Batch-Schreibvorgang ein und leert ihn.
    /// Der `save()` passiert im Hintergrund-`ModelActor`; hier nur Task-Start.
    private func scheduleFlush() {
        guard let ride = currentRide, !pendingPersist.isEmpty else { return }
        let batch = pendingPersist
        pendingPersist.removeAll(keepingCapacity: true)
        let repo = repository
        persistTasks.append(Task { try? await repo.appendBatch(batch, to: ride) })
    }

    /// Wartet, bis alle eingereihten Batch-Schreibvorgänge abgeschlossen sind.
    private func drainPersist() async {
        scheduleFlush()                          // Rest einreihen
        let tasks = persistTasks
        persistTasks.removeAll(keepingCapacity: true)
        for task in tasks { await task.value }
    }

    func stop() async {
        guard state == .recording else { return }
        state = .finishing
        let finalStats = engine.computeStatistics(from: points)
        statistics = finalStats
        await drainPersist()                        // gepufferte Samples sicher schreiben (AP6)
        if let ride = currentRide {
            try? await repository.finishRide(ride, statistics: finalStats)
        }
        currentRide = nil
        lastPersistedBucket = -1
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
        pendingPersist.removeAll(keepingCapacity: true)
        statistics = engine.computeStatistics(from: existing)
        // Uhr beim letzten `t` fortsetzen; erste neue Frame-Zeit wird zum Anker.
        clock = RecordingClock(elapsed: existing.last?.t ?? 0)
        lastPersistedBucket = existing.last.map { mode.bucket(for: $0.t) } ?? -1
        state = .recording
    }
}
