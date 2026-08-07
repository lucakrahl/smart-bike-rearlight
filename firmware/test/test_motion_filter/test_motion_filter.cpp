// Host-Unit-Test des Normbetrags-Gate-Komplementaerfilters (NFR-TST-03).
// Laeuft ohne ESP32: pio test -e native
//
// Herleitung/Nachweis der Regime-/Verankerungslogik: s.
// docs/Validierung/stufe1_normgate.md (Feldtest 06.08.2026, r=-0,132 n=939).
#include <unity.h>
#include <cmath>
#include <cstdio>
#include "motion_filter.h"
#include "tail_light_fsm.h"
#include "imu_health.h"
#include "config.h"

using namespace logic;

namespace {

constexpr float G = 9.80665f;
constexpr float DT = 0.01f;  // 100 Hz Standardtakt fuer die Tests

// Ruhige, ebene Lage -- schliesst SOWOHL die (seit B-FW.11 entkoppelte,
// kuerzere) Nickwinkel-Verankerung (MOTION_ANCHOR_WINDOW_S) ALS AUCH die
// Bias-Startkalibrierung (MOTION_BIAS_CALIB_SAMPLE_THRESHOLD kumulierte
// STATIC-Samples, kein Zusammenhang noetig) ab. Laufzeit = das Maximum
// beider Anforderungen (bei den aktuellen Konstanten dominiert die
// Bias-Schwelle: 200 Samples = 2,0 s > 0,3-s-Anker). gyro_bias_dps: fest
// anliegender simulierter Sensor-Nullpunktfehler waehrend des Fensters
// (0 => idealer Sensor).
void anchorLevel(MotionFilter& f, float gyro_bias_dps = 0.0f) {
  const float gyro_rads = gyro_bias_dps * (3.14159265f / 180.0f);
  const int anchor_steps = static_cast<int>(MOTION_ANCHOR_WINDOW_S / DT + 0.5f);
  const int bias_steps = static_cast<int>(MOTION_BIAS_CALIB_SAMPLE_THRESHOLD);
  const int steps = (bias_steps > anchor_steps) ? bias_steps : anchor_steps;
  for (int i = 0; i < steps; ++i) {
    f.update({0.0f, 0.0f, G, gyro_rads, DT});
  }
}

}  // namespace

// T1: Reine Neigung erzeugt kein Bremssignal.
void test_pure_tilt_yields_no_brake_signal() {
  MotionFilter filter;
  const float tilt_rad = 0.17453f;  // 10 Grad
  const float ay = G * sinf(tilt_rad);
  const float az = G * cosf(tilt_rad);

  float max_out = 0.0f;
  for (int i = 0; i < 300; ++i) {  // 3 s
    const float out = filter.update({0.0f, ay, az, 0.0f, DT});
    if (out > max_out) max_out = out;
  }
  TEST_ASSERT_TRUE(max_out < 0.3f);
}

// T2: Anhaltende Bremsung wird NICHT weggefiltert (Kernregression zum
// Feldbefund, r=-0,132). STATIC-Vorlauf VOR dem Bremseinsatz bildet den
// realen Betriebsfall ab (der Filter laeuft im Fahrbetrieb durchgehend und
// ist bereits verankert + kalibriert, bevor je gebremst wird) -- ein
// Kaltstart direkt in eine Bremsung kann ein einzelnes Sample grundsaetzlich
// nicht von "bereits geneigt" unterscheiden, s. T15 fuer genau diesen Fall.
void test_sustained_braking_survives_static_lead_in() {
  MotionFilter filter;
  anchorLevel(filter);  // ebene Ruhe -> pitch_rad_=0 verankert + Bias kalibriert

  const float checkpoints_s[4] = {0.5f, 1.0f, 2.0f, 3.0f};
  int cp_idx = 0;
  int step = 0;
  const int total_steps = static_cast<int>(3.0f / DT + 0.5f);
  for (; step < total_steps; ++step) {
    const float out = filter.update({0.0f, 4.0f, G, 0.0f, DT});
    const float t_s = (step + 1) * DT;
    if (cp_idx < 4 && t_s >= checkpoints_s[cp_idx] - DT / 2.0f) {
      char msg[96];
      snprintf(msg, sizeof(msg), "Checkpoint t=%.1fs: out=%.3f (erwartet 4,0+-0,5)",
               checkpoints_s[cp_idx], out);
      TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5f, 4.0f, out, msg);
      ++cp_idx;
    }
  }
}

// T3: Uebergang Neigung -> Bremsung im Gefaelle.
void test_tilt_then_braking_on_slope() {
  MotionFilter filter;
  const float tilt_rad = 0.13963f;  // 8 Grad
  const float ay_tilt = G * sinf(tilt_rad);
  const float az_tilt = G * cosf(tilt_rad);

  float out = 0.0f;
  for (int i = 0; i < 200; ++i) {  // 2 s reine Neigung (schliesst Verankerung nach 1 s ein)
    out = filter.update({0.0f, ay_tilt, az_tilt, 0.0f, DT});
  }
  TEST_ASSERT_TRUE_MESSAGE(out < 0.3f, "Phase 1 (Neigung) sollte kein Bremssignal liefern");

  for (int i = 0; i < 200; ++i) {  // 2 s zusaetzlich 3,5 m/s^2 im Gefaelle
    out = filter.update({0.0f, ay_tilt + 3.5f, az_tilt, 0.0f, DT});
  }
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.7f, 3.5f, out,
                                    "Phase 2 (Bremsung im Gefaelle) sollte ~3,5 liefern");
}

// T4: Stossimpuls wird unterdrueckt (drei Einzel-Sample-Stoesse auf der
// Brems-Achse selbst, nicht nur lateral -- sonst waere der Test trivial,
// da ein reiner ax-Spike ay/den Ausgang gar nicht beruehrt).
void test_shock_impulse_suppressed() {
  MotionFilter filter;
  anchorLevel(filter);

  float max_out = 0.0f;
  const int spike_at_step[3] = {20, 80, 140};  // je 0,6 s auseinander (> MOTION_SHOCK_RESET_S)
  for (int i = 0; i < 200; ++i) {  // 2 s Konstantfahrt
    bool spike = (i == spike_at_step[0] || i == spike_at_step[1] || i == spike_at_step[2]);
    const float ay = spike ? 29.0f : 0.0f;
    const float out = filter.update({0.0f, ay, G, 0.0f, DT});
    if (out > max_out) max_out = out;
  }
  TEST_ASSERT_TRUE_MESSAGE(max_out < BRAKE_ON_MS2, "Stoss darf die Ansprechschwelle nie erreichen");
}

// T5: Stoss waehrend echter Bremsung maskiert die Bremsung nicht dauerhaft.
void test_shock_during_braking_does_not_mask_it() {
  MotionFilter filter;
  anchorLevel(filter);

  for (int i = 0; i < 100; ++i) {  // 1 s Bremsung vor dem Stoss
    filter.update({0.0f, 4.0f, G, 0.0f, DT});
  }
  filter.update({0.0f, 29.0f, G, 0.0f, DT});  // 1 Sample Stoss mitten in der Bremsung

  float out = 0.0f;
  for (int i = 0; i < 25; ++i) {  // 250 ms weiter bremsen (> Hold + Ruecksprung-Uebergang)
    out = filter.update({0.0f, 4.0f, G, 0.0f, DT});
  }
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5f, 4.0f, out,
                                    "250 ms nach dem Stoss sollte die Bremsung wieder sauber erkannt sein");
}

// T6: Regime-Sequenz -- Sprung auf ein 6-m/s^2-Bremsprofil ueber mehrere
// 10-ms-Schritte ist DYNAMIC, nicht SHOCK (‖a‖=11,5 bei 6 m/s^2 Bremsung,
// deutlich unter dem SHOCK-Rueckfallkriterium 6,0 m/s^2 Normueberschuss).
void test_regime_ramp_to_braking_is_dynamic_not_shock() {
  MotionFilter filter;
  anchorLevel(filter);

  for (int i = 1; i <= 10; ++i) {  // 100 ms Rampe, 0,6 m/s^2 je 10-ms-Schritt
    filter.update({0.0f, 0.6f * i, G, 0.0f, DT});
  }
  filter.update({0.0f, 6.0f, G, 0.0f, DT});
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MotionRegime::Dynamic), static_cast<int>(filter.regime()));
}

// T7: Verankerung bei bereits geneigtem Kaltstart -- kein Einschwing-
// Fehlsignal, weder waehrend der Verankerungsphase (Ausgang gehalten bei 0)
// noch danach (Verankerung liefert den korrekten Winkel).
void test_anchoring_at_tilted_cold_start_yields_no_spurious_signal() {
  MotionFilter filter;
  const float tilt_rad = 0.2618f;  // 15 Grad
  const float ay = G * sinf(tilt_rad);
  const float az = G * cosf(tilt_rad);

  float max_out = 0.0f;
  for (int i = 0; i < 150; ++i) {  // 1,5 s: deckt das 1-s-Verankerungsfenster + Marge ab
    const float out = filter.update({0.0f, ay, az, 0.0f, DT});
    if (out > max_out) max_out = out;
  }
  TEST_ASSERT_TRUE_MESSAGE(max_out < 0.3f, "Weder waehrend noch nach der Verankerung ein Fehlsignal");
  TEST_ASSERT_TRUE_MESSAGE(filter.anchored(), "Nach 1,5 s muss die Verankerung abgeschlossen sein");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.02f, tilt_rad, filter.pitch_rad(),
                                    "Verankerter Nickwinkel sollte der realen 15-Grad-Neigung entsprechen");
}

// T8: Drift-Nachweis OHNE Bias-Schaetzung -- belegt Beschraenktheit (nicht
// Kleinheit) des Winkelfehlers; Charakterisierungstest, keine Forderung.
// gyro_bias_enabled=false bedeutet: bias_calibrated_ bleibt fuer immer
// false, daher nutzt DYNAMIC durchgehend die KALIBRIERTE Zeitkonstante
// MOTION_COMPL_TAU_SLOW_S=90s (nicht den kuerzeren UNCAL-Fallback -- der
// greift nur waehrend/ohne eine noch AUSSTEHENDE, aber grundsaetzlich
// aktivierte Kalibrierung, s. Kommentar in config.h). Herleitung:
// stationaerer Fehler eines Komplementaerfilters bei konstantem Gyro-Bias b
// und Zeitkonstante tau ist fehler(t)=b*tau*(1-exp(-t/tau)); bei
// b=0,5 deg/s, tau=90s, t=60s (=2/3*tau):
// 0,5*90*(1-exp(-60/90)) = 45*0,4866 = 21,90 Grad (per Simulation bestaetigt,
// s. Assertion unten -- deckungsgleich mit der Handrechnung). Das entspraeche
// g*sin(21,9 Grad)=3,66 m/s^2 -- deutlich ueber BRAKE_ON_MS2, ein groesseres
// Dauerfalschsignal als beim vorherigen TAU_SLOW=30s (12,97 Grad) - das ist
// die direkte, gewollte Konsequenz der Verlaengerung auf 90s: sie ist nur
// bei tatsaechlich kalibriertem Bias sicher (s. T14), und T8 zeigt genau
// diesen Trade-off (deshalb "Charakterisierung", keine Forderung).
// ax (statt ay/az) haelt DYNAMIC am Laufen, ohne den atan2f(ay,az)-Zielwert
// zu kontaminieren (ay=0,az=g bleiben durchgehend die physikalische
// Wahrheit) -- isoliert so den Gyro-Bias-Effekt vom Bremskontaminations-
// Effekt aus T2.
void test_gyro_drift_bounded_without_bias_estimation() {
  MotionParams params;
  params.gyro_bias_enabled = false;
  MotionFilter filter(params);

  const float bias_rad_s = 0.5f * (3.14159265f / 180.0f);
  anchorLevel(filter, /*gyro_bias_dps=*/0.5f);  // Sensor-Bias liegt bereits waehrend der Verankerung an

  for (int i = 0; i < 6000; ++i) {  // 60 s DYNAMIC (ax haelt Regime dynamisch, ay/az bleiben Wahrheit)
    filter.update({2.0f, 0.0f, G, bias_rad_s, DT});
  }

  const float pitch_deg = filter.pitch_rad() * (180.0f / 3.14159265f);
  char msg[96];
  snprintf(msg, sizeof(msg), "pitch=%.2f deg (erwartet ~21,9 deg, beschraenkt, nicht klein)", pitch_deg);
  TEST_ASSERT_TRUE_MESSAGE(pitch_deg > 17.0f && pitch_deg < 27.0f, msg);
}

// T9: Beschleunigen (Nicht-Brems-Richtung) loest kein Bremslicht aus.
void test_accelerating_in_non_braking_direction_yields_zero() {
  MotionFilter filter;
  anchorLevel(filter);

  float max_out = 0.0f;
  for (int i = 0; i < 200; ++i) {  // 2 s Antritt/Sprint
    const float out = filter.update({0.0f, -2.5f, G, 0.0f, DT});
    if (out > max_out) max_out = out;
  }
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, max_out);
}

// T10: dt-Robustheit -- identisches Bremsprofil bei dt=10 ms vs. dt=12 ms
// weicht um weniger als 0,3 m/s^2 voneinander ab (Nachweis, dass die
// dt-normierten Groessen -- Jerk, alpha_dyn/alpha_slow -- tatsaechlich
// abtastratenunabhaengig sind).
void test_dt_robustness() {
  auto run = [](float dt) {
    MotionFilter filter;
    const int anchor_steps = static_cast<int>(MOTION_ANCHOR_WINDOW_S / dt + 0.5f);
    for (int i = 0; i < anchor_steps; ++i) {
      filter.update({0.0f, 0.0f, G, 0.0f, dt});
    }
    float out = 0.0f;
    const int brake_steps = static_cast<int>(2.0f / dt + 0.5f);  // 2 s Bremsung
    for (int i = 0; i < brake_steps; ++i) {
      out = filter.update({0.0f, 4.0f, G, 0.0f, dt});
    }
    return out;
  };

  const float out_10ms = run(0.010f);
  const float out_12ms = run(0.012f);
  char msg[96];
  snprintf(msg, sizeof(msg), "10ms=%.3f 12ms=%.3f", out_10ms, out_12ms);
  TEST_ASSERT_TRUE_MESSAGE(fabsf(out_10ms - out_12ms) < 0.3f, msg);
}

// T11: Rampe auf 2,2 m/s^2 in 300 ms, 3 s halten -> Ausgang durchgehend
// >= 2,0 (BRAKE_ON_MS2). Knapp oberhalb der Ansprechschwelle gewaehlt, um
// zu belegen, dass die MOTION_COMPL_TAU_SLOW_S-Beimischung ueber diesen
// Zeitraum nicht so weit "wegdriftet", dass die FR-TL-06-Ansprechschwelle
// unterschritten wird.
void test_ramp_to_2p2_stays_above_brake_on_threshold() {
  MotionFilter filter;
  anchorLevel(filter);

  for (int i = 1; i <= 30; ++i) {  // 300 ms Rampe, 30 Schritte
    filter.update({0.0f, 2.2f * i / 30.0f, G, 0.0f, DT});
  }

  float min_out = 1000.0f;
  for (int i = 0; i < 300; ++i) {  // 3 s halten
    const float out = filter.update({0.0f, 2.2f, G, 0.0f, DT});
    if (out < min_out) min_out = out;
  }
  char msg[64];
  snprintf(msg, sizeof(msg), "min_out=%.3f", min_out);
  TEST_ASSERT_TRUE_MESSAGE(min_out >= BRAKE_ON_MS2, msg);
}

// T12: Vollbremsung 8,0 m/s^2 als Rampe ueber 80 ms (8 Schritte) -> nicht
// SHOCK, wird durchgereicht.
void test_ramp_full_braking_8p0_is_dynamic() {
  MotionFilter filter;
  anchorLevel(filter);

  for (int i = 1; i <= 8; ++i) {
    filter.update({0.0f, 1.0f * i, G, 0.0f, DT});
  }
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MotionRegime::Dynamic), static_cast<int>(filter.regime()));
}

// T13: Ein-Sample-Sprung auf 30 m/s^2 -> SHOCK.
void test_single_sample_jump_to_30_is_shock() {
  MotionFilter filter;
  anchorLevel(filter);

  filter.update({0.0f, 30.0f, G, 0.0f, DT});
  TEST_ASSERT_EQUAL_INT(static_cast<int>(MotionRegime::Shock), static_cast<int>(filter.regime()));
}

// T14: Bias-Startkalibrierung (Stufe 1) AKTIV -- STATIC-Kalibrierfenster
// (200 kumulierte Samples, seit B-FW.11 entkoppelt von der kuerzeren
// Pitch-Verankerung, s. anchorLevel()) mit 0,5 deg/s Bias, danach 60 s
// DYNAMIC (identischer Aufbau wie T8). Da das synthetische Gyro-Signal
// rauschfrei konstant ist, mittelt Stufe 1 nahezu perfekt auf den wahren
// Bias -- realer Sensorrauschen wuerde einen kleinen Restfehler lassen
// (Schaetzung ~0,15 Grad laut Auftrag), hier bleibt die Grenze < 2 Grad
// daher mit deutlicher Reserve erfuellt.
void test_gyro_bias_calibration_keeps_drift_under_2deg() {
  MotionFilter filter;  // MOTION_GYRO_BIAS_ENABLED=true per Default
  anchorLevel(filter, /*gyro_bias_dps=*/0.5f);  // Verankerung + Stufe-1-Kalibrierfenster
  TEST_ASSERT_TRUE(filter.anchored());
  TEST_ASSERT_TRUE_MESSAGE(filter.biasCalibrated(), "Stufe 1 muss nach anchorLevel() abgeschlossen sein");

  const float bias_rad_s = 0.5f * (3.14159265f / 180.0f);
  for (int i = 0; i < 6000; ++i) {  // 60 s DYNAMIC, gleicher Aufbau wie T8
    filter.update({2.0f, 0.0f, G, bias_rad_s, DT});
  }

  const float pitch_deg = filter.pitch_rad() * (180.0f / 3.14159265f);
  char msg[96];
  snprintf(msg, sizeof(msg), "pitch=%.3f deg (erwartet < 2 deg dank Bias-Kalibrierung)", pitch_deg);
  TEST_ASSERT_TRUE_MESSAGE(fabsf(pitch_deg) < 2.0f, msg);
}

// T15: Kaltstart DIREKT in eine 4,0-m/s^2-Bremsung, ohne jede STATIC-
// Vorgeschichte -- die Verankerung kann nie ueber ein STATIC-Fenster
// abgeschlossen werden (Norm bleibt durchgehend ausserhalb des Bands) und
// greift stattdessen auf den MOTION_ANCHOR_TIMEOUT_S=2-s-Fallback zurueck
// (ebene Lage annehmen). Bis dahin bleibt der Ausgang sicherheitshalber 0;
// danach wird die Bremsung sofort korrekt erkannt (Nachweis fuer den in der
// vorigen Iteration gefundenen Kaltstart-Bug, s. stufe1_normgate.md).
void test_cold_start_directly_into_braking_holds_zero_until_timeout() {
  MotionFilter filter;

  for (int i = 0; i < 199; ++i) {  // bis knapp vor 2,0 s
    const float out = filter.update({0.0f, 4.0f, G, 0.0f, DT});
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out);
  }

  float out = 0.0f;
  for (int i = 0; i < 10; ++i) {  // t=2,0 .. 2,1 s: Timeout greift, Bremsung wird sichtbar
    out = filter.update({0.0f, 4.0f, G, 0.0f, DT});
  }
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5f, 4.0f, out, "Nach dem Timeout sollte die Bremsung sofort erkannt sein");
}

// T16: derselbe Verlauf wie T11 (Rampe auf 2,2 m/s^2 in 300 ms, 3 s halten),
// aber durch die VOLLE Kette motion_filter -> imu_health -> tail_light_fsm
// (statt nur den rohen Filterausgang zu pruefen). Das ist die Aussage, die
// spaeter in der Arbeit steht: das Bremslicht schaltet bei dieser Rampe
// ein und bleibt ueber die vollen 3 s oberhalb des Schlusslicht-
// Grundniveaus (TAILLIGHT_DUTY_PCT).
void test_ramp_to_2p2_full_chain_keeps_brake_light_on() {
  MotionFilter motion;
  ImuHealth health;
  TailLightFsm fsm;
  anchorLevel(motion);

  uint32_t t_ms = 0;
  for (int i = 1; i <= 30; ++i, t_ms += 10) {  // 300 ms Rampe
    const float ay = 2.2f * i / 30.0f;
    // Minimale Alternation an az verhindert IMU_FROZEN_LIMIT (ImuHealth
    // sieht sonst 30 bit-identische Zyklen als eingefrorenen Sensor an --
    // bei einem synthetisch konstanten Profil real passiert das nie,
    // s. main.cpp runExperiment()-Kommentar zum selben Kunstgriff).
    //
    // BEWUSSTES TESTARTEFAKT, keine reale Sensor-Simulation -- Amplitude
    // +-0,001 m/s^2 (Peak-Peak 0,002), alterniert jedes Sample. Sicherheits-
    // abstand gegen die Regime-Schwellen (STATIC-Baseline ax=ay~0):
    //   accel_norm-Schwankung +-0,001 m/s^2 -- Faktor 120 unter
    //     MOTION_NORM_STATIC_BAND=0,12.
    //   Jerk zwischen aufeinanderfolgenden Samples: |Delta_norm|/dt_s*0,01
    //     = 0,002/0,01*0,01 = 0,002 m/s^2/10ms -- Faktor 1000 unter
    //     MOTION_NORM_JERK_DELTA=2,0.
    // Beide Sicherheitsabstaende >> Faktor 10 -- kein Einfluss auf Regime
    // oder Ausgang messbar.
    //
    // Die Alternationsamplitude von +-0,001 m/s^2 liegt unterhalb der
    // Sensoraufloesung: Der MPU6050 liefert im +-16-g-Bereich 2048 LSB/g,
    // ein LSB entspricht 0,00479 m/s^2. Das Artefakt betraegt somit ca.
    // 0,21 LSB und ist auf realer Hardware nicht darstellbar -- es ist ein
    // rein rechnerischer Kunstgriff im Testpfad. Daraus folgt zugleich, dass
    // die Freeze-Erkennung in ImuHealth auf den skalierten Float-Werten
    // arbeitet und nicht auf den Rohregistern.
    //
    // Ein Fehlausloesen der Freeze-Erkennung im Feld ist dadurch nicht zu
    // erwarten: Das Rauschen des MPU6050 liegt bei rund 400 ug/sqrt(Hz), bei
    // 100 Hz Bandbreite also bei etwa 0,04 m/s^2 effektiv -- rund acht LSB.
    // Der Sensor dithert im Stillstand ausreichend, um nie als eingefroren
    // zu gelten.
    const float az = G + (((t_ms / 10u) % 2u == 0u) ? 0.001f : -0.001f);
    const float decel = motion.update({0.0f, ay, az, 0.0f, DT});
    const ImuHealthOutput health_out = health.update(true, 0.0f, ay, az, 0.0f, t_ms);
    const float tail_input =
        (health_out.plausible && health_out.escalation_trusted) ? decel : 0.0f;
    fsm.update(tail_input, SystemState::Run, t_ms);
  }

  bool ever_below_tail_pct = false;
  bool reached_brakelight = false;
  for (int i = 0; i < 300; ++i, t_ms += 10) {  // 3 s halten
    const float az = G + (((t_ms / 10u) % 2u == 0u) ? 0.001f : -0.001f);
    const float decel = motion.update({0.0f, 2.2f, az, 0.0f, DT});
    const ImuHealthOutput health_out = health.update(true, 0.0f, 2.2f, az, 0.0f, t_ms);
    const float tail_input =
        (health_out.plausible && health_out.escalation_trusted) ? decel : 0.0f;
    const TailLightOutput tl = fsm.update(tail_input, SystemState::Run, t_ms);
    if (tl.state == TailLightState::Brakelight) reached_brakelight = true;
    if (tl.duty_pct <= TAILLIGHT_DUTY_PCT) ever_below_tail_pct = true;
  }

  TEST_ASSERT_TRUE_MESSAGE(reached_brakelight, "Bremslicht sollte bei 2,2 m/s^2 einschalten");
  TEST_ASSERT_FALSE_MESSAGE(ever_below_tail_pct,
                             "Bremslicht-Duty sollte ueber die vollen 3 s oberhalb des Schlusslicht-Grundniveaus bleiben");
}

// T17 (B-FW.11 R6/Reparaturrunde): Verankerung uebersteht bis zu
// MOTION_ANCHOR_NON_STATIC_TOLERANCE-1 (=2) aufeinanderfolgende
// Nicht-STATIC-Ausreisser, ohne das Fenster abzubrechen -- realistisches
// Sensorrauschen (B-FW.11-Registerbefund) darf die Verankerung nicht bei
// jedem einzelnen Ausreisser neu starten lassen.
void test_anchor_tolerates_up_to_two_consecutive_non_static_samples() {
  MotionFilter filter;
  for (int i = 0; i < 15; ++i) {  // Haelfte der 30 fuer MOTION_ANCHOR_WINDOW_S=0,3s noetigen Samples
    filter.update({0.0f, 0.0f, G, 0.0f, DT});
  }
  TEST_ASSERT_FALSE_MESSAGE(filter.anchored(), "Nach nur 15 von 30 Samples darf noch nicht verankert sein");

  filter.update({0.0f, 3.0f, G, 0.0f, DT});  // 2 aufeinanderfolgende Ausreisser (toleriert)
  filter.update({0.0f, 3.0f, G, 0.0f, DT});
  TEST_ASSERT_FALSE_MESSAGE(filter.anchored(), "2 Ausreisser in Folge duerfen das Fenster noch nicht abbrechen");

  bool anchored_now = false;
  int steps_to_anchor = -1;
  for (int i = 0; i < 20 && !anchored_now; ++i) {
    filter.update({0.0f, 0.0f, G, 0.0f, DT});
    if (filter.anchored()) {
      anchored_now = true;
      steps_to_anchor = i + 1;
    }
  }
  TEST_ASSERT_TRUE_MESSAGE(anchored_now, "Sollte nach den restlichen STATIC-Samples verankert sein");
  char msg[64];
  snprintf(msg, sizeof(msg), "steps_to_anchor=%d (erwartet ~15, NICHT ~30)", steps_to_anchor);
  TEST_ASSERT_TRUE_MESSAGE(steps_to_anchor <= 16, msg);  // Fortsetzung, kein Neustart
}

// T18: Gegenprobe zu T17 -- DREI aufeinanderfolgende Ausreisser reissen das
// Fenster tatsaechlich ab (Toleranzschwelle MOTION_ANCHOR_NON_STATIC_TOLERANCE=3).
void test_anchor_resets_after_three_consecutive_non_static_samples() {
  MotionFilter filter;
  for (int i = 0; i < 15; ++i) {
    filter.update({0.0f, 0.0f, G, 0.0f, DT});
  }
  filter.update({0.0f, 3.0f, G, 0.0f, DT});
  filter.update({0.0f, 3.0f, G, 0.0f, DT});
  filter.update({0.0f, 3.0f, G, 0.0f, DT});  // 3. Ausreisser in Folge -> Abbruch
  TEST_ASSERT_FALSE(filter.anchored());

  int steps_to_anchor = -1;
  for (int i = 0; i < 40; ++i) {
    filter.update({0.0f, 0.0f, G, 0.0f, DT});
    if (filter.anchored() && steps_to_anchor < 0) steps_to_anchor = i + 1;
  }
  char msg[64];
  snprintf(msg, sizeof(msg), "steps_to_anchor=%d (erwartet ~30, kompletter Neustart)", steps_to_anchor);
  TEST_ASSERT_TRUE_MESSAGE(steps_to_anchor >= 29 && steps_to_anchor <= 31, msg);
}

// T19 (B-FW.11 R6): Bias-Kalibrierung sammelt kumulierte STATIC-Samples
// OHNE Zusammenhangsforderung -- muss ueber viele, wiederholt durch
// deutlich dynamische Bloecke unterbrochene STATIC-Phasen hinweg
// abschliessen, sobald insgesamt MOTION_BIAS_CALIB_SAMPLE_THRESHOLD (200)
// STATIC-Samples akkumuliert wurden (unabhaengig davon, wie oft die -- hier
// bewusst separat getestete -- Verankerung dabei selbst abreisst).
void test_bias_calibration_accumulates_across_non_contiguous_static_periods() {
  MotionFilter filter;
  const float gyro_rads = 0.5f * (3.14159265f / 180.0f);
  int static_seen = 0;
  bool bias_done = false;
  for (int block = 0; block < 40 && !bias_done; ++block) {
    for (int i = 0; i < 10 && !bias_done; ++i) {  // 10 STATIC-Samples
      filter.update({0.0f, 0.0f, G, gyro_rads, DT});
      ++static_seen;
      if (filter.biasCalibrated()) bias_done = true;
    }
    for (int i = 0; i < 5 && !bias_done; ++i) {  // 5 deutlich dynamische Samples (reissen die Verankerung ab)
      filter.update({0.0f, 5.0f, G, gyro_rads, DT});
    }
  }
  TEST_ASSERT_TRUE_MESSAGE(bias_done, "Bias-Kalibrierung sollte trotz wiederholter Unterbrechungen abschliessen");
  char msg[96];
  snprintf(msg, sizeof(msg), "static_seen=%d (erwartet 200..205)", static_seen);
  TEST_ASSERT_TRUE_MESSAGE(static_seen >= 200 && static_seen <= 205, msg);

  const float bias_deg = filter.gyroBiasRads() * (180.0f / 3.14159265f);
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.05f, 0.5f, bias_deg,
                                    "Bias sollte trotz Unterbrechungen nahe am wahren 0,5-deg/s-Wert liegen");
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_pure_tilt_yields_no_brake_signal);
  RUN_TEST(test_sustained_braking_survives_static_lead_in);
  RUN_TEST(test_tilt_then_braking_on_slope);
  RUN_TEST(test_shock_impulse_suppressed);
  RUN_TEST(test_shock_during_braking_does_not_mask_it);
  RUN_TEST(test_regime_ramp_to_braking_is_dynamic_not_shock);
  RUN_TEST(test_anchoring_at_tilted_cold_start_yields_no_spurious_signal);
  RUN_TEST(test_gyro_drift_bounded_without_bias_estimation);
  RUN_TEST(test_accelerating_in_non_braking_direction_yields_zero);
  RUN_TEST(test_dt_robustness);
  RUN_TEST(test_ramp_to_2p2_stays_above_brake_on_threshold);
  RUN_TEST(test_ramp_full_braking_8p0_is_dynamic);
  RUN_TEST(test_single_sample_jump_to_30_is_shock);
  RUN_TEST(test_gyro_bias_calibration_keeps_drift_under_2deg);
  RUN_TEST(test_cold_start_directly_into_braking_holds_zero_until_timeout);
  RUN_TEST(test_ramp_to_2p2_full_chain_keeps_brake_light_on);
  RUN_TEST(test_anchor_tolerates_up_to_two_consecutive_non_static_samples);
  RUN_TEST(test_anchor_resets_after_three_consecutive_non_static_samples);
  RUN_TEST(test_bias_calibration_accumulates_across_non_contiguous_static_periods);
  return UNITY_END();
}
