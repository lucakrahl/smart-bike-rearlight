// led_output.cpp — Umsetzung PWM-Treiber. Hardwareabhaengig (Arduino.h).
#include "led_output.h"
#include <Arduino.h>
#include "config.h"

namespace drivers {

void attach(int pin) {
  ledcAttach(pin, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
}

void setDutyPercent(int pin, uint8_t duty_pct) {
  const uint8_t pct = duty_pct > 100 ? 100 : duty_pct;
  const uint32_t max_raw = (1u << PWM_RESOLUTION_BITS) - 1u;  // z. B. 255 bei 8 Bit
  const uint32_t raw = (max_raw * pct) / 100u;
  ledcWrite(pin, raw);
}

}  // namespace drivers
