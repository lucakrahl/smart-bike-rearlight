// motion_filter.h — Normbetrags-Gate + Komplementaerfilter (R4->R2)
// REINE LOGIK, hardwarefrei (NFR-TST-01): kein #include <Arduino.h>.
// Nimmt IMU-Rohwerte + Zeitschritt als einfache Eingabewerte entgegen (kein
// millis()); kennt nicht den konkreten IMU-Treiber. Host-testbar (siehe
// firmware/test/test_motion_filter/).
//
// Stufe 1 (Feldtest 06.08.2026): klassifiziert jedes Sample ueber den Betrag
// des Beschleunigungsvektors in ein Regime (Static/Dynamic/Shock) und
// entscheidet erst danach, wie stark der Beschleunigungssensor die
// Nickschaetzung stuetzen darf -- s. docs/Validierung/stufe1_normgate.md.
// Physikalischer Hintergrund: Neigung UND anhaltende Bremsung sind beide
// niederfrequent (keine Frequenztrennung moeglich), aber ‖a‖ unterscheidet
// sie zuverlaessig (reine Neigung: ‖a‖=g; Bremsung/Stoss: ‖a‖>g).
//
// Achsen-/Vorzeichenkonvention am realen Board verifiziert, s. Kommentar bei
// MOTION_BRAKE_SIGN in config.h (Y=Fahrtrichtung, Z=oben, X=seitlich;
// Bremsen erzeugt einen positiven Wert).
#pragma once
#include <cstdint>
#include "config.h"

namespace logic {

enum class MotionRegime {
  Static,   // ‖a‖≈g: Beschleunigungssensor ist glaubwuerdig die Schwerkraft
  Dynamic,  // echte Laengs-/Querbeschleunigung (Bremsung, Kurvenfahrt)
  Shock,    // Stoss/Impuls (Fahrbahnunebenheit) -- Accel unbrauchbar
};

struct MotionParams {
  float gravity_ms2 = 9.80665f;            // Standardschwerebeschleunigung
  float brake_sign  = MOTION_BRAKE_SIGN;   // kalibriert am realen Board, s. config.h

  float norm_static_band = MOTION_NORM_STATIC_BAND;
  float norm_jerk_delta  = MOTION_NORM_JERK_DELTA;
  float norm_shock_delta = MOTION_NORM_SHOCK_DELTA;

  float shock_hold_s           = MOTION_SHOCK_HOLD_MS / 1000.0f;
  float shock_reset_s          = MOTION_SHOCK_RESET_S;
  float shock_max_continuous_s = MOTION_SHOCK_MAX_CONTINUOUS_S;

  float compl_tau_s            = MOTION_COMPL_TAU_S;             // STATIC-Zeitkonstante
  float compl_tau_slow_s       = MOTION_COMPL_TAU_SLOW_S;        // DYNAMIC-Zeitkonstante NACH Bias-Kalibrierung
  float compl_tau_slow_uncal_s = MOTION_COMPL_TAU_SLOW_UNCAL_S;  // DYNAMIC-Zeitkonstante VOR/OHNE Bias-Kalibrierung
  float max_gyro_only_s        = MOTION_MAX_GYRO_ONLY_MS / 1000.0f;  // Backstop-Watchdog

  float   anchor_window_s = MOTION_ANCHOR_WINDOW_S;
  uint8_t anchor_non_static_tolerance = MOTION_ANCHOR_NON_STATIC_TOLERANCE;
  float   anchor_timeout_s = MOTION_ANCHOR_TIMEOUT_S;

  bool     gyro_bias_enabled  = MOTION_GYRO_BIAS_ENABLED;
  float    gyro_bias_tau_s    = MOTION_GYRO_BIAS_TAU_S;
  float    gyro_bias_max_rate = MOTION_GYRO_BIAS_MAX_RATE;
  float    gyro_bias_clamp    = MOTION_GYRO_BIAS_CLAMP;
  uint32_t bias_calib_sample_threshold = MOTION_BIAS_CALIB_SAMPLE_THRESHOLD;

  bool  output_smoothing_enabled = MOTION_OUTPUT_SMOOTHING_ENABLED;
  float output_lpf_hz            = MOTION_OUTPUT_LPF_HZ;
};

struct MotionInput {
  float accel_x_ms2, accel_y_ms2, accel_z_ms2;  // Rohbeschleunigung inkl. g
  float gyro_x_rads;                             // Drehrate um die Nickachse (X)
  float dt_s;                                    // Zeitschritt seit letztem Sample (real gemessen, s. main.cpp)
};

class MotionFilter {
 public:
  explicit MotionFilter(const MotionParams& params = MotionParams());

  // Verarbeitet ein IMU-Sample. Rueckgabe: nur die Beschleunigung in
  // Brems-Richtung (Y-Achse, gravitationskompensiert), als positiver Wert
  // (Eingang fuer FR-TL-06). Beschleunigen in Nicht-Brems-Richtung -> ~0,
  // damit Sprints/Antritte nicht das Bremslicht ausloesen.
  float update(const MotionInput& in);

  // Setzt den Filterzustand ZUM TEIL zurueck (z. B. nach einer kurzen IMU-
  // Ausfall-Phase, s. main.cpp):
  //   ZURUECKGESETZT: prev_accel_norm_ (+Seed-Flag), shock_elapsed_s_,
  //     non_shock_elapsed_s_, Shock-Hold-Zustand, Median-Puffer,
  //     Tiefpass-Zustand, die Pitch-Verankerungs-Akkumulatoren (sofern
  //     noch !anchored_).
  //   ERHALTEN: pitch_rad_, der geschaetzte Gyro-Nullpunktfehler
  //     (gyroBiasRads()), der Verankerungsstatus (anchored()), der
  //     Bias-Kalibrierstatus (biasCalibrated()) UND der laufende
  //     Bias-Kalibrierungs-Akkumulator (kumulierte STATIC-Samples ohne
  //     Zusammenhangsforderung -- ein kurzer Aussetzer entwertet bereits
  //     gesammelte Samples nicht, s. B-FW.11 R6).
  // Begruendung: nach einem kurzen Aussetzer ist die Einbaulage weiterhin
  // gueltig -- eine Neuverankerung mitten in einer Bremsung waere schaedlich
  // (wuerde den Ausgang erneut auf 0 zwingen, bis ein neues STATIC-Fenster
  // oder der 2-s-Timeout durchlaeuft).
  void reset();

  // Diagnose-Zugriff (Punkt A.2.5 / Feldtest-Nachweis "Scheinneigung", s.
  // BENCH_MODE-Experiment D in main.cpp sowie MOTION_DIAG_LOG_ENABLED).
  MotionRegime regime() const { return regime_; }
  float pitch_rad() const { return pitch_rad_; }
  float accel_norm() const { return accel_norm_; }
  float normDelta() const { return norm_delta_; }       // Jerk, 10-ms-normiert
  float gyroBiasRads() const { return gyro_bias_rads_; }
  bool  anchored() const { return anchored_; }
  bool  biasCalibrated() const { return bias_calibrated_; }  // Stufe 1 abgeschlossen? (s. MOTION_COMPL_TAU_SLOW_UNCAL_S)

 private:
  MotionParams params_;

  bool  norm_seeded_ = false;   // fuer prev_accel_norm_-Erstbefuellung (Jerk-Basis)
  float pitch_rad_ = 0.0f;
  float prev_accel_norm_ = 0.0f;
  float accel_norm_ = 0.0f;
  float norm_delta_ = 0.0f;

  // Nickwinkel-Verankerung (NUR Pitch -- braucht Zusammenhang, s.
  // motion_filter.cpp und config.h-Kommentar bei MOTION_ANCHOR_WINDOW_S).
  bool     anchored_ = false;
  float    anchor_window_elapsed_s_ = 0.0f;
  float    anchor_since_start_s_ = 0.0f;
  float    anchor_ay_sum_ = 0.0f;
  float    anchor_az_sum_ = 0.0f;
  uint32_t anchor_sample_count_ = 0;
  uint8_t  anchor_non_static_streak_ = 0;  // Toleranzzaehler, s. MOTION_ANCHOR_NON_STATIC_TOLERANCE

  // Bias-Startkalibrierung (Stufe 1, ENTKOPPELT von der Verankerung,
  // B-FW.11 R6): kumulierte STATIC-Samples ohne Zusammenhangsforderung.
  // NICHT von reset() betroffen (s. reset()-Kommentar oben).
  float    bias_calib_gyro_sum_ = 0.0f;
  uint32_t bias_calib_sample_count_ = 0;

  MotionRegime regime_ = MotionRegime::Static;
  bool  forced_release_ = false;      // Zwangsfreigabe nach 1 s Dauer-Shock
  float shock_elapsed_s_ = 0.0f;      // kumulative Shock-Zeit (auch ueber mehrere re-getriggerte Episoden)
  float non_shock_elapsed_s_ = 0.0f;  // durchgehend shockfrei, fuer den Reset von shock_elapsed_s_

  bool  shock_hold_active_ = false;
  float shock_hold_elapsed_s_ = 0.0f;
  float held_output_ = 0.0f;
  float last_final_output_ = 0.0f;

  float gyro_only_elapsed_s_ = 0.0f;  // Backstop-Watchdog (MOTION_MAX_GYRO_ONLY_MS)

  float gyro_bias_rads_ = 0.0f;  // ueberlebt reset(), s. Kommentar oben
  bool  bias_calibrated_ = false;  // Stufe 1 abgeschlossen? ueberlebt reset() ebenso (gehoert zur gelernten Sensoreigenschaft)

  // Ausgangsglaettung: 3-Sample-Median + Einpol-Tiefpass (MOTION_OUTPUT_LPF_HZ)
  float   median_buf_[3] = {0.0f, 0.0f, 0.0f};
  uint8_t median_count_ = 0;
  float   lpf_state_ = 0.0f;
  bool    lpf_initialized_ = false;
};

}  // namespace logic
