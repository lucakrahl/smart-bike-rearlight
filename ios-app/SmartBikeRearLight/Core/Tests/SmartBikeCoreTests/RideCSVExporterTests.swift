import Testing
import Foundation
@testable import SmartBikeCore

/// Validierungsvollständiger Export (Metadaten-Kopf + Roh-/abgeleitete Spalten).
struct RideCSVExporterTests {

    private let utc = TimeZone(identifier: "UTC")!
    private var header: String { RideCSVExporter.columns.joined(separator: ";") }

    @Test func metadataHeaderColumnsAndValues() {
        let points = [
            TrackPoint(t: 0, lat: 51.1, lon: 6.7, altitudeM: 200, speedKmph: 0, courseDeg: 12,
                       sats: 9, hdop: 0.8, gnssFix: .fixOK, brakeDecelMs2: 0, brakeLightPct: 0,
                       imuHealth: .ok, pressurePa: 98950, gnssAltitudeM: 205, temperatureC: 21.5,
                       deviceTimestampMs: 1234, baroValid: true, systemState: .run,
                       initDegraded: false, watchdogRecovered: false, frameVersion: 2),
            TrackPoint(t: 1, lat: 51.1, lon: 6.7, altitudeM: 201.5, speedKmph: 36, courseDeg: 10,
                       sats: 9, hdop: 0.9, gnssFix: .fixOK, brakeDecelMs2: 2.5, brakeLightPct: 80,
                       imuHealth: .recovering, pressurePa: 98900, gnssAltitudeM: 206, temperatureC: 21.6,
                       deviceTimestampMs: 1334, baroValid: true, systemState: .run,
                       initDegraded: false, watchdogRecovered: true, frameVersion: 2),
        ]
        let csv = RideCSVExporter.export(points: points, startedAt: Date(timeIntervalSince1970: 0),
                                         endedAt: Date(timeIntervalSince1970: 60),
                                         appVersion: "1.2.3", timeZone: utc)
        let lines = csv.components(separatedBy: "\r\n")

        // Metadaten-Kopf.
        #expect(lines[0] == "sep=;")
        #expect(lines.contains("# schema_version;2"))
        #expect(lines.contains("# geraet;SmartBikeRearLight"))
        #expect(lines.contains("# app_version;1.2.3"))
        #expect(lines.contains("# start_utc;1970-01-01T00:00:00Z"))
        #expect(lines.contains("# ende_utc;1970-01-01T00:01:00Z"))

        // Spaltenkopf + genau 23 Spalten, danach 2 Datenzeilen.
        #expect(RideCSVExporter.columns.count == 23)
        let headerIndex = lines.firstIndex(of: header)
        #expect(headerIndex != nil)
        let dataLines = Array(lines[(headerIndex! + 1)...])
        #expect(dataLines.count == 2)
        for row in dataLines { #expect(row.components(separatedBy: ";").count == 23) }

        // Roh- und abgeleitete Werte nebeneinander + neue Felder (Zeile 1 s).
        let row = dataLines[1]
        #expect(row.hasPrefix("1,0;00:00:01;1334;"))    // t_s, uhrzeit, device_timestamp_ms
        #expect(row.contains(";201,5;98900;206,0;"))     // hoehe_m(genutzt);pressure_pa(roh);gnss_altitude_m(roh)
        #expect(row.contains(";2,50;80;RECOVERING;"))    // brake_decel;brake_light_pct;imu_health
        #expect(row.hasSuffix(";1;RUN;0;1;2"))           // baro_valid;system_state;init_degraded;watchdog;frame_version
    }

    @Test func missingOptionalsStayEmpty() {
        let points = [TrackPoint(t: 0, lat: 51.1, lon: 6.7, altitudeM: nil, speedKmph: 10,
                                 courseDeg: 0, sats: 9, hdop: 1, gnssFix: .noFix)]
        let csv = RideCSVExporter.export(points: points, startedAt: Date(timeIntervalSince1970: 0),
                                         endedAt: nil, appVersion: "x", timeZone: utc)
        let lines = csv.components(separatedBy: "\r\n")
        #expect(lines.contains("# ende_utc;"))           // leeres Ende
        let cols = lines[lines.firstIndex(of: header)! + 1].components(separatedBy: ";")
        #expect(cols.count == 23)
        #expect(cols[5] == "")        // hoehe_m
        #expect(cols[6] == "")        // pressure_pa
        #expect(cols[7] == "")        // gnss_altitude_m
        #expect(cols[8] == "")        // temperature_c
        #expect(cols[10] == "NO_FIX") // fix_status
        #expect(cols[15] == "")       // brake_decel_ms2
        #expect(cols[16] == "")       // brake_light_pct
        #expect(cols[17] == "OK")     // imu_health (Default)
        #expect(cols[18] == "")       // baro_valid
        #expect(cols[19] == "")       // system_state
        #expect(cols[22] == "")       // frame_version
    }
}
