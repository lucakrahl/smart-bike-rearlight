import Foundation
import SmartBikeCore

/// Simulierte Telemetriequelle für Entwicklung im Simulator (kein echtes Gerät).
/// Erfüllt denselben Vertrag wie `BLEConnectionService` und liefert gültige
/// **113-Byte-v3-Frames** (Little-Endian, App Bible Kap. 10 / Vertrag v3) mit ~10 Hz.
/// Byte-Aufbau über `TelemetryFrameEncoder` (dieselbe Quelle wie die Round-Trip-Tests).
/// WARUM: Ermöglicht das Live-Cockpit ohne ESP32; der BLE-Vertrag (FR-SYS-04)
/// bleibt dabei unverändert, da hier echte Frames im Firmware-Format erzeugt werden.
actor MockTelemetrySource: TelemetrySource {
    private(set) var connectionState: ConnectionState = .disconnected
    private var continuation: AsyncStream<Data>.Continuation?
    private var task: Task<Void, Never>?

    nonisolated func frames() -> AsyncStream<Data> {
        AsyncStream { continuation in
            Task { await self.setContinuation(continuation) }
        }
    }
    private func setContinuation(_ c: AsyncStream<Data>.Continuation) { self.continuation = c }

    func start() async {
        guard task == nil else { return }
        connectionState = .connected
        task = Task { await self.run() }
    }

    func stop() async {
        task?.cancel()
        task = nil
        connectionState = .disconnected
        continuation?.finish()
    }

    /// Erzeugt fortlaufend Frames im 10-Hz-Takt (100 ms) — identisch zur
    /// Telemetrierate der Firmware. Kein `delay()`, sondern kooperatives `Task.sleep`.
    private func run() async {
        let periodNs: UInt64 = 100_000_000        // 100 ms → 10 Hz
        var timestampMs: UInt32 = 0
        var tick: Double = 0
        while !Task.isCancelled {
            continuation?.yield(Self.makeFrame(timestampMs: timestampMs, tick: tick))
            timestampMs &+= 100                    // hochzählend, überlaufsicher
            tick += 1
            try? await Task.sleep(nanoseconds: periodNs)
        }
    }

    /// Baut ein plausibles v3-Frame (113 Byte, version 3) und serialisiert es über den
    /// gemeinsamen `TelemetryFrameEncoder`. Enthält bewusst Sonderfälle über die Zeit:
    /// `bias_calibrated == 0` in den ersten ~3 s (Boot), sowie periodische Phasen mit
    /// `gnss_accel_valid == 0` (dann `gnss_accel_ms2 = 0`).
    static func makeFrame(timestampMs: UInt32, tick: Double) -> Data {
        // Geschwindigkeit sinusförmig 0…35 km/h (Periode ~12,6 s bei 10 Hz).
        let speed = Float((sin(tick * 0.05) * 0.5 + 0.5) * 35.0)

        // Route: sanft gekrümmter Pfad um Düsseldorf (Amplituden > float32-Auflösung bei ~51°).
        let angle = tick * 0.004
        let lat = Float(51.2277 + sin(angle) * 0.006)
        let lon = Float(6.7735 + (1 - cos(angle)) * 0.009)

        // Höhe barometrisch: Druck so, dass sich ~200 m ±15 m ergeben (p = p0·(1−h/44330)^5.255).
        let targetAltitude = 200.0 + sin(tick * 0.02) * 15.0
        let pressure = Float(101_325.0 * pow(1.0 - targetAltitude / 44_330.0, 5.255))

        // Bremsen: pulsierende Verzögerung (nur positive Phasen) + Bremskennlinie 20 %→100 %.
        let brakeDecel = Float(max(0.0, -cos(tick * 0.05) * 3.0))     // 0…3 m/s²
        let braking = brakeDecel > 0.2
        let brakeLightPct: UInt8 = braking ? UInt8(min(100.0, 20.0 + Double(brakeDecel) / 3.0 * 80.0)) : 0

        // Sonderfälle:
        let biasCalibrated: UInt8 = tick < 30 ? 0 : 1                 // erste ~3 s unkalibriert
        let gnssValid: UInt8 = sin(tick * 0.03) > -0.5 ? 1 : 0        // periodische Ausfälle
        let gnssAccel: Float = gnssValid == 1 ? brakeDecel * 0.9 : 0  // 0, wenn ungültig

        // Regime-Zähler (Summe ~10), Shock/Dynamic steigen beim Bremsen.
        let dynamicN: UInt8 = braking ? 6 : 2
        let shockN: UInt8 = braking ? UInt8(min(4.0, Double(brakeDecel))) : 0
        let staticN = UInt8(max(0, 10 - Int(dynamicN) - Int(shockN)))

        let frame = TelemetryFrame(
            version: 3, timestampMs: timestampMs,
            accelX: 0, accelY: 0, accelZ: 9.81, gyroX: 0, gyroY: 0, gyroZ: 0,
            brakeDecel: brakeDecel,
            pressurePa: pressure, temperatureC: 21.5,
            lat: lat, lon: lon, speedKmph: speed, courseDeg: Float((angle * 57.3).truncatingRemainder(dividingBy: 360)),
            altitudeM: Float(targetAltitude),           // GNSS-Höhe (Referenz)
            sats: 9, hdop: 0.8,
            utcYear: 2026, utcMonth: 8, utcDay: 7,
            utcHour: 15, utcMinute: 42, utcSecond: UInt8((Int(tick) / 10) % 60),
            systemState: .run, initDegraded: false, imuHealth: .ok,
            baroValid: true, gnssFix: .fixOK, watchdogRecovered: false,
            brakeLightPct: brakeLightPct,
            gnssAccelMs2: gnssAccel,
            pitchRad: Float(sin(tick * 0.02) * 0.05),
            gyroBiasRads: -0.01,
            normDeltaMin: braking ? -0.5 : -0.2,
            normDeltaMax: braking ? brakeDecel + 1.0 : 0.5,
            jerkMax: braking ? brakeDecel * 1.5 : 0.3,
            regimeStaticN: staticN, regimeDynamicN: dynamicN, regimeShockN: shockN,
            biasCalibrated: biasCalibrated, gnssAccelValid: gnssValid,
            dtMaxMs: 10, loopMaxUs: 1200
        )
        return TelemetryFrameEncoder.encode(frame)
    }
}
