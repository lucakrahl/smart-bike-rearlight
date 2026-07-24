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
constexpr float    INIT_BLINK_FREQ_HZ   = 2.0f;   // FR-TL-03 Diagnose-Blinken
constexpr float    ESS_BLINK_FREQ_HZ    = 4.0f;   // FR-TL-07 (experimentell)

// ---- Bremskennlinie — KALIBRIERWERTE (NVS-ueberschreibbar, FR-CFG-01) -----
constexpr float BRAKE_ON_MS2    = 2.0f;   // FR-TL-06 Einschaltschwelle
constexpr float BRAKE_OFF_MS2   = 1.5f;   // FR-TL-06 Ausschalthysterese
constexpr float BRAKE_FULL_MS2  = 5.0f;   // FR-TL-06 Saettigung (100 %)
constexpr float ESS_ON_MS2      = 5.0f;   // FR-TL-07 Notbrems-Blinken ein
constexpr float ESS_OFF_MS2     = 3.0f;   // FR-TL-07 Hysterese aus
constexpr uint32_t BRAKE_MIN_HOLD_MS = 300;  // FR-TL-06 Mindesthaltezeit
constexpr float COMPL_FILTER_ALPHA   = 0.98f; // Bible Kap. 6.4

// ---- Notbrems-Blinken default AUS (FR-TL-07, § 67 Abs. 4) -----------------
constexpr bool ESS_ENABLED_DEFAULT = false;

// ---- Sampling / Telemetrie (fest, FR-SNS-02 / FR-TEL-02) ------------------
constexpr uint32_t PERIOD_IMU_MS   = 10;    // 100 Hz
constexpr uint32_t PERIOD_BARO_MS  = 100;   // 10 Hz
constexpr uint32_t PERIOD_GNSS_MS  = 1000;  // 1 Hz
constexpr uint32_t PERIOD_TELE_MS  = 100;   // 10 Hz
constexpr uint16_t TELEMETRY_SCHEMA_VERSION = 1;  // FR-TEL-06

// ---- Ringpuffer (NFR-RES-01) ---------------------------------------------
constexpr uint16_t RINGBUFFER_FRAMES = 600;  // ~60 s @ 10 Hz

// ---- GNSS-Fix-Kriterien — KALIBRIERWERTE (NVS, FR-TEL-05) -----------------
constexpr uint32_t GNSS_MAX_AGE_MS = 3000;
constexpr uint8_t  GNSS_MIN_SATS   = 4;

// ---- RF-Codes (fest, FR-RF-01) -------------------------------------------
constexpr uint32_t RF_CODE_LEFT  = 10967538;  // Taste 1
constexpr uint32_t RF_CODE_RIGHT = 10967537;  // Taste 2

// ---- Sicherheit ----------------------------------------------------------
constexpr uint32_t WATCHDOG_TIMEOUT_MS = 2000;  // FR-SAF-03
