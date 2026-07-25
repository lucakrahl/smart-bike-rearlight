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
#include "led_output.h"       // drivers  (R2/R3 Aktorik, CON-02)
#include "lifecycle_fsm.h"    // logic    (R1)
#include "tail_light_fsm.h"   // logic    (R2)

// PWM-Kanal-Zuordnung erfolgt ueber ledcAttach() (Core v3.x, CON-02).
// TODO(FR-...): weitere Treiber-/Logikmodule einbinden, sobald angelegt:
//   #include "rf_input.h"       // drivers  (FR-RF-01)
//   #include "button_decoder.h" // logic    (FR-RF-03/04)
//   #include "sensors.h"        // drivers  (FR-SNS)
//   #include "telemetry.h"      // drivers  (FR-TEL)
//   #include "scheduler.h"      // optional Helfer

// ---- kooperativer Scheduler: einfache Task-Zeitstempel -------------------
static uint32_t t_imu = 0, t_baro = 0, t_gnss = 0, t_tele = 0;

// R1 (Lebenszyklus) und R2 (Ruecklicht) als reine Logik-Instanzen; werden von
// taskLifecycleAndTailLight() 100 Hz getickt (s. u.).
static logic::LifecycleFsm lifecycleFsm;
static logic::TailLightFsm tailLightFsm;

// ------------------------------------------------------------------------
// Task-Stubs — hier kommt die Logik der jeweiligen Region hinein.
// ------------------------------------------------------------------------
static void taskLifecycleAndTailLight() {
  // 100 Hz. Tickt R1 (Lebenszyklus) + R2 (Ruecklicht). Sicherheitskritisch,
  // Reaktionszeit <= 50 ms (NFR-RT-01).
  const uint32_t now = millis();

  const bool  critical_sensors_ready = false;  // TODO(M3): aus IMU-Init
  const float decel_ms2 = 0.0f;                // TODO(M3): aus IMU (Komplementaerfilter)

  const logic::LifecycleOutput sys = lifecycleFsm.update(critical_sensors_ready, now);
  const logic::TailLightOutput tl  = tailLightFsm.update(decel_ms2, sys.state, now);
  drivers::setDutyPercent(PIN_BRAKE_LIGHT, tl.duty_pct);

  // TODO(temp debug): entfernen, sobald Telemetrie (M5) den Zustand ausgibt.
  static uint32_t t_debug = 0;
  if (now - t_debug >= 1000) {
    t_debug = now;
    Serial.printf("[R1/R2] sys=%d degraded=%d tl=%d duty=%u%%\n",
                  (int)sys.state, (int)sys.degraded, (int)tl.state, tl.duty_pct);
  }
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
  // taskLifecycleAndTailLight() treibt die Duty ab dem ersten loop()-Durchlauf.
  drivers::attach(PIN_BRAKE_LIGHT);
  drivers::attach(PIN_BLINK_LEFT);
  drivers::attach(PIN_BLINK_RIGHT);

  // TODO(FR-CFG-03): Konfiguration aus NVS laden (Defaults bei leerem NVS).
  // TODO(FR-SAF-03): Task-Watchdog aktivieren (~2 s) — Hardware, gehoert hier
  // in setup()/loop(), nicht in lifecycle_fsm (s. TODO dort).
}

void loop() {
  const uint32_t now = millis();

  // Feste Reihenfolge, sicherheitsrelevantes zuerst (NFR-RT-03).
  if (now - t_imu  >= PERIOD_IMU_MS)  { t_imu  = now; taskLifecycleAndTailLight(); }
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
