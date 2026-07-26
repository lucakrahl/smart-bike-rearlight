// rf_input.cpp — Umsetzung RF-Empfangstreiber. Hardwareabhaengig (RCSwitch).
#include "rf_input.h"
#include <RCSwitch.h>
#include "pins.h"

namespace drivers {

namespace {
RCSwitch rcSwitch;
}  // namespace

void rfBegin() {
  rcSwitch.enableReceive(PIN_RF_DATA);
}

RfSignal rfRead() {
  if (!rcSwitch.available()) {
    return RfSignal{false, 0};
  }
  const uint32_t code = rcSwitch.getReceivedValue();
  rcSwitch.resetAvailable();
  return RfSignal{true, code};
}

}  // namespace drivers
