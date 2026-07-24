// main.cpp — Einstieg & kooperativer Scheduler (Bible Kap. 6.8, NFR-RT-03)
//
// Dieses File verdrahtet Treiber (lib/drivers) und Logik (lib/logic) ueber
// einen nicht-blockierenden millis()-Scheduler. NOCH GERUEST: die Task-Rumpfe
// sind Stubs mit Verweis auf die umzusetzenden Anforderungen. Modul fuer Modul
// mit Claude Code fuellen (Reihenfolge s. docs/roadmap.md), Logik vor Hardware,
// jeweils mit Host-Unit-Test (pio test -e native).
//
// WICHTIG: kein delay() im Betrieb. Jeder Task-Schritt bleibt kurz (< 10 ms
// Worst-Case-Loop, NFR-RT-04).

#include <Arduino.h>
#include "pins.h"
#include "config.h"

// PWM-Kanal-Zuordnung erfolgt ueber ledcAttach() (Core v3.x, CON-02).
// TODO(FR-...): Treiber-/Logikmodule einbinden, sobald angelegt:
//   #include "led_output.h"     // drivers  (R2/R3 Aktorik)
//   #include "state_machine.h"  // logic    (R1..R3)
//   #include "rf_input.h"       // drivers  (FR-RF-01)
//   #include "button_decoder.h" // logic    (FR-RF-03/04)
//   #include "sensors.h"        // drivers  (FR-SNS)
//   #include "brake_curve.h"    // logic    (FR-TL-06)
//   #include "telemetry.h"      // drivers  (FR-TEL)
//   #include "scheduler.h"      // optional Helfer

// ---- kooperativer Scheduler: einfache Task-Zeitstempel -------------------
static uint32_t t_imu = 0, t_baro = 0, t_gnss = 0, t_tele = 0;

// ------------------------------------------------------------------------
// Task-Stubs — hier kommt die Logik der jeweiligen Region hinein.
// ------------------------------------------------------------------------
static void taskImuAndBrakeLight() {
  // 100 Hz. TODO(FR-SNS-02, FR-TL-05/06): IMU lesen, Komplementaerfilter,
  // Verzoegerung -> Bremskennlinie -> rote LED (R2). Sicherheitskritisch,
  // Reaktionszeit <= 50 ms (NFR-RT-01).
}

static void taskBaro() {
  // 10 Hz. TODO(FR-SNS-02): BMP280 lesen (optional, FR-STA-05).
}

static void taskGnss() {
  // 1 Hz. TODO(FR-SNS-02, FR-TEL-05): NMEA parsen, Fix-Status bestimmen.
}

static void taskBlinker() {
  // TODO(FR-BLK-01..09): Blinker-State-Machine (R3), Blinktakt 1,5 Hz.
}

static void taskRf() {
  // TODO(FR-RF-01..06): RF-Codes lesen, Tastenerkennung (kurz/lang),
  // Events an die Blinker-State-Machine.
}

static void taskTelemetry() {
  // 10 Hz. TODO(FR-TEL-02..06): Frame (versioniert) bauen, per BLE senden
  // oder in den RAM-Ringpuffer schreiben (NFR-RES-01).
}

static void taskConfigConsole() {
  // TODO(FR-CFG-02): nicht-blockierendes serielles Kalibrier-Interface
  // (get/set/list/reset) ueber Serial (UART0).
}

// ------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println(F("[boot] Smart Bike Rear Light — Firmware-Geruest"));

  // R1 INIT: rote LED als Diagnose-Anzeige (FR-TL-03), bis Sensoren bereit.
  ledcAttach(PIN_BRAKE_LIGHT, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
  ledcAttach(PIN_BLINK_LEFT,  PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
  ledcAttach(PIN_BLINK_RIGHT, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);

  // TODO(FR-STA-01): Sensor-Init mit Timeout 5 s -> RUN oder degradierter RUN.
  // TODO(FR-CFG-03): Konfiguration aus NVS laden (Defaults bei leerem NVS).
  // TODO(FR-SAF-03): Task-Watchdog aktivieren (~2 s).

  // Platzhalter bis Module stehen: rotes Schlusslicht auf Grundhelligkeit,
  // damit ein sicheres Rueckicht vorhanden ist (FR-SAF-01).
  const uint8_t taillight_duty = (uint8_t)(255u * TAILLIGHT_DUTY_PCT / 100u);
  ledcWrite(PIN_BRAKE_LIGHT, taillight_duty);
}

void loop() {
  const uint32_t now = millis();

  // Feste Reihenfolge, sicherheitsrelevantes zuerst (NFR-RT-03).
  if (now - t_imu  >= PERIOD_IMU_MS)  { t_imu  = now; taskImuAndBrakeLight(); }
  taskRf();
  taskBlinker();
  if (now - t_baro >= PERIOD_BARO_MS) { t_baro = now; taskBaro(); }
  if (now - t_gnss >= PERIOD_GNSS_MS) { t_gnss = now; taskGnss(); }
  if (now - t_tele >= PERIOD_TELE_MS) { t_tele = now; taskTelemetry(); }
  taskConfigConsole();

  // TODO(FR-SAF-03): Watchdog fuettern.
  // Optionaler CPU-Idle (NFR-PWR-01); haelt 100 Hz problemlos.
  delay(1);
}
