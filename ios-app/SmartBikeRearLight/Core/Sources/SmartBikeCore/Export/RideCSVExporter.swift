import Foundation

/// Baut die CSV-Repräsentation einer Fahrt für den Export (rein, host-testbar).
///
/// Aufbau: `sep=;` (Excel-Trennerhinweis) → Metadaten-Kopf (`#`-Zeilen) → Spaltenkopf →
/// Datenzeilen. Excel-Konvention (deutsch): Trenner **Semikolon** `;`, Dezimaltrennzeichen
/// **Komma** `,`, Zeilenende **CRLF** (`\r\n`); die App-Schicht stellt beim Schreiben ein
/// UTF-8-BOM voran (Umlaute). `fix_status` als `FIX_OK`/`NO_FIX`/`NO_DATA`; leere
/// Optionalfelder bleiben leer. Roh- und abgeleitete Werte stehen nebeneinander
/// (pressure_pa/gnss_altitude_m roh ↔ hoehe_m genutzt).
///
/// AP7: 35 Spalten. `temperature_c` entfällt aus der CSV (bleibt in der Persistenz),
/// die 13 v3-Analysefelder werden in Offset-Reihenfolge (81→111) hinten angehängt.
/// `schema_version` in der Präambel ist die **Geräte**-Frame-Version (aus `frame_version`),
/// nicht die App-Version; bei gemischten Versionen innerhalb einer Fahrt trägt sie das
/// Minimum plus die Zeile `# frame_version_gemischt;ja` (E-2). `t_s` mit 2 Nachkommastellen (E-4).
public enum RideCSVExporter {
    public static let delimiter = ";"

    public static let columns = [
        "t_s", "uhrzeit", "device_timestamp_ms",
        "speed_kmph", "distanz_km",
        "hoehe_m", "pressure_pa", "gnss_altitude_m",
        "course_deg", "fix_status", "sats", "hdop", "lat", "lon",
        "brake_decel_ms2", "brake_light_pct", "imu_health",
        "baro_valid", "system_state", "init_degraded", "watchdog_recovered", "frame_version",
        // v3-Analyse-/Aggregatfelder in Offset-Reihenfolge (81→111); leer bei v2-Samples.
        "gnss_accel_ms2", "pitch_rad", "gyro_bias_rads",
        "norm_delta_min", "norm_delta_max", "jerk_max",
        "regime_static_n", "regime_dynamic_n", "regime_shock_n",
        "bias_calibrated", "gnss_accel_valid", "dt_max_ms", "loop_max_us",
    ]

    public static func export(points: [TrackPoint], startedAt: Date, endedAt: Date?,
                              appVersion: String, deviceName: String = "SmartBikeRearLight",
                              timeZone: TimeZone = .current) -> String {
        let cumulative = StatisticsEngine().cumulativeDistanceKm(for: points)

        // schema_version = Geräte-Frame-Version. Bei gemischten Versionen (E-2) das Minimum
        // und ein Kennzeichen. Ohne jegliche frame_version fällt es auf v2 zurück.
        let frameVersions = points.compactMap(\.frameVersion)
        let schemaVersion = frameVersions.min() ?? Int(TelemetryFrame.schemaVersion)
        let mixed = Set(frameVersions).count > 1

        let timeFormatter = DateFormatter()
        timeFormatter.locale = Locale(identifier: "de_DE")
        timeFormatter.timeZone = timeZone
        timeFormatter.dateFormat = "HH:mm:ss"

        let iso = ISO8601DateFormatter()   // Standard: UTC, „…Z"

        var lines: [String] = [
            "sep=\(delimiter)",
            "# SmartBikeRearLight Fahrt-Export",
            "# schema_version\(delimiter)\(schemaVersion)",
            "# frame_version_gemischt\(delimiter)\(mixed ? "ja" : "nein")",
            "# geraet\(delimiter)\(deviceName)",
            "# app_version\(delimiter)\(appVersion)",
            "# start_utc\(delimiter)\(iso.string(from: startedAt))",
            "# ende_utc\(delimiter)\(endedAt.map { iso.string(from: $0) } ?? "")",
            columns.joined(separator: delimiter),
        ]

        for (i, p) in points.enumerated() {
            let row = [
                dec(p.t, 2),                         // E-4: ≥ 2 Nachkommastellen
                timeFormatter.string(from: startedAt.addingTimeInterval(p.t)),
                p.deviceTimestampMs.map(String.init) ?? "",
                dec(p.speedKmph, 1),
                dec(cumulative[i], 3),
                dec(p.altitudeM, 1),                 // genutzte Höhe (abgeleitet)
                dec(p.pressurePa, 0),                // roh (Baro-Quelle)
                dec(p.gnssAltitudeM, 1),             // roh (GNSS)
                dec(p.courseDeg, 1),
                fixStatus(p.gnssFix),
                "\(p.sats)",
                dec(p.hdop, 1),
                dec(p.lat, 6),
                dec(p.lon, 6),
                dec(p.brakeDecelMs2, 2),
                intStr(p.brakeLightPct),
                imuHealth(p.imuHealth),
                boolStr(p.baroValid),
                systemState(p.systemState),
                boolStr(p.initDegraded),
                boolStr(p.watchdogRecovered),
                intStr(p.frameVersion),
                // v3 (Offset 81→111); leere Zelle bei v2-Samples (Optional == nil).
                dec(p.gnssAccelMs2, 2),
                dec(p.pitchRad, 4),
                dec(p.gyroBiasRads, 4),
                dec(p.normDeltaMin, 3),
                dec(p.normDeltaMax, 3),
                dec(p.jerkMax, 3),
                intStr(p.regimeStaticN),
                intStr(p.regimeDynamicN),
                intStr(p.regimeShockN),
                boolStr(p.biasCalibrated),
                boolStr(p.gnssAccelValid),
                intStr(p.dtMaxMs),
                intStr(p.loopMaxUs),
            ]
            lines.append(row.joined(separator: delimiter))
        }
        return lines.joined(separator: "\r\n")
    }

    // MARK: - Formatierung

    /// Fixkommazahl mit Dezimalkomma; leeres Feld bei `nil`.
    private static func dec(_ value: Double?, _ places: Int) -> String {
        guard let value else { return "" }
        return String(format: "%.\(places)f", value).replacingOccurrences(of: ".", with: ",")
    }

    private static func intStr(_ value: Int?) -> String { value.map(String.init) ?? "" }
    private static func boolStr(_ value: Bool?) -> String { value.map { $0 ? "1" : "0" } ?? "" }

    private static func fixStatus(_ fix: GnssFixStatus) -> String {
        switch fix {
        case .noData: return "NO_DATA"
        case .noFix:  return "NO_FIX"
        case .fixOK:  return "FIX_OK"
        }
    }

    private static func imuHealth(_ s: ImuHealthState) -> String {
        switch s {
        case .ok:         return "OK"
        case .recovering: return "RECOVERING"
        case .failed:     return "FAILED"
        }
    }

    private static func systemState(_ s: SystemState?) -> String {
        switch s {
        case .initializing: return "INIT"
        case .run:          return "RUN"
        case nil:           return ""
        }
    }
}
