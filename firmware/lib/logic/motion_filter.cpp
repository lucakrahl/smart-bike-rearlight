// motion_filter.cpp — Umsetzung Normbetrags-Gate + Komplementaerfilter.
// Rein, hardwarefrei. Herleitung/Nachweis s. docs/Validierung/stufe1_normgate.md.
#include "motion_filter.h"
#include <cmath>

namespace logic {

namespace {
constexpr float kTwoPi = 6.28318530718f;
// Toleranz fuer alle Schwellenvergleiche auf dt_s-Summen: 0,01f ist in
// float32 nicht exakt darstellbar, eine Summe von z. B. 100x0,01f faellt
// dadurch real ~2e-6 unter 1,0f -- ohne Epsilon wuerde ein exakt getimtes
// Fenster (z. B. die Verankerung) so systematisch eine Zykluslaenge zu spaet
// schliessen. 1e-4f (0,1 ms) liegt weit ueber jedem realistischen
// Rundungsfehler, aber weit unter jeder Sample-Periode.
constexpr float kEps = 1e-4f;

// Median von drei Werten (Ausgangsglaettung, s. update()).
float median3(float a, float b, float c) {
  const float lo = a < b ? a : b;
  const float hi = a < b ? b : a;
  if (c < lo) return lo;
  if (c > hi) return hi;
  return c;
}
}  // namespace

MotionFilter::MotionFilter(const MotionParams& params) : params_(params) {}

float MotionFilter::update(const MotionInput& in) {
  const float dt_s = in.dt_s;
  const float accel_norm = sqrtf(in.accel_x_ms2 * in.accel_x_ms2 +
                                  in.accel_y_ms2 * in.accel_y_ms2 +
                                  in.accel_z_ms2 * in.accel_z_ms2);

  if (!norm_seeded_) {
    // Erstbefuellung statt 0: ein Default-Delta von 0 vermeidet einen
    // kuenstlichen SHOCK-Fehlstart, falls die Startlage bereits geneigt
    // oder in Bewegung ist.
    prev_accel_norm_ = accel_norm;
    norm_seeded_ = true;
  }

  // Jerk auf ein 10-ms-Aequivalent normiert (MOTION_NORM_JERK_DELTA ist als
  // "m/s^2 je 10 ms" spezifiziert), damit die Schwelle unabhaengig vom
  // tatsaechlichen Abtastintervall vergleichbar bleibt.
  norm_delta_ = (dt_s > 1e-6f) ? (accel_norm - prev_accel_norm_) / dt_s * 0.01f : 0.0f;
  prev_accel_norm_ = accel_norm;
  accel_norm_ = accel_norm;

  const float norm_excess = accel_norm - params_.gravity_ms2;
  const bool is_static = fabsf(norm_excess) <= params_.norm_static_band;
  const bool shock_criterion =
      !is_static && (fabsf(norm_delta_) > params_.norm_jerk_delta ||
                      norm_excess > params_.norm_shock_delta);

  // Kumulative Shock-Zeit + Zwangsfreigabe: verhindert, dass dauerhaft raue
  // Fahrbahn (wiederholt re-getriggerter Shock) das Bremslicht unbegrenzt
  // einfriert. Erst nach MOTION_SHOCK_RESET_S durchgehend ausserhalb der
  // Shock-Kriterien wird die Sperre wieder aufgehoben.
  if (shock_criterion) {
    shock_elapsed_s_ += dt_s;
    non_shock_elapsed_s_ = 0.0f;
  } else {
    non_shock_elapsed_s_ += dt_s;
    if (non_shock_elapsed_s_ >= params_.shock_reset_s - kEps) {
      shock_elapsed_s_ = 0.0f;
      forced_release_ = false;
    }
  }
  if (shock_elapsed_s_ >= params_.shock_max_continuous_s - kEps) {
    forced_release_ = true;  // Regime faellt explizit auf DYNAMIC
  }

  MotionRegime regime;
  if (is_static) {
    regime = MotionRegime::Static;
  } else if (shock_criterion && !forced_release_) {
    regime = MotionRegime::Shock;
  } else {
    regime = MotionRegime::Dynamic;
  }
  regime_ = regime;

  // ---- Bias-Startkalibrierung (Stufe 1, ENTKOPPELT von der Verankerung) -
  // Kumulierte STATIC-Samples OHNE Zusammenhangsforderung: ein Mittelwert
  // braucht keine Nachbarschaft, nur genug unabhaengige Stichproben
  // (B-FW.11 R6 -- die urspruengliche Kopplung an das Verankerungsfenster
  // war fachlich falsch). Laeuft unabhaengig vom Verankerungsstatus, ab
  // dem allerersten Sample, bis MOTION_BIAS_CALIB_SAMPLE_THRESHOLD erreicht
  // ist -- danach nie wieder (bias_calibrated_ bleibt true, s. reset()).
  if (params_.gyro_bias_enabled && !bias_calibrated_ && is_static) {
    bias_calib_gyro_sum_ += in.gyro_x_rads;
    ++bias_calib_sample_count_;
    if (bias_calib_sample_count_ >= params_.bias_calib_sample_threshold) {
      gyro_bias_rads_ = bias_calib_gyro_sum_ / static_cast<float>(bias_calib_sample_count_);
      if (gyro_bias_rads_ > params_.gyro_bias_clamp) gyro_bias_rads_ = params_.gyro_bias_clamp;
      if (gyro_bias_rads_ < -params_.gyro_bias_clamp) gyro_bias_rads_ = -params_.gyro_bias_clamp;
      // Erst ab hier darf DYNAMIC die lange TAU_SLOW-Zeitkonstante nutzen
      // (s. Kommentar bei MOTION_COMPL_TAU_SLOW_UNCAL_S in config.h) --
      // NICHT ueber den Anker-Timeout-Pfad unten erreichbar.
      bias_calibrated_ = true;
    }
  }

  // ---- Nickwinkel-Verankerung (NUR Pitch, braucht Zusammenhang) ---------
  // Ein Filter ohne Vorgeschichte kann "bereits geneigt" nicht von "bereits
  // bremsend" unterscheiden (Feldtest-Nachweis, s. stufe1_normgate.md) --
  // deshalb wird der Bremsausgang gehalten, bis ein zusammenhaengendes
  // STATIC-Fenster (MOTION_ANCHOR_WINDOW_S) eine vertrauenswuerdige
  // Nicklage liefert.
  if (!anchored_) {
    anchor_since_start_s_ += dt_s;
    if (is_static) {
      anchor_non_static_streak_ = 0;  // Toleranzzaehler zuruecksetzen
      anchor_window_elapsed_s_ += dt_s;
      anchor_ay_sum_ += in.accel_y_ms2;
      anchor_az_sum_ += in.accel_z_ms2;
      ++anchor_sample_count_;
      if (anchor_window_elapsed_s_ >= params_.anchor_window_s - kEps) {
        // Mittelung auf ay/az-Ebene vor atan2f (nicht des Winkels selbst) --
        // robuster gegen Rauschen, keine Unstetigkeitsprobleme hier
        // relevant (kleine Winkel im STATIC-Fenster).
        const float n = static_cast<float>(anchor_sample_count_);
        pitch_rad_ = atan2f(anchor_ay_sum_ / n, anchor_az_sum_ / n);
        anchored_ = true;
      }
    } else {
      // Toleriert bis zu (anchor_non_static_tolerance - 1) aufeinander-
      // folgende Nicht-STATIC-Ausreisser, ohne das Fenster abzubrechen --
      // reisst erst beim n-ten Ausreisser in Folge (B-FW.11: reales
      // Sensorrauschen/Aliasing verursacht sonst zu haeufige Abbrueche,
      // s. bench_run_notes.md). Das Ausreisser-Sample selbst traegt NICHT
      // zur Mittelung bei (wuerde den Anker sonst kontaminieren).
      ++anchor_non_static_streak_;
      if (anchor_non_static_streak_ >= params_.anchor_non_static_tolerance) {
        anchor_window_elapsed_s_ = 0.0f;
        anchor_ay_sum_ = anchor_az_sum_ = 0.0f;
        anchor_sample_count_ = 0;
        anchor_non_static_streak_ = 0;
      }
    }
    if (!anchored_ && anchor_since_start_s_ >= params_.anchor_timeout_s - kEps) {
      // Timeout: ebene Lage annehmen -- konservativ, der Ausgang wurde bis
      // hierhin ohnehin auf 0 gehalten (s. u.). Die Bias-Kalibrierung
      // (oben) laeuft davon unberuehrt weiter.
      pitch_rad_ = 0.0f;
      anchored_ = true;
    }
  }

  if (!anchored_) {
    // Noch unverankert: Bremsausgang sicherheitshalber auf 0, bis eine
    // vertrauenswuerdige Nicklage feststeht.
    last_final_output_ = 0.0f;
    return 0.0f;
  }

  // Gyro-Nullpunkt-Schaetzung Stufe 2 (Temperaturdrift-Nachfuehrung, s. o.):
  // laeuft NUR bei ruhiger STATIC-Lage, damit eine echte Drehung/Bremsung
  // nicht faelschlich als Bias gelernt wird.
  if (params_.gyro_bias_enabled && regime == MotionRegime::Static &&
      fabsf(in.gyro_x_rads) < params_.gyro_bias_max_rate) {
    const float alpha_bias = params_.gyro_bias_tau_s / (params_.gyro_bias_tau_s + dt_s);
    gyro_bias_rads_ = alpha_bias * gyro_bias_rads_ + (1.0f - alpha_bias) * in.gyro_x_rads;
    if (gyro_bias_rads_ > params_.gyro_bias_clamp) gyro_bias_rads_ = params_.gyro_bias_clamp;
    if (gyro_bias_rads_ < -params_.gyro_bias_clamp) gyro_bias_rads_ = -params_.gyro_bias_clamp;
  }
  const float gyro_corrected = in.gyro_x_rads - gyro_bias_rads_;

  // Nickschaetzung je Regime:
  //   STATIC:  wie zuvor, aber dt-normiert (alpha_dyn statt festem alpha).
  //   DYNAMIC: sehr langsame Beimischung (tau_slow=30s) statt reiner Gyro-
  //            Propagation -- verhindert Drift, kontaminiert laengere
  //            Bremsungen aber nur sehr langsam (Minuten-Zeitskala).
  //   SHOCK:   reine Gyro-Propagation (Accel waehrend eines Stosses
  //            unbrauchbar).
  bool accel_blended = false;
  if (regime == MotionRegime::Static) {
    const float pitch_from_accel = atan2f(in.accel_y_ms2, in.accel_z_ms2);
    const float alpha_dyn = params_.compl_tau_s / (params_.compl_tau_s + dt_s);
    pitch_rad_ = alpha_dyn * (pitch_rad_ + gyro_corrected * dt_s) +
                 (1.0f - alpha_dyn) * pitch_from_accel;
    accel_blended = true;
  } else if (regime == MotionRegime::Dynamic) {
    const float pitch_from_accel = atan2f(in.accel_y_ms2, in.accel_z_ms2);
    // TAU_SLOW=90s ist nur bei kalibriertem Bias sicher (s. config.h); vor/
    // ohne Stufe-1-Kalibrierung (auch wenn MOTION_GYRO_BIAS_ENABLED=false
    // die Kalibrierung dauerhaft deaktiviert) gilt der kuerzere, konservative
    // Fallback-Wert.
    const float tau_slow = (params_.gyro_bias_enabled && !bias_calibrated_)
                                ? params_.compl_tau_slow_uncal_s
                                : params_.compl_tau_slow_s;
    const float alpha_slow = tau_slow / (tau_slow + dt_s);
    pitch_rad_ = alpha_slow * (pitch_rad_ + gyro_corrected * dt_s) +
                 (1.0f - alpha_slow) * pitch_from_accel;
    accel_blended = true;
  } else {  // Shock
    pitch_rad_ = pitch_rad_ + gyro_corrected * dt_s;
  }

  // Backstop-Watchdog (rein defensiv): die kontinuierliche tau_slow-
  // Beimischung in DYNAMIC deckt den Regelfall bereits ab; dieser Zweig
  // faengt nur den theoretischen Fall ab, dass ueber sehr lange Zeit
  // ununterbrochen SHOCK klassifiziert wuerde.
  if (accel_blended) {
    gyro_only_elapsed_s_ = 0.0f;
  } else {
    gyro_only_elapsed_s_ += dt_s;
    if (gyro_only_elapsed_s_ >= params_.max_gyro_only_s - kEps) {
      pitch_rad_ = atan2f(in.accel_y_ms2, in.accel_z_ms2);
      gyro_only_elapsed_s_ = 0.0f;
    }
  }

  // Gravitationsanteil auf der Y-Achse (Fahrtrichtung) herausrechnen.
  const float gravity_y = params_.gravity_ms2 * sinf(pitch_rad_);
  const float linear_accel_y = in.accel_y_ms2 - gravity_y;
  const float momentary = params_.brake_sign * linear_accel_y > 0.0f
                               ? params_.brake_sign * linear_accel_y
                               : 0.0f;

  // SHOCK-Hold: waehrend eines Stosses den zuletzt gueltigen Wert halten
  // statt den kontaminierten Momentanwert durchzureichen; maximal
  // MOTION_SHOCK_HOLD_MS je Episode, danach Momentanwert trotz weiterhin
  // klassifiziertem SHOCK wieder durchreichen (verhindert Maskierung einer
  // echten Dauerbremsung durch wiederholte Stoesse -- unabhaengig von der
  // laengerfristigen Zwangsfreigabe oben, die erst nach 1 s greift).
  float pre_smooth;
  if (regime == MotionRegime::Shock) {
    if (!shock_hold_active_) {
      shock_hold_active_ = true;
      shock_hold_elapsed_s_ = 0.0f;
      held_output_ = last_final_output_;
    } else {
      shock_hold_elapsed_s_ += dt_s;
    }
    pre_smooth = (shock_hold_elapsed_s_ < params_.shock_hold_s) ? held_output_ : momentary;
  } else {
    shock_hold_active_ = false;
    shock_hold_elapsed_s_ = 0.0f;
    pre_smooth = momentary;
  }

  // Schwache Ausgangsglaettung (3-Sample-Median + Einpol-Tiefpass), NICHT
  // auf accel_y vor dem Gate: Zusatzlatenz < 20 ms, NFR-RT-01 bleibt mit
  // deutlicher Reserve gewahrt.
  float output = pre_smooth;
  if (params_.output_smoothing_enabled) {
    median_buf_[2] = median_buf_[1];
    median_buf_[1] = median_buf_[0];
    median_buf_[0] = pre_smooth;
    if (median_count_ < 3) ++median_count_;
    const float median_val =
        (median_count_ < 3) ? pre_smooth : median3(median_buf_[0], median_buf_[1], median_buf_[2]);

    const float rc = 1.0f / (kTwoPi * params_.output_lpf_hz);
    const float alpha_lpf = dt_s / (rc + dt_s);
    if (!lpf_initialized_) {
      lpf_state_ = median_val;
      lpf_initialized_ = true;
    } else {
      lpf_state_ = lpf_state_ + alpha_lpf * (median_val - lpf_state_);
    }
    output = lpf_state_;
  }

  last_final_output_ = output;
  return output;
}

void MotionFilter::reset() {
  // s. Kommentar bei der Deklaration (motion_filter.h): pitch_rad_,
  // gyro_bias_rads_ und anchored_ (samt seiner Akkumulatoren, die nach
  // erfolgter Verankerung ohnehin unbenutzt bleiben) sind bewusst NICHT in
  // dieser Liste.
  norm_seeded_ = false;
  prev_accel_norm_ = 0.0f;
  accel_norm_ = 0.0f;
  norm_delta_ = 0.0f;
  shock_elapsed_s_ = 0.0f;
  non_shock_elapsed_s_ = 0.0f;
  forced_release_ = false;
  shock_hold_active_ = false;
  shock_hold_elapsed_s_ = 0.0f;
  held_output_ = 0.0f;
  last_final_output_ = 0.0f;
  gyro_only_elapsed_s_ = 0.0f;
  median_buf_[0] = median_buf_[1] = median_buf_[2] = 0.0f;
  median_count_ = 0;
  lpf_state_ = 0.0f;
  lpf_initialized_ = false;
  // Falls die Verankerung beim reset() noch nicht abgeschlossen war (sehr
  // frueher Ausfall waehrend des allerersten Anlaufs): Akkumulatoren
  // ebenfalls zuruecksetzen, das Fenster startet dann sauber neu. Ist
  // anchored_ bereits true, bleiben diese Felder unbenutzt.
  if (!anchored_) {
    anchor_window_elapsed_s_ = 0.0f;
    anchor_since_start_s_ = 0.0f;
    anchor_ay_sum_ = anchor_az_sum_ = 0.0f;
    anchor_sample_count_ = 0;
    anchor_non_static_streak_ = 0;
  }
  // bias_calib_gyro_sum_/bias_calib_sample_count_ werden bewusst NICHT
  // zurueckgesetzt (s. Kommentar bei der Deklaration in motion_filter.h) --
  // die kumulative Bias-Kalibrierung braucht keinen Zusammenhang, ein
  // kurzer Aussetzer entwertet bereits gesammelte Samples nicht.
}

}  // namespace logic
