// brake_curve.h — Bremslicht-Kennlinie (Proportionalteil, FR-TL-06)
// REINE LOGIK, hardwarefrei (NFR-TST-01): kein #include <Arduino.h>.
// Host-testbar (siehe firmware/test/test_logic/test_brake_curve.cpp).
#pragma once

namespace logic {

struct BrakeCurveParams {
  float on_ms2   = 2.0f;   // Einschaltschwelle (FR-TL-06)
  float full_ms2 = 5.0f;   // Saettigung -> 100 %
  int   tail_pct = 20;     // Schlusslicht-Grundhelligkeit (FR-TL-04)
};

// Bildet den Betrag der Verzoegerung [m/s^2] auf die Duty der roten LED [%] ab.
//   decel < on_ms2      -> tail_pct (reines Schlusslicht)
//   on_ms2..full_ms2    -> linear von tail_pct auf 100 %
//   decel >= full_ms2   -> 100 %
// Hysterese/Mindesthaltezeit und Notbrems-Blinken (FR-TL-07) liegen in der
// State-Machine, nicht hier (dieser Teil ist bewusst zustandslos & rein).
int brakeDutyPercent(float decel_ms2, const BrakeCurveParams& p);

}  // namespace logic
