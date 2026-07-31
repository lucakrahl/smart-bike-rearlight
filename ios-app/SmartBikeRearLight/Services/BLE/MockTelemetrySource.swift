import Foundation
import SmartBikeCore

/// Simulierte Telemetriequelle für Entwicklung im Simulator (kein echtes Gerät).
/// Erfüllt denselben Vertrag wie `BLEConnectionService` und liefert gültige
/// 80-Byte-Frames (Little-Endian, feste Offsets — App Bible Kap. 10) mit ~10 Hz.
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

    /// Baut ein gültiges 80-Byte-Frame an den festen Offsets des BLE-Vertrags.
    /// Nur die für das Cockpit relevanten Felder sind belegt; der Rest bleibt 0.
    static func makeFrame(timestampMs: UInt32, tick: Double) -> Data {
        var bytes = [UInt8](repeating: 0, count: TelemetryFrame.byteCount)

        func writeU16(_ v: UInt16, at o: Int) {
            withUnsafeBytes(of: v.littleEndian) { raw in
                bytes[o] = raw[0]; bytes[o + 1] = raw[1]
            }
        }
        func writeU32(_ v: UInt32, at o: Int) {
            withUnsafeBytes(of: v.littleEndian) { raw in
                for i in 0..<4 { bytes[o + i] = raw[i] }
            }
        }
        func writeF32(_ v: Float, at o: Int) { writeU32(v.bitPattern, at: o) }

        writeU16(TelemetryFrame.schemaVersion, at: 0)   // version == 1 (Offset 0)
        writeU32(timestampMs, at: 2)                    // timestamp_ms (Offset 2)

        // Geschwindigkeit sinusförmig 0…35 km/h (Offset 50); Periode ~12,6 s bei 10 Hz.
        let speed = Float((sin(tick * 0.05) * 0.5 + 0.5) * 35.0)
        writeF32(speed, at: 50)

        writeF32(200, at: 58)                           // altitude_m ~200 (Offset 58)
        bytes[62] = 9                                   // sats = 9 (Offset 62)
        bytes[78] = GnssFixStatus.fixOK.rawValue        // gnss_fix = 2 (Offset 78)

        return Data(bytes)
    }
}
