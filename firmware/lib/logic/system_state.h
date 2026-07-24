// system_state.h — Lebenszyklus-Zustand als reiner Eingabewert fuer R2/R3 (M1)
// R1 (Region 1, M2) wird diesen Wert liefern; R2 kennt nur den Wert, nicht
// die R1-Implementierung (Bible Kap. 6.6, FR-STA-01/02).
#pragma once

namespace logic {

enum class SystemState {
  Init,
  Run,
};

}  // namespace logic
