import Foundation
import SmartBikeCore

/// Schicht 1 — Schnittstelle (App Bible 9.2). Liefert rohe 80-Byte-Frames als
/// AsyncStream + den Verbindungszustand. Ermöglicht Mocks in Tests.
public protocol TelemetrySource: Sendable {
    func frames() -> AsyncStream<Data>
    var connectionState: ConnectionState { get async }
    func start() async
    func stop() async
}
