// rf_input.h — 433-MHz-Funkempfang (RCSwitch, GPIO4, FR-RF-01)
// HARDWARE-TREIBER: kapselt nur die RCSwitch-API, enthaelt keine
// Entprellungs-/Tastenerkennungslogik (die liegt in
// firmware/lib/logic/button_decoder.h).
#pragma once
#include <cstdint>

namespace drivers {

struct RfSignal {
  bool     has_code;  // true: seit dem letzten rfRead() ist ein neuer Code eingetroffen
  uint32_t code;       // nur gueltig wenn has_code
};

// Aktiviert den 433-MHz-Empfang (RCSwitch) an PIN_RF_DATA (FR-RF-01).
void rfBegin();

// Liest den zuletzt empfangenen Code (falls vorhanden) und setzt den
// RCSwitch-Empfangspuffer zurueck. Kontinuierlich pro loop()-Durchlauf
// aufzurufen (kein fester Task-Takt, FR-RF-01).
RfSignal rfRead();

}  // namespace drivers
