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
constexpr float COMPL_FILTER_ALPHA   = 0.98f; // Bible Kap. 6.4

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
constexpr uint16_t TELEMETRY_SCHEMA_VERSION = 2;  // FR-TEL-06

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
// Client (Web-Bluetooth-Stack) ab, s. onMTUChange in ble_telemetry.cpp. Fuer
// ein 81-Byte-Frame in einer einzigen Notification werden mindestens 84
// (Frame + 3 Byte ATT-Overhead) benoetigt. [TODO(offen): Feldverifikation
// mit der spaeteren Web-App]
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
