// led_output.h — PWM-Ausgabe LED-Kanaele (CON-02: 5 kHz, ledcAttach/ledcWrite
// Core v3.x — ledcSetup()/ledcAttachPin() sind verboten, siehe CLAUDE.md).
// HARDWARE-TREIBER: kapselt nur die Arduino/Espressif-API, enthaelt keine
// Zustandslogik. Zustandsmaschinen (R2/R3) liegen in firmware/lib/logic und
// liefern hier nur das fertige Duty-Prozent an.
#pragma once
#include <cstdint>

namespace drivers {

// Initialisiert den PWM-Kanal am angegebenen Pin (Frequenz/Aufloesung aus
// config.h, CON-02). Muss vor setDutyPercent() fuer diesen Pin erfolgen.
void attach(int pin);

// duty_pct: 0..100. Werte > 100 werden gekappt (Fail-safe). Der Rohwert wird
// aus PWM_RESOLUTION_BITS abgeleitet, kein hartkodierter Skalenendwert.
void setDutyPercent(int pin, uint8_t duty_pct);

}  // namespace drivers
