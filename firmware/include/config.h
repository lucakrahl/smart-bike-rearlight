// config.h — Compile-Zeit-Defaults & Konstanten (Bible Kap. 2, FR-CFG)
// Kalibrierwerte (unten markiert) sind zur Laufzeit ueber NVS ueberschreibbar
// (FR-CFG-01/02). Struktur-/Normwerte sind fest.
#pragma once
#include <stdint.h>

// ---- Konfig-Schema (FR-CFG-03) -------------------------------------------
constexpr uint16_t CONFIG_VERSION = 1;

// ---- Zustandsmaschine / Timing (fest) ------------------------------------
constexpr uint32_t INIT_TIMEOUT_MS      = 5000;   // FR-STA-01
constexpr uint32_t BLINKER_TIMEOUT_MS   = 60000;  // FR-BLK-03 (Selbstabschaltung)
constexpr uint32_t LONGPRESS_MS         = 5000;   // FR-BLK-04/07 (Warnblinker)
constexpr uint32_t RF_RELEASE_TIMEOUT_MS = 150;   // FR-RF-03 (vorlaeufig, s. Verifikationstest)

// ---- LED / PWM (fest, Norm) ----------------------------------------------
constexpr uint32_t PWM_FREQ_HZ          = 5000;   // CON-02
constexpr uint8_t  PWM_RESOLUTION_BITS  = 8;      // 0..255
constexpr uint8_t  TAILLIGHT_DUTY_PCT   = 20;     // FR-TL-04 Schlusslicht-Grundhelligkeit
constexpr float    BLINK_FREQ_HZ        = 1.5f;   // FR-BLK-08 (ECE R6)
constexpr uint8_t  BLINK_DUTY_PCT       = 50;     // FR-BLK-08 Zeit-Duty-Cycle
constexpr float    INIT_BLINK_FREQ_HZ   = 2.0f;   // FR-TL-03 Diagnose-Blinken
constexpr uint8_t  INIT_BLINK_DUTY_PCT  = 50;     // FR-TL-03 Zeit-Duty-Cycle
constexpr uint8_t  INIT_BLINK_HIGH_PCT  = 50;     // FR-TL-03, C3.1 Helligkeits-Amplitude oben (unten 0 %)
constexpr float    ESS_BLINK_FREQ_HZ    = 4.0f;   // FR-TL-07 (experimentell)
constexpr uint8_t  ESS_BLINK_DUTY_PCT   = 50;     // FR-TL-07 Zeit-Duty-Cycle (analog INIT_BLINK_DUTY_PCT)

// ---- Bremskennlinie — KALIBRIERWERTE (NVS-ueberschreibbar, FR-CFG-01) -----
constexpr float BRAKE_ON_MS2    = 2.0f;   // FR-TL-06 Einschaltschwelle
constexpr float BRAKE_OFF_MS2   = 1.5f;   // FR-TL-06 Ausschalthysterese
constexpr float BRAKE_FULL_MS2  = 5.0f;   // FR-TL-06 Saettigung (100 %)
constexpr float ESS_ON_MS2      = 5.0f;   // FR-TL-07 Notbrems-Blinken ein
constexpr float ESS_OFF_MS2     = 3.0f;   // FR-TL-07 Hysterese aus
constexpr uint32_t BRAKE_MIN_HOLD_MS = 300;  // FR-TL-06 Mindesthaltezeit
// Historischer Fixwert, seit dem Normbetrags-Gate (Feldtest 06.08.2026)
// funktional abgeloest durch MOTION_COMPL_TAU_S (dt-normiert); hier nur zur
// Traceability/Historie unveraendert belassen, nicht mehr verdrahtet.
constexpr float COMPL_FILTER_ALPHA   = 0.98f; // Bible Kap. 6.4

// ---- Motion-Filter Stufe 1 — Normbetrags-Gate (Feldtest 06.08.2026) -------
// Physikalischer Diskriminator ueber den Betrag des Beschleunigungsvektors
// (Static/Dynamic/Shock) statt reiner atan2f-Nickschaetzung, s.
// docs/Validierung/stufe1_normgate.md. Ersatz fuer den per r=-0,132 (n=939)
// belegten Fehlbefund (Scheinneigung bei anhaltender Bremsung).
constexpr float MOTION_NORM_STATIC_BAND = 0.12f;  // m/s^2, vorlaeufig, feldzuparametrieren.
// Umrechnung Winkelfehler-Toleranz -> Band ist PYTHAGOREISCH, nicht linear
// mit g skaliert: a = sqrt((g+delta)^2 - g^2). delta=0,12 -> a=1,539 m/s^2
// (g=9,80665) -- Reserve zur 2,0-m/s^2-Ansprechschwelle (BRAKE_ON_MS2).
// Umkehrformel fuer die spaetere Feldparametrierung (Zielwinkel a bekannt):
// delta = sqrt(a^2 + g^2) - g.
constexpr float MOTION_NORM_JERK_DELTA  = 2.0f;   // m/s^2 je 10 ms, dt-normiert; vorlaeufig
constexpr float MOTION_NORM_SHOCK_DELTA = 6.0f;   // m/s^2, reines Betrags-Rueckfallkriterium; vorlaeufig

constexpr uint32_t MOTION_SHOCK_HOLD_MS           = 200;   // ms, Halten je Shock-Episode
constexpr float    MOTION_SHOCK_RESET_S           = 0.5f;  // s, durchgehend shockfrei, bevor die kumulative Shock-Zeit genullt wird
constexpr float    MOTION_SHOCK_MAX_CONTINUOUS_S  = 1.0f;  // s, Zwangsfreigabe -> Regime DYNAMIC (dauerhaft raue Fahrbahn friert sonst das Bremslicht ein)

// TAU_S=0,49s stammte aus dem alten alpha=0,98 @ 10 ms und war nie fuer
// dieses System eigenstaendig begruendet. Seit das explizite Anker-Fenster
// existiert (atan2f-Seed erst bei abgeschlossener Verankerung, s.
// MOTION_ANCHOR_WINDOW_S), muss das laufende STATIC-Filter die
// Startkonvergenz nicht mehr leisten -- es fuehrt nur noch Fahrbahnneigung
// waehrend der Fahrt nach (Zeitkonstante Sekunden bis Minuten realistisch).
// Sicherheitspruefung: ein Steigungssprung von 10% = 5,7 Grad erzeugt
// g*sin(5,7 Grad)=0,98 m/s^2, deutlich unter BRAKE_ON_MS2=2,0 -- ein
// traeges Filter ist hier unkritisch (belegt in T11, s. stufe1_normgate.md
// "Totzone des STATIC-Bands").
constexpr float MOTION_COMPL_TAU_S = 3.0f;  // s, dt-normierte STATIC-Zeitkonstante

// TAU_SLOW=90s ist erst durch die zweistufige Gyro-Bias-Kalibrierung
// zulaessig: stationaerer Fehler eps = b*tau = 0,005 deg/s * 90 s
// = 0,45 deg = 0,077 m/s^2 (b = realistischer Restbias NACH Stufe 1, s.
// MOTION_GYRO_BIAS_*).
constexpr float MOTION_COMPL_TAU_SLOW_S = 90.0f;  // s, DYNAMIC-Zeitkonstante NACH abgeschlossener Bias-Kalibrierung

// Solange Stufe 1 der Bias-Kalibrierung noch nicht gelaufen ist (oder wenn
// MOTION_GYRO_BIAS_ENABLED=false die Kalibrierung komplett deaktiviert):
// bei vollem unkorrigiertem Bias (b=0,5 deg/s) waere mit TAU_SLOW=90s ein
// stationaerer Fehler von 0,5*90=45 deg moeglich -- bis zur Kalibrierung
// gilt deshalb der kuerzere, konservativere Wert.
constexpr float MOTION_COMPL_TAU_SLOW_UNCAL_S = 30.0f;  // s, DYNAMIC-Zeitkonstante VOR/OHNE Bias-Kalibrierung

constexpr uint32_t MOTION_MAX_GYRO_ONLY_MS = 30000; // ms, reiner Backstop-Watchdog -- die kontinuierliche TAU_SLOW-Beimischung deckt den Regelfall bereits ab

// Gyro-Nullpunkt-Schaetzung: PFLICHT (nicht optional) seit dem Nachweis,
// dass ein realistischer 0,5-deg/s-MPU6050-Nullpunktfehler ueber
// MOTION_COMPL_TAU_SLOW_S sonst einen Winkelfehler von ~15 deg
// (=2,54 m/s^2, ueber BRAKE_ON_MS2!) erzeugt -- s. test_motion_filter T8.
constexpr bool  MOTION_GYRO_BIAS_ENABLED  = true;
constexpr float MOTION_GYRO_BIAS_TAU_S    = 60.0f;   // s, langsamer Einpol
constexpr float MOTION_GYRO_BIAS_MAX_RATE = 0.035f;  // rad/s (~2 deg/s) -- Gate: Schaetzer laeuft nur bei ruhiger STATIC-Lage
constexpr float MOTION_GYRO_BIAS_CLAMP    = 0.087f;  // rad/s (~5 deg/s), harte Begrenzung des geschaetzten Bias

constexpr bool  MOTION_OUTPUT_SMOOTHING_ENABLED = true;
constexpr float MOTION_OUTPUT_LPF_HZ = 15.0f;  // Hz, Einpol-Tiefpass auf den Ausgang (NICHT auf accel_y vor dem Gate)

constexpr bool MOTION_DIAG_LOG_ENABLED = false;  // Vollrate-Diagnose-Log ueber Serial (main.cpp, nur BENCH_MODE)

// Nickwinkel-Verankerung: ein Filter ohne Vorgeschichte kann bei Sample 1
// "bereits geneigt" nicht von "bereits bremsend" unterscheiden -- deshalb
// wird der Bremsausgang bis zur Verankerung sicherheitshalber auf 0
// gehalten, statt (wie zuvor) sofort per atan2f zu raten. Braucht Zusammen-
// hang (kohaerente Lagereferenz) -- ANDERS als die Bias-Kalibrierung weiter
// unten (B-FW.11 R6: die urspruengliche Kopplung beider in ein gemeinsames
// Fenster war fachlich falsch, s. docs/Validierung/bench_run_notes.md).
//
// MOTION_ANCHOR_WINDOW_S 1,0->0,3 s (B-FW.11 Reparaturrunde): am realen
// Board zeigte die STATIC-Klassifikation im Ruhezustand nur ~70 % Erfolgs-
// rate (Sensorrauschen/Aliasing, s. bench_run_notes.md) -- ein 1,0-s-
// Fenster (100 Samples am Stueck) wird bei p=0,7 statistisch praktisch nie
// erreicht (0,7^100~3e-16), ein 0,3-s-Fenster (30 Samples) ist realistisch
// erreichbar. MOTION_ANCHOR_NON_STATIC_TOLERANCE toleriert zusaetzlich bis
// zu 2 aufeinanderfolgende Nicht-STATIC-Ausreisser, ohne das Fenster
// abzubrechen -- reisst erst beim 3. Ausreisser in Folge.
constexpr float   MOTION_ANCHOR_WINDOW_S = 0.3f;  // s, zusammenhaengendes STATIC-Fenster fuer die Pitch-Verankerung
constexpr uint8_t MOTION_ANCHOR_NON_STATIC_TOLERANCE = 3;  // aufeinanderfolgende Nicht-STATIC-Samples, die das Fenster abbrechen
constexpr float   MOTION_ANCHOR_TIMEOUT_S = 2.0f;  // s, Fallback: ebene Lage annehmen, falls kein STATIC-Fenster zustande kommt

// Bias-Startkalibrierung (Stufe 1): kumulierte STATIC-Abtastungen OHNE
// Zusammenhangsforderung -- ein Mittelwert braucht keine Nachbarschaft,
// nur genug unabhaengige STATIC-Samples (B-FW.11 R6). Bei ~70 % realer
// STATIC-Rate sind 200 kumulierte Samples in real ~2,9 s erreichbar (statt
// vorher: nie, s. bench_run_notes.md).
constexpr uint32_t MOTION_BIAS_CALIB_SAMPLE_THRESHOLD = 200;

// Achsen-/Vorzeichenkonvention der IMU: am realen Board verifiziert (Bremsen
// erzeugt positiven Wert der gravitationskompensierten Y-Beschleunigung).
// Y=Fahrtrichtung (Bible, gesichert), Z=oben, X=seitlich=Nickachse bei
// ebenem Stand. MOTION_BRAKE_SIGN legt das Vorzeichen fest, das als
// "Bremsen" zaehlt; entgegen der urspruenglichen Annahme ist das +1 (nicht -1).
constexpr float MOTION_BRAKE_SIGN = 1.0f;

// ---- Notbrems-Blinken default AUS (FR-TL-07, § 67 Abs. 4) -----------------
constexpr bool ESS_ENABLED_DEFAULT = false;

// ---- Sampling / Telemetrie (fest, FR-SNS-02 / FR-TEL-02) ------------------
constexpr uint32_t PERIOD_IMU_MS   = 10;    // 100 Hz
constexpr uint32_t PERIOD_BARO_MS  = 100;   // 10 Hz (Task-Slot; BMP280-FORCED-Zyklus s. BARO_FORCED_CYCLE_MS)
constexpr uint32_t PERIOD_GNSS_MS  = 1000;  // 1 Hz
constexpr uint32_t PERIOD_TELE_MS  = 100;   // 10 Hz
// v2: brake_light_pct (Offset 80) ergaenzt -- tatsaechlich kommandierte
// Ruecklicht-Duty neben dem rohen brake_decel_ms2-Eingang (FR-TEL-06).
// v3 (docs/BLE_Frame_v3_Schnittstelle.md): GNSS-Referenzbeschleunigung
// (E1, beobachtend), Filter-Innensicht (pitch_rad/gyro_bias_rads/
// bias_calibrated) und 100-Hz-Fensteraggregate (Regime-Zaehler, Jerk/Norm-
// Extrema, dt_max_ms/loop_max_us fuer NFR-RT-04) -- Offsets 81-112,
// 81->113 Byte.
constexpr uint16_t TELEMETRY_SCHEMA_VERSION = 3;  // FR-TEL-06

// BMP280 laeuft im FORCED-Mode bewusst langsamer als der 10-Hz-Task-Slot:
// Trigger und Read sind auf zwei Zyklen entkoppelt (s. bmp280_driver), das
// hier ist die Zykluszeit dazwischen. Reduziert die Selbsterwaermung
// (gemessen +2,6 °C im Dauerbetrieb) durch niedrige Duty-Cycle.
constexpr uint32_t BARO_FORCED_CYCLE_MS = 1000;

// ---- Ringpuffer (NFR-RES-01) ---------------------------------------------
constexpr uint16_t RINGBUFFER_FRAMES = 600;  // ~60 s @ 10 Hz

// ---- GNSS-Fix-Kriterien — KALIBRIERWERTE (NVS, FR-TEL-05) -----------------
constexpr uint32_t GNSS_MAX_AGE_MS = 3000;
constexpr uint8_t  GNSS_MIN_SATS   = 4;
// Schwelle fuer "noch nie sinnvolle NMEA-Daten empfangen" (NO_DATA statt
// NO_FIX) — ein einzelner NMEA-Satz ist bereits > 64 Zeichen lang.
constexpr uint32_t GNSS_MIN_CHARS_PROCESSED = 64;

// ---- GNSS-Referenzbeschleunigung (gnss_speed_ref, v3-Frame, E1) -----------
// Bewusst SCHAERFER als die obigen FR-TEL-05-Fix-Kriterien (GNSS_MIN_SATS=4)
// -- Fahrt 5 des Feldtests 06.08.2026 zeigte FIX_OK bei HDOP=0,8 trotz grob
// falscher Loesung. gnss_speed_ref dient der Korrelationsanalyse der
// motion_filter-Schwellwerte, nicht der Fix-Statusanzeige; dafuer reicht die
// bestehende Toleranz nicht.
constexpr uint8_t GNSS_SPEED_REF_MIN_SATS      = 5;
constexpr float   GNSS_SPEED_REF_MAX_HDOP      = 2.5f;
constexpr float   GNSS_SPEED_REF_MIN_SPEED_MPS = 1.5f;  // unterhalb dessen dominiert GNSS-Geschwindigkeitsrauschen die Ableitung

// ---- GNSS (fest) -----------------------------------------------------------
constexpr uint32_t GNSS_BAUD        = 9600;  // Quectel L86 NMEA-Default
constexpr uint16_t GNSS_UART_RX_BUF = 1024;  // Overflow-Reserve vor Serial2.begin()
// Debug-Ausgabe in taskGnss() nutzt den bereits vorhandenen PERIOD_GNSS_MS-
// Task-Takt (~1 Hz) — kein eigener GNSS_DEBUG_CYCLE_MS noetig.

// ---- RF-Codes (fest, FR-RF-01) -------------------------------------------
constexpr uint32_t RF_CODE_LEFT  = 10967538;  // Taste 1
constexpr uint32_t RF_CODE_RIGHT = 10967537;  // Taste 2

// ---- Sicherheit ----------------------------------------------------------
constexpr uint32_t WATCHDOG_TIMEOUT_MS = 2000;  // FR-SAF-03

// ---- I2C (fest) ------------------------------------------------------------
// Bus wird zentral einmalig in main.cpp/setup() initialisiert (Wire.begin() +
// Wire.setTimeOut()); die Treiber (imu_driver, bmp280_driver) sind reine
// Busnutzer und rufen selbst kein Wire.begin() auf (FR-SNS-03 an einer Stelle).
constexpr uint32_t I2C_TIMEOUT_MS   = 50;    // FR-SNS-03 (~25-50 ms Budget)
constexpr uint8_t  MPU6050_I2C_ADDR = 0x68;  // Bible Kap. 4.2
constexpr uint8_t  BMP280_I2C_ADDR  = 0x76;  // Bible Kap. 4.2
constexpr uint8_t  MPU6050_WHOAMI_REG = 0x75;  // MPU6050-Datenblatt Reg. 117
constexpr uint8_t  MPU6050_WHOAMI_VAL = 0x68;  // erwarteter Fixwert (s. dort)

// ---- IMU-Gesundheit / Recovery (fest, FR-SNS-04/05, FR-STA-04) ------------
constexpr uint8_t  IMU_FAIL_LIMIT               = 5;    // ~50 ms @ 100 Hz, NFR-RT-01-Budget
constexpr uint16_t IMU_FROZEN_LIMIT             = 30;   // ~300 ms @ 100 Hz [TODO(offen): Feldverifikation]
constexpr float    IMU_ACCEL_MIN_MAGNITUDE_MS2  = 3.0f; // deutlich < 1 g, schliesst "alle Achsen ~0" aus
// FSR +-16 g (s. imuBegin()): Fahrrad-Stoesse (Bordstein, Schlagloch) bis
// ~20 g moeglich, ein engeres FSR wuerde dort saettigen und die
// Plausibilitaetspruefung faelschlich ansprechen lassen. Schwelle dicht am
// Anschlag (0,95 * 16 g). [TODO(offen): Feldverifikation]
constexpr float    IMU_ACCEL_MAX_MAGNITUDE_MS2  = 149.0f;
constexpr uint8_t  IMU_RECOVERY_MAX_ATTEMPTS    = 3;
constexpr uint32_t IMU_RECOVERY_MIN_INTERVAL_MS = 5000; // Hintergrund-Reinit-Rate im FAILED-Zustand

// Sprung-/Slew-Plausibilitaet: ein einzelnes Muell-aber-in-Range-Sample darf
// keine Bremseskalation ausloesen (Fehlerinjektionstest SDA-Kurzschluss,
// s. lessons_learned.md). Vergleichsbasis ist das jeweils letzte GELESENE
// (nicht nur plausible) Sample, s. imu_health.cpp. [TODO(offen): Feldverifikation]
constexpr float    IMU_ACCEL_MAX_SLEW_MS2        = 80.0f; // ~8 g Sprung/10 ms
constexpr float    IMU_GYRO_MAX_SLEW_RADS        = 4.0f;
// Eskalation (hoehere Bremslicht-Duty) erst nach N aufeinanderfolgenden
// plausiblen Zyklen vertrauen; De-Eskalation bleibt sofort wirksam (main.cpp
// erzwingt 0 = sicheres Dauer-Schlusslicht, solange nicht vertraut wird).
// [TODO(offen): Feldverifikation]
constexpr uint8_t  IMU_ESCALATION_CONFIRM_CYCLES = 3;

// ---- BLE-Telemetrie (fest, FR-TEL-01/02, FR-SYS-04) ------------------------
// UUIDs frisch generiert (v4, uuidgen), keine Bedeutung ausser Eindeutigkeit.
constexpr const char* BLE_DEVICE_NAME        = "SmartBikeRearLight";
constexpr const char* BLE_SERVICE_UUID       = "587bb505-9f9d-4ae0-96fd-0b29adfc4b03";
constexpr const char* BLE_CHARACTERISTIC_UUID = "8c604d09-743f-4850-9109-19604a17f358";
// Bevorzugtes MTU (Server-Wunsch); tatsaechlich ausgehandelter Wert haengt vom
// Client ab, s. onMTUChange in ble_telemetry.cpp. Fuer ein 113-Byte-Frame
// (Schema v3) in einer einzigen Notification werden mindestens 116
// (Frame + 3 Byte ATT-Overhead) benoetigt -- ausgehandelt werden 185
// (Nutzlast 182), am realen Geraet bestaetigt (s. ble_brownout_fallstudie.md).
constexpr uint16_t BLE_PREFERRED_MTU = 185;
// Gesendete Funkleistung. Reduziert (statt Werkseinstellung) fuer die
// Energiebilanz (NFR-PWR); wirkt NICHT auf die RF-Kalibrierungs-Stromspitze
// waehrend NimBLEDevice::init() (setPower() greift erst danach) -- war daher
// nicht die Ursache des fruehen Brownout-Bootloops auf dem Altboard, s.
// docs/ble_brownout_fallstudie.md (Root Cause: Spannungsregler des
// Altboards). [TODO(offen): Feldverifikation -- haengt von der
// tatsaechlichen Netzteil-/Kabel-Guete ab]
constexpr int8_t BLE_TX_POWER_DBM = -12;
// Anzahl gepufferter Frames, die taskTelemetry() nach einem Reconnect pro
// 10-Hz-Tick zusaetzlich zum Live-Frame nachliefert (Backfill, FR-TEL-01/04).
// [TODO(offen): Feldverifikation -- Kompromiss Aufholtempo vs. BLE-Stack-Last]
constexpr uint8_t BLE_BACKFILL_FRAMES_PER_TICK = 5;

// ---- Debug (temporaer) -----------------------------------------------------
constexpr bool DEBUG_SERIAL = true;  // schaltet TODO(temp debug)-Ausgaben; vor Auslieferung false
