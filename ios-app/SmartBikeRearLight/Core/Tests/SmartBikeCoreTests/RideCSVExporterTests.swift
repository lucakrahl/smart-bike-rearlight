import Testing
import Foundation
@testable import SmartBikeCore

/// AP7: CSV-Export mit 35 Spalten. `temperature_c` entfällt aus der CSV, die 13 v3-Felder
/// hängen in Offset-Reihenfolge (81→111) hinten an. Präambel-`schema_version` = Geräte-
/// Frame-Version (E-2), `t_s` mit ≥ 2 Nachkommastellen (E-4). Der Golden-Header (inkl.
/// Präambel) ist die verbindliche Doku für die externe Auswertung und wird hier eingefroren.
struct RideCSVExporterTests {

    private let utc = TimeZone(identifier: "UTC")!
    private var header: String { RideCSVExporter.columns.joined(separator: ";") }

    /// Punkt mit allen 13 v3-Feldern gesetzt (Standard: frame_version 3).
    private func v3Point(t: Double, frameVersion: Int = 3) -> TrackPoint {
        TrackPoint(t: t, lat: 51.1, lon: 6.7, altitudeM: 201.5, speedKmph: 36, courseDeg: 10,
                   sats: 9, hdop: 0.9, gnssFix: .fixOK, brakeDecelMs2: 2.5, brakeLightPct: 80,
                   imuHealth: .recovering, pressurePa: 98900, gnssAltitudeM: 206, temperatureC: 21.6,
                   deviceTimestampMs: 1334, baroValid: true, systemState: .run,
                   initDegraded: false, watchdogRecovered: true, frameVersion: frameVersion,
                   gnssAccelMs2: 3.1, pitchRad: 0.12, gyroBiasRads: -0.015,
                   normDeltaMin: -1.2, normDeltaMax: 2.7, jerkMax: 4.4,
                   regimeStaticN: 3, regimeDynamicN: 5, regimeShockN: 2,
                   biasCalibrated: true, gnssAccelValid: true, dtMaxMs: 12, loopMaxUs: 1234)
    }

    /// v2-Punkt: keine v3-Felder (Optionals nil), frame_version 2.
    private func v2Point(t: Double) -> TrackPoint {
        TrackPoint(t: t, lat: 51.1, lon: 6.7, altitudeM: 200, speedKmph: 10, courseDeg: 0,
                   sats: 9, hdop: 1, gnssFix: .fixOK, brakeDecelMs2: 0, brakeLightPct: 0,
                   imuHealth: .ok, pressurePa: 98950, gnssAltitudeM: 205, temperatureC: 21.5,
                   deviceTimestampMs: 1234, baroValid: true, systemState: .run,
                   initDegraded: false, watchdogRecovered: false, frameVersion: 2)
    }

    private func lines(_ points: [TrackPoint], appVersion: String = "9.9.9",
                       endedAt: Date? = Date(timeIntervalSince1970: 60)) -> [String] {
        let csv = RideCSVExporter.export(points: points, startedAt: Date(timeIntervalSince1970: 0),
                                         endedAt: endedAt, appVersion: appVersion, timeZone: utc)
        return csv.components(separatedBy: "\r\n")
    }

    // MARK: - Struktur / Spaltenzahl

    @Test func columnCountIs35() {
        #expect(RideCSVExporter.columns.count == 35)
        let ls = lines([v3Point(t: 0), v2Point(t: 1)])
        let hIdx = ls.firstIndex(of: header)!
        let dataLines = Array(ls[(hIdx + 1)...])
        #expect(dataLines.count == 2)
        for row in dataLines { #expect(row.components(separatedBy: ";").count == 35) }
    }

    // MARK: - Golden-Header (verbindliche Doku, exakter String)

    @Test func goldenHeaderIncludingPreamble() {
        let ls = lines([v3Point(t: 0)])   // uniform frame_version 3 → schema_version 3, nicht gemischt
        let golden = [
            "sep=;",
            "# SmartBikeRearLight Fahrt-Export",
            "# schema_version;3",
            "# frame_version_gemischt;nein",
            "# geraet;SmartBikeRearLight",
            "# app_version;9.9.9",
            "# start_utc;1970-01-01T00:00:00Z",
            "# ende_utc;1970-01-01T00:01:00Z",
            "t_s;uhrzeit;device_timestamp_ms;speed_kmph;distanz_km;hoehe_m;pressure_pa;gnss_altitude_m;course_deg;fix_status;sats;hdop;lat;lon;brake_decel_ms2;brake_light_pct;imu_health;baro_valid;system_state;init_degraded;watchdog_recovered;frame_version;gnss_accel_ms2;pitch_rad;gyro_bias_rads;norm_delta_min;norm_delta_max;jerk_max;regime_static_n;regime_dynamic_n;regime_shock_n;bias_calibrated;gnss_accel_valid;dt_max_ms;loop_max_us",
        ]
        #expect(Array(ls.prefix(golden.count)) == golden)
    }

    // MARK: - Formatierung

    @Test func usesDecimalComma() {
        let row = lines([v3Point(t: 0)]).last!
        #expect(row.contains(";36,0;"))     // speed_kmph
        #expect(row.contains(";2,50;"))     // brake_decel_ms2
        #expect(!row.contains("."))         // nirgends ein Dezimalpunkt
    }

    @Test func timestampHasAtLeastTwoDecimals() {
        let ls = lines([v3Point(t: 0), v3Point(t: 1.5)])
        let hIdx = ls.firstIndex(of: header)!
        let data = Array(ls[(hIdx + 1)...])
        #expect(data[0].hasPrefix("0,00;"))   // t = 0
        #expect(data[1].hasPrefix("1,50;"))   // t = 1,5
        // Erste Zelle jeder Zeile: ganzzahliger Teil, Komma, ≥ 2 Nachkommastellen.
        for row in data {
            let cell = row.components(separatedBy: ";")[0]
            let parts = cell.components(separatedBy: ",")
            #expect(parts.count == 2)
            #expect(parts[1].count >= 2)
        }
    }

    // MARK: - E-2: schema_version = Geräteversion, nicht App-Version

    @Test func schemaVersionIsDeviceNotAppVersion() {
        let ls = lines([v3Point(t: 0)], appVersion: "9.9.9")   // App-Version ≠ 3
        #expect(ls.contains("# schema_version;3"))             // Geräte-Frame-Version
        #expect(ls.contains("# app_version;9.9.9"))            // App-Version separat, unverändert
        #expect(!ls.contains("# schema_version;9.9.9"))        // NICHT die App-Version
    }

    @Test func mixedFrameVersionsCarryMinimumAndFlag() {
        let mixed = lines([v2Point(t: 0), v3Point(t: 1)])      // frame_version 2 und 3
        #expect(mixed.contains("# schema_version;2"))          // Minimum
        #expect(mixed.contains("# frame_version_gemischt;ja"))

        let uniform = lines([v3Point(t: 0), v3Point(t: 1)])    // beide 3
        #expect(uniform.contains("# schema_version;3"))
        #expect(uniform.contains("# frame_version_gemischt;nein"))
    }

    // MARK: - temperature_c raus, v3-Zellen leer bei v2

    @Test func temperatureColumnRemoved() {
        #expect(!RideCSVExporter.columns.contains("temperature_c"))
        #expect(!header.contains("temperature_c"))
    }

    @Test func v2SampleLeavesV3CellsEmpty() {
        let ls = lines([v3Point(t: 0), v2Point(t: 1)])
        let hIdx = ls.firstIndex(of: header)!
        let v3Row = ls[hIdx + 1].components(separatedBy: ";")
        let v2Row = ls[hIdx + 2].components(separatedBy: ";")
        // Die 13 v3-Spalten sind die letzten 13 (Index 22…34).
        let v3Range = 22..<35
        for i in v3Range { #expect(!v3Row[i].isEmpty, "v3-Sample: Spalte \(i) darf nicht leer sein") }
        for i in v3Range { #expect(v2Row[i].isEmpty, "v2-Sample: v3-Spalte \(i) muss leer sein") }
        // Konkrete v3-Werte des v3-Samples (Dezimalkomma, Bool als 0/1, Int roh).
        #expect(v3Row[22] == "3,10")   // gnss_accel_ms2
        #expect(v3Row[31] == "1")      // bias_calibrated
        #expect(v3Row[34] == "1234")   // loop_max_us
    }
}
