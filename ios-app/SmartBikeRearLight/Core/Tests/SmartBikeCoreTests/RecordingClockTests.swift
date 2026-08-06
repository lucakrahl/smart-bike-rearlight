import Testing
import Foundation
@testable import SmartBikeCore

/// Robuste Aufzeichnungszeit gegen Geräte-Uhr-Resets und Lücken (Bugfix Zeit/Distanz).
struct RecordingClockTests {
    private let maxDt = RecordingClock.maxSampleDt

    @Test func firstSampleIsAnchor() {
        let (next, step) = RecordingClock().advanced(to: 1000)
        #expect(step.accepted)
        #expect(step.dt == 0)
        #expect(step.time == 0)
        #expect(!step.didReset)
        #expect(next.elapsed == 0)
    }

    @Test func normalStepsAccumulateExactly() {
        var c = RecordingClock()
        (c, _) = c.advanced(to: 0)
        var step: RecordingClockStep
        (c, step) = c.advanced(to: 100)
        #expect(abs(step.dt - 0.1) < 1e-9)
        #expect(abs(step.time - 0.1) < 1e-9)
        (c, step) = c.advanced(to: 200)
        #expect(abs(step.time - 0.2) < 1e-9)
        #expect(!step.didReset)
    }

    @Test func gapIsClampedNotBlownUp() {
        var c = RecordingClock()
        (c, _) = c.advanced(to: 1000)
        let (next, step) = c.advanced(to: 1000 + 30_000)   // 30 s Funkabriss
        #expect(!step.didReset)
        #expect(step.dt == maxDt)                           // dt gekappt
        #expect(abs(next.elapsed - maxDt) < 1e-9)           // Dauer wächst nur um 1,5 s
    }

    @Test func deviceResetToZeroIsDetectedAndClamped() {
        var c = RecordingClock()
        (c, _) = c.advanced(to: 60_000)
        (c, _) = c.advanced(to: 60_100)                     // elapsed = 0,1 s
        let (next, step) = c.advanced(to: 0)                // Rücklicht-Neustart
        #expect(step.didReset)
        #expect(step.accepted)
        #expect(step.dt == maxDt)
        #expect(next.elapsed > c.elapsed)                   // monoton
        #expect(abs(next.elapsed - (0.1 + maxDt)) < 1e-9)
    }

    @Test func duplicateTimestampIsRejected() {
        var c = RecordingClock()
        (c, _) = c.advanced(to: 5000)
        (c, _) = c.advanced(to: 5100)
        let before = c.elapsed
        let (next, step) = c.advanced(to: 5100)             // gleicher Zeitstempel
        #expect(!step.accepted)
        #expect(step.dt == 0)
        #expect(next.elapsed == before)
    }

    @Test func backfillReconstructsRealElapsed() {
        // Nach Reconnect ohne Neustart: gepufferte Frames mit fortlaufenden 100-ms-Stempeln.
        var c = RecordingClock()
        (c, _) = c.advanced(to: 0)
        for i in 1...50 { (c, _) = c.advanced(to: UInt32(i * 100)) }
        #expect(abs(c.elapsed - 5.0) < 1e-9)                // 50 × 0,1 s exakt rekonstruiert
    }

    @Test func timeStaysMonotonicAcrossResetSequence() {
        var c = RecordingClock()
        var last = -1.0
        for ts: UInt32 in [0, 100, 200, 300, 0, 100, 200, 300] {   // Reset mittendrin
            let (next, step) = c.advanced(to: ts)
            c = next
            if step.accepted { #expect(step.time >= last); last = step.time }
        }
    }
}
