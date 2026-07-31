import Foundation

// Firmware-Statusfelder (App Bible Kap. 10, unveränderlich).
public enum SystemState: UInt8, Sendable, Equatable { case initializing = 0, run = 1 }
public enum ImuHealthState: UInt8, Sendable, Equatable { case ok = 0, recovering = 1, failed = 2 }
public enum GnssFixStatus: UInt8, Sendable, Equatable { case noData = 0, noFix = 1, fixOK = 2 }

// App-Zustandsmodell (App Bible Kap. 9.4).
public enum ConnectionState: Sendable, Equatable {
    case disconnected, scanning, connecting, connected, bluetoothOff, unauthorized
}
public enum LiveDataState: Sendable, Equatable { case fresh, stale, none }   // AR-UX-05
public enum RecordingState: Sendable, Equatable { case idle, recording, finishing }  // AR-DATA-01

// Cockpit-Personalisierung (App Bible 6.4 / AR-LIVE-08).
public enum TileSize: String, Codable, Sendable, CaseIterable {
    case s1x1 = "1x1", s2x1 = "2x1", s3x1 = "3x1", s2x2 = "2x2"
}

// Metrik-Katalog (AR-LIVE-09). Erweiterbar ohne Bruch (AR-NFR-EXT-01).
public enum MetricID: String, Codable, Sendable, CaseIterable {
    case speed, avgSpeed, maxSpeed, distance, duration, altitude, ascent, descent, course, sats, hdop, connection
}
