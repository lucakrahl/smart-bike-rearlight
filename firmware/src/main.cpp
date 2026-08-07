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
#include <Wire.h>
#include <esp_task_wdt.h>     // FR-SAF-03 (Task-Watchdog)
#include <esp_system.h>       // FR-SAF-03 (esp_reset_reason())
#include "pins.h"
#include "config.h"
#include "led_output.h"       // drivers  (R2/R3 Aktorik, CON-02)
#include "imu_driver.h"       // drivers  (R4 IMU, FR-SNS-01/02/03)
#include "bmp280_driver.h"    // drivers  (R4 Baro, FR-SNS-01/02/03)
#include "gnss_driver.h"      // drivers  (R4 GNSS/L86, FR-SNS-01/02)
#include "rf_input.h"         // drivers  (RF-Empfang, FR-RF-01)
#include "ble_telemetry.h"    // drivers  (R4 BLE-Transport, FR-TEL-01, FR-SYS-04)
#include "lifecycle_fsm.h"    // logic    (R1)
#include "tail_light_fsm.h"   // logic    (R2)
#include "motion_filter.h"    // logic    (R4->R2, Komplementaerfilter)
#include "imu_health.h"       // logic    (R4 IMU-Gesundheit, FR-SNS-04/05, FR-STA-04)
#include "button_decoder.h"   // logic    (RF-Tastenerkennung, FR-RF-02/03/04)
#include "blinker_fsm.h"      // logic    (R3, FR-BLK-01..09)
#include "gnss_fix.h"         // logic    (R4 GNSS-Fix-Bewertung, FR-TEL-05)
#include "telemetry_frame.h"  // logic    (R4 Telemetrie-Frame, FR-TEL-02/03/06)
#include "telemetry_buffer.h" // logic    (R4 RAM-Ringpuffer, FR-TEL-04)
#include "telemetry_window_agg.h"  // logic (v3 Fensteraggregation, docs/BLE_Frame_v3_Schnittstelle.md)
#include "gnss_speed_ref.h"        // logic (v3 GNSS-Referenz, E1, rein beobachtend, E5)

// PWM-Kanal-Zuordnung erfolgt ueber ledcAttach() (Core v3.x, CON-02).

// ---- kooperativer Scheduler: einfache Task-Zeitstempel -------------------
static uint32_t t_imu = 0, t_baro = 0, t_gnss = 0, t_tele = 0;

// R1 (Lebenszyklus) und R2 (Ruecklicht) als reine Logik-Instanzen; werden von
// taskLifecycleAndTailLight() 100 Hz getickt (s. u.).
static logic::LifecycleFsm lifecycleFsm;
static logic::TailLightFsm tailLightFsm;
static logic::MotionFilter motionFilter;
static uint32_t t_motion_prev_ms = 0;  // fuer dt_s der Komplementaerfilter-Eingabe

// IMU-Gesundheit (FR-SNS-04/05, FR-STA-04): getrennt von R1/lifecycleFsm
// gefuehrt, da dessen degraded-Flag laut lifecycle_fsm.h bewusst nur das
// INIT-Ergebnis abbildet (FR-STA-06: kein Ruecksprung nach RUN). Ein
// Laufzeit-Sensorausfall gated hier ausschliesslich R2 (s. u.).
static logic::ImuHealth imuHealth;
static bool imu_was_healthy_last_cycle_ = false;  // fuer motionFilter-Reset beim Uebergang

// Letzter IMU-Gesundheitsstatus, vorgehalten fuer die spaetere Telemetrie
// (FR-TEL-03), die Sensor-Fehler als eigenes Flag an die App melden soll.
static logic::ImuHealthOutput lastImuHealth{};

// Letzter von taskLifecycleAndTailLight() ermittelter Lebenszyklus-Zustand.
// Vorbelegt mit Init: Tastendruecke werden verworfen, bis R1 erstmals RUN
// liefert (FR-BLK-09-Gating, s. taskBlinker()).
static logic::SystemState lastSystemState = logic::SystemState::Init;

// RF-Tastenerkennung (Sub-FSM vor R3) + Blinker-FSM (R3).
static logic::ButtonDecoder buttonDecoder;
static logic::BlinkerFsm    blinkerFsm;

// Von taskRf() gesetztes, von taskBlinker() im selben loop()-Durchlauf genau
// einmal konsumiertes Ereignis (taskRf laeuft vor taskBlinker, s. loop()).
static logic::ButtonEvent pendingButtonEvent = logic::ButtonEvent::NONE;

// FR-SAF-03: true, wenn dieser Boot durch einen Watchdog-/Panic-Reset
// ausgeloest wurde (in setup() einmalig gesetzt). Vorgehalten fuer die
// spaetere Telemetrie (FR-TEL-03) -- ein WDT-Reset im Fahrbetrieb bedeutet
// einen kurzen Neustart (Init-Blink -> Schlusslicht), akzeptierte
// Fail-Safe-Ausnahme (FR-SAF-01).
static bool bootRecoveredFromWatchdog = false;

// ---- Telemetrie (M5 Teil C2, FR-TEL-01..04) -------------------------------
// Letzte Sensor-/Statuswerte, file-scope vorgehalten (Muster wie
// lastImuHealth oben) -- taskTelemetry() liest hieraus, statt Sensoren
// erneut/asynchron abzufragen. accel/gyro werden nur bei read_ok
// aktualisiert (letzte tatsaechliche Messung, keine Nullen bei einem
// transienten I2C-Blip, s. taskLifecycleAndTailLight()); brake_decel_ms2
// dagegen bewusst jeden Zyklus (roher motion_filter-Wert inkl. der durch
// !healthy_this_cycle erzwungenen 0, s. telemetry_frame.h Feld-Kommentar).
static drivers::ImuSample lastImuSample{};
static float lastDecelMs2 = 0.0f;
static bool lastInitDegraded = false;
static drivers::BaroSample lastBaroSample{};
static bool lastBaroValid = false;
static drivers::GnssData lastGnssData{};
static logic::GnssFixStatus lastGnssFix = logic::GnssFixStatus::NO_DATA;
// Tatsaechlich kommandierte Ruecklicht-Duty (derselbe Wert wie an
// drivers::setDutyPercent() uebergeben) -- im Gegensatz zu lastDecelMs2
// bereits durch tail_light_fsm gegatet, s. telemetry_frame.h Feld-Kommentar
// zu brake_light_pct.
static uint8_t lastBrakeLightPct = 0;

// RAM-Ringpuffer (FR-TEL-04, NFR-RES-01) fuer Frames ohne BLE-Verbindung
// bzw. als Reconnect-Backfill-Quelle (s. taskTelemetry()).
static logic::TelemetryBuffer telemetryBuffer;

// ---- v3-Erweiterung (docs/BLE_Frame_v3_Schnittstelle.md) ------------------
// Fensteraggregation der 100-Hz-Groessen zwischen zwei Telemetrie-Frames
// (Kap. 3.2/3.3) -- gefuettert in taskLifecycleAndTailLight(), ausgelesen +
// zurueckgesetzt in taskTelemetry().
static logic::TelemetryWindowAgg telemetryWindowAgg;

// Dauer des VORHERIGEN loop()-Durchlaufs in us (micros(), NFR-RT-04, s.
// loop()). Bewusst ein Zyklus "alt": der aktuelle Durchlauf ist zum
// Zeitpunkt des add()-Aufrufs in taskLifecycleAndTailLight() (fruehzeitig
// in loop(), s. u.) noch nicht abgeschlossen. Bei einem 100-Hz-Fenster mit
// ~10 Samples ist dieser Ein-Zyklus-Versatz fuer ein Maximum irrelevant.
static uint32_t lastLoopDurationUs = 0;

// GNSS-Referenzbeschleunigung (E1, BEOBACHTEND, E5): wirkt an keiner Stelle
// auf motion_filter/tail_light_fsm zurueck, s. gnss_speed_ref.h. Nur fuer
// die v3-Telemetrie vorgehalten.
static logic::GnssSpeedRef gnssSpeedRef;
static logic::GnssSpeedRefOutput lastGnssSpeedRef{};
static uint32_t t_gnss_speed_ref_prev_ms = 0;  // fuer real gemessenes dt_s zwischen Fixes

// ------------------------------------------------------------------------
// Task-Stubs — hier kommt die Logik der jeweiligen Region hinein.
// ------------------------------------------------------------------------
static void taskLifecycleAndTailLight() {
  // 100 Hz. Tickt R1 (Lebenszyklus) + R2 (Ruecklicht). Sicherheitskritisch,
  // Reaktionszeit <= 50 ms (NFR-RT-01).
  const uint32_t now = millis();

  drivers::ImuSample raw{};
  const bool read_ok = drivers::imuRead(raw);
  const logic::ImuHealthOutput health = imuHealth.update(
      read_ok, raw.accel_x_ms2, raw.accel_y_ms2, raw.accel_z_ms2, raw.gyro_x_rads, now);
  lastImuHealth = health;  // fuer spaetere Telemetrie (FR-TEL-03/FR-STA-04)
  if (read_ok) {
    lastImuSample = raw;  // letzte tatsaechliche Messung, s. Kommentar oben
  }

  // Zwei bewusst getrennte Gates, auch wenn sie sich aktuell pro Zyklus
  // decken: motionFilter wird an der reinen Zyklus-Gesundheit (read_ok &&
  // plausible) gespeist, R2 weiter unten dagegen am state-basierten
  // degraded-Flag -- so bleibt ein spaeterer Debounce-/Hysterese-Umbau von
  // "degraded" ohne Wirkung auf die Filter-Fuetterung.
  const bool healthy_this_cycle = read_ok && health.plausible;

  float decel_ms2 = 0.0f;
  if (healthy_this_cycle) {
    if (!imu_was_healthy_last_cycle_) {
      // Nach einer Ausfall-/Recovery-Phase: alten Neigungswinkel verwerfen
      // und Zeitbasis neu setzen, sonst wuerde ein grosser dt_s-Sprung bzw.
      // ein waehrenddessen veralteter Winkel eine falsche Brems-
      // Beschleunigung vortaeuschen (s. motion_filter::reset()).
      motionFilter.reset();
      t_motion_prev_ms = now;
    }
    float dt_s = (now - t_motion_prev_ms) / 1000.0f;
    t_motion_prev_ms = now;
    // Defensive Begrenzung (1..50 ms): ein Ausreisser nach einer Blockade
    // (z. B. I2C-Recovery-Zyklus) soll die Gyro-Integration nicht
    // "sprengen"; dt_s ist der real gemessene Abtastabstand, keine
    // Konstante -- ein einzelner grosser Sprung waere sonst ungebremst.
    if (dt_s < 0.001f) dt_s = 0.001f;
    if (dt_s > 0.050f) dt_s = 0.050f;
    decel_ms2 = motionFilter.update(
        {raw.accel_x_ms2, raw.accel_y_ms2, raw.accel_z_ms2, raw.gyro_x_rads, dt_s});

    // v3-Fensteraggregation (Vertrag Kap. 3.2/3.3): norm_delta ist hier
    // bewusst ‖a‖-g (accel_norm()-g), NICHT motionFilter.normDelta() --
    // Letzteres liefert den Jerk (Namenskollision, s. Hinweis in
    // telemetry_window_agg.h). Nur bei healthy_this_cycle gefuettert, sonst
    // waeren accel_norm()/regime() nicht von diesem Zyklus.
    constexpr float kGravityMs2 = 9.80665f;
    telemetryWindowAgg.add(dt_s, motionFilter.accel_norm() - kGravityMs2,
                            motionFilter.normDelta(), motionFilter.regime(), lastLoopDurationUs);
  }
  imu_was_healthy_last_cycle_ = healthy_this_cycle;
  lastDecelMs2 = decel_ms2;  // fuer Telemetrie: roher Filterwert, s. Feld-Kommentar telemetry_frame.h

  if (health.request_recovery) {
    drivers::imuRecoverStage(health.recovery_stage);
  }

  const bool critical_sensors_ready = drivers::imuIsReady();
  const logic::LifecycleOutput sys = lifecycleFsm.update(critical_sensors_ready, now);
  lastInitDegraded = sys.degraded;  // fuer Telemetrie (FR-TEL-03)

  // FR-STA-04 Fail-Safe: auf JEDEM ungesunden Zyklus explizit 0 -- nicht
  // erst ab "degraded" (das kippt erst nach IMU_FAIL_LIMIT Zyklen). Schon
  // waehrend dieser Zaehlphase soll tail_light_fsm keinen frischen Wert
  // bekommen. Zusaetzlich: escalation_trusted verlangt IMU_ESCALATION_
  // CONFIRM_CYCLES aufeinanderfolgende plausible Zyklen, bevor ein
  // Bremswert ueberhaupt durchgereicht wird -- ein einzelnes Muell-aber-
  // plausibles Sample (Fehlerinjektionstest SDA-Kurzschluss) darf sonst
  // sofort eine Bremseskalation ausloesen. decel_ms2 ist strukturell
  // bereits 0, wenn !healthy_this_cycle (frische lokale Variable, nur im
  // healthy_this_cycle-Zweig oben befuellt) -- hier trotzdem explizit an
  // beide Bedingungen gebunden, damit die Zusicherung im Code sichtbar ist.
  // tail_light_fsm faellt dadurch ueber seine bestehende Hysterese/
  // Mindesthaltezeit von selbst sicher auf Dauer-Schlusslicht zurueck --
  // R2 selbst bleibt unveraendert, muss vom IMU-Zustand nichts wissen.
  const float tail_input =
      (healthy_this_cycle && health.escalation_trusted) ? decel_ms2 : 0.0f;
  const logic::TailLightOutput tl = tailLightFsm.update(tail_input, sys.state, now);
  drivers::setDutyPercent(PIN_BRAKE_LIGHT, tl.duty_pct);
  lastBrakeLightPct = tl.duty_pct;  // fuer Telemetrie, s. Kommentar bei der Deklaration
  lastSystemState = sys.state;  // fuer FR-BLK-09-Gating in taskBlinker()

  // TODO(temp debug): entfernen, sobald Telemetrie (M5) den Zustand ausgibt.
  static uint32_t t_debug = 0;
  if (now - t_debug >= 1000) {
    t_debug = now;
    Serial.printf("[R1/R2] sys=%d init_degraded=%d tl=%d duty=%u%% imu_health=%d imu_plausible=%d\n",
                  (int)sys.state, (int)sys.degraded, (int)tl.state, tl.duty_pct,
                  (int)health.state, (int)health.plausible);
  }
}

static void taskBaro() {
  // Task-Slot 10 Hz (s. loop()), BMP280-FORCED-Zyklus laeuft bewusst
  // langsamer (BARO_FORCED_CYCLE_MS, ~1 Hz): Trigger und Read sind auf zwei
  // aufeinanderfolgende Zyklen entkoppelt, dazwischen schlaeft der Sensor
  // (Selbsterwaermung minimiert). Kein blockierendes Warten: bmp280Read()
  // liest immer eine laengst abgeschlossene Messung (Konversionszeit ~6 ms
  // << BARO_FORCED_CYCLE_MS, s. bmp280_driver.cpp).
  static uint32_t t_cycle = 0;
  static bool measurement_pending = false;

  const uint32_t now = millis();
  if (now - t_cycle < BARO_FORCED_CYCLE_MS) {
    return;
  }
  t_cycle = now;

  if (!drivers::bmp280IsReady()) {
    lastBaroValid = false;  // optionaler Sensor (FR-STA-05), fuer Telemetrie
    return;                 // kein Effekt auf Licht/Blinker
  }

  if (measurement_pending) {
    lastBaroSample = drivers::bmp280Read();
    lastBaroValid = true;
    // TODO(temp debug): entfernen, sobald Telemetrie (M5 Teil B) den Wert
    // ausgibt. Hinter DEBUG_SERIAL abschaltbar.
    if (DEBUG_SERIAL) {
      Serial.printf("[Baro] pressure=%.1f Pa temp=%.2f C\n", lastBaroSample.pressure_pa,
                    lastBaroSample.temperature_c);
    }
  }
  drivers::bmp280TriggerMeasurement();
  measurement_pending = true;
}

static void taskGnss() {
  // 1 Hz (PERIOD_GNSS_MS, s. loop()). NMEA wird nicht hier, sondern jeden
  // loop()-Durchlauf per gnssPump() geparst (s. loop()) — hier wird nur
  // der aktuelle Stand gelesen und bewertet. Optionaler Sensor (FR-STA-05):
  // kein Fix/keine Daten beeinflussen Licht/Blinker nicht.
  lastGnssData = drivers::gnssRead();
  lastGnssFix = logic::gnssFixStatus(lastGnssData.location_valid, lastGnssData.location_age_ms,
                                      lastGnssData.sats, lastGnssData.chars_processed);

  // v3 (E1, BEOBACHTEND/E5): grobe Referenzbeschleunigung aus aufeinander-
  // folgenden 1-Hz-Fixes -- wirkt an keiner Stelle auf motion_filter/
  // tail_light_fsm zurueck (s. gnss_speed_ref.h). dt_s ist die real
  // gemessene Zeit seit dem letzten taskGnss()-Aufruf, nicht PERIOD_GNSS_MS
  // (gleiches Muster wie motionFilter, s. taskLifecycleAndTailLight()); der
  // erste Aufruf liefert einen unplausibel grossen dt_s, ist aber
  // wirkungslos, da has_prev_ dort ohnehin noch false ist.
  const uint32_t now_gnss = millis();
  const float gnss_dt_s = (now_gnss - t_gnss_speed_ref_prev_ms) / 1000.0f;
  t_gnss_speed_ref_prev_ms = now_gnss;
  lastGnssSpeedRef = gnssSpeedRef.update({lastGnssData.speed_kmph / 3.6f, lastGnssData.sats,
                                          lastGnssData.hdop, lastGnssData.location_valid, gnss_dt_s});

  // TODO(temp debug): entfernen, sobald Telemetrie (M5 Teil B) den Wert
  // ausgibt. Hinter DEBUG_SERIAL abschaltbar.
  if (DEBUG_SERIAL) {
    Serial.printf("[GNSS] lat=%.6f lon=%.6f sats=%u hdop=%.2f status=%d\n", lastGnssData.lat,
                  lastGnssData.lon, lastGnssData.sats, lastGnssData.hdop, (int)lastGnssFix);
  }
}

static void taskBlinker() {
  // Kein fester Task-Takt: laeuft jeden loop()-Durchlauf, direkt nach taskRf()
  // (s. loop()), damit ein erkanntes Ereignis genau einmal verarbeitet wird.
  const uint32_t now = millis();

  // FR-BLK-09: RF-Blinkerbefehle erst ab RUN wirksam, waehrend INIT verworfen.
  const logic::ButtonEvent event =
      (lastSystemState == logic::SystemState::Run) ? pendingButtonEvent
                                                      : logic::ButtonEvent::NONE;
  pendingButtonEvent = logic::ButtonEvent::NONE;  // genau einmal konsumiert

  const logic::BlinkerOutput out = blinkerFsm.update(event, now);
  drivers::setDutyPercent(PIN_BLINK_LEFT,  out.left_on  ? 100 : 0);
  drivers::setDutyPercent(PIN_BLINK_RIGHT, out.right_on ? 100 : 0);
}

static void taskRf() {
  // Kein fester Task-Takt: laeuft jeden loop()-Durchlauf (FR-RF-01,
  // kontinuierliche Ueberwachung). Laeuft vor taskBlinker() (s. loop()).
  const uint32_t now = millis();
  const drivers::RfSignal rf = drivers::rfRead();
  pendingButtonEvent = buttonDecoder.update(rf.has_code, rf.code, now);
}

static void taskTelemetry() {
  // 10 Hz (FR-TEL-02). Frame aus den zuletzt gesampelten Werten bauen und je
  // nach BLE-Zustand direkt senden oder puffern (FR-TEL-01/04).
  const uint32_t now = millis();

  logic::TelemetryFrame frame{};
  frame.timestamp_ms = now;

  frame.accel_x_ms2 = lastImuSample.accel_x_ms2;
  frame.accel_y_ms2 = lastImuSample.accel_y_ms2;
  frame.accel_z_ms2 = lastImuSample.accel_z_ms2;
  frame.gyro_x_rads = lastImuSample.gyro_x_rads;
  frame.gyro_y_rads = lastImuSample.gyro_y_rads;
  frame.gyro_z_rads = lastImuSample.gyro_z_rads;
  frame.brake_decel_ms2 = lastDecelMs2;

  frame.pressure_pa = lastBaroSample.pressure_pa;
  frame.temperature_c = lastBaroSample.temperature_c;

  frame.lat = lastGnssData.lat;
  frame.lon = lastGnssData.lon;
  frame.speed_kmph = lastGnssData.speed_kmph;
  frame.course_deg = lastGnssData.course_deg;
  frame.altitude_m = lastGnssData.altitude_m;
  frame.sats = lastGnssData.sats;
  frame.hdop = lastGnssData.hdop;
  frame.utc_year = lastGnssData.year;
  frame.utc_month = lastGnssData.month;
  frame.utc_day = lastGnssData.day;
  frame.utc_hour = lastGnssData.hour;
  frame.utc_minute = lastGnssData.minute;
  frame.utc_second = lastGnssData.second;

  frame.system_state = static_cast<uint8_t>(lastSystemState);
  frame.init_degraded = lastInitDegraded;
  frame.imu_health_state = static_cast<uint8_t>(lastImuHealth.state);
  frame.baro_valid = lastBaroValid;
  frame.gnss_fix_status = static_cast<uint8_t>(lastGnssFix);
  frame.watchdog_recovered = bootRecoveredFromWatchdog;
  frame.brake_light_pct = lastBrakeLightPct;

  // v3 (docs/BLE_Frame_v3_Schnittstelle.md Kap. 3.2). gnss_accel_ms2 ist bei
  // !valid IMMER 0.0f (nie NaN, s. Feld-Kommentar telemetry_frame.h).
  frame.gnss_accel_ms2 = lastGnssSpeedRef.valid ? lastGnssSpeedRef.accel_ms2 : 0.0f;
  frame.gnss_accel_valid = lastGnssSpeedRef.valid;
  frame.pitch_rad = motionFilter.pitch_rad();
  frame.gyro_bias_rads = motionFilter.gyroBiasRads();
  frame.bias_calibrated = motionFilter.biasCalibrated();

  const logic::WindowAggSnapshot windowSnap = telemetryWindowAgg.snapshotAndReset();
  frame.norm_delta_min = windowSnap.norm_delta_min;
  frame.norm_delta_max = windowSnap.norm_delta_max;
  frame.jerk_max = windowSnap.jerk_max;
  frame.regime_static_n = windowSnap.regime_static_n;
  frame.regime_dynamic_n = windowSnap.regime_dynamic_n;
  frame.regime_shock_n = windowSnap.regime_shock_n;
  frame.dt_max_ms = windowSnap.dt_max_ms;
  frame.loop_max_us = windowSnap.loop_max_us;


  uint8_t bytes[logic::TELEMETRY_FRAME_SIZE];
  logic::telemetryFrameSerialize(frame, bytes);

  if (!drivers::bleIsConnected()) {
    telemetryBuffer.push(bytes);  // FR-TEL-04: ohne Verbindung puffern
    return;
  }

  if (telemetryBuffer.empty()) {
    drivers::bleNotify(bytes, logic::TELEMETRY_FRAME_SIZE);  // Live-Fall: direkt senden
    return;
  }

  // Reconnect-Backfill: frisches Frame hinten anstellen (haelt die FIFO-
  // Chronologie strikt, kein zweiter Strom noetig), dann bis zu
  // BLE_BACKFILL_FRAMES_PER_TICK aelteste Frames nachliefern -- das holt
  // 60 s Backlog binnen weniger Sekunden auf, ohne die Live-Rate zu stoeren
  // (dieser Block laeuft weiterhin nur einmal pro 100-ms-Tick).
  telemetryBuffer.push(bytes);
  uint8_t backlog[logic::TELEMETRY_FRAME_SIZE];
  for (uint8_t i = 0; i < BLE_BACKFILL_FRAMES_PER_TICK; ++i) {
    if (telemetryBuffer.empty()) {
      break;
    }
    telemetryBuffer.pop(backlog);
    if (!drivers::bleNotify(backlog, logic::TELEMETRY_FRAME_SIZE)) {
      // Sende-Queue des BLE-Stacks voll (selten): Frame nicht verlieren,
      // aber auch nicht auf einen Retry innerhalb dieses Ticks warten
      // (NFR-RT-04). Wird ans Pufferende zurueckgelegt -- unter dieser
      // seltenen Kongestion nicht mehr streng chronologisch, dafuer ohne
      // eigene peek()-Erweiterung der bereits getesteten TelemetryBuffer-API.
      telemetryBuffer.push(backlog);
      break;
    }
  }
}

static void taskConfigConsole() {
  // TODO(temp debug): Watchdog-Verifikationshook -- 'H' ueber den Serial-
  // Monitor sendet das Board absichtlich in einen Hang; der Task-Watchdog
  // (FR-SAF-03) muss es binnen WATCHDOG_TIMEOUT_MS selbst neustarten. Nur
  // hinter DEBUG_SERIAL, vor Auslieferung entfernen.
  if (DEBUG_SERIAL && Serial.available() && Serial.read() == 'H') {
    Serial.println(F("[debug] simulating hang for WDT test"));
    while (true) {}
  }

  // TODO(FR-CFG-02): nicht-blockierendes serielles Kalibrier-Interface
  // (get/set/list/reset) ueber Serial (UART0).
}

// ------------------------------------------------------------------------
#ifndef BENCH_MODE
void setup() {
  Serial.begin(115200);
  Serial.println(F("[boot] Smart Bike Rear Light — Firmware-Geruest"));

  // FR-SAF-03 Diagnose: moeglichst frueh in setup(), damit der Log-Eintrag
  // garantiert den tatsaechlichen Boot-Grund widerspiegelt.
  const esp_reset_reason_t reset_reason = esp_reset_reason();
  bootRecoveredFromWatchdog = reset_reason == ESP_RST_TASK_WDT || reset_reason == ESP_RST_INT_WDT ||
                              reset_reason == ESP_RST_WDT || reset_reason == ESP_RST_PANIC;
  if (DEBUG_SERIAL && bootRecoveredFromWatchdog) {
    Serial.println(F("[boot] recovered from watchdog reset"));
  }

  // I2C-Bus zentral EINMALIG initialisieren (FR-SNS-03: Timeout an einer
  // Stelle). imuBegin()/bmp280Begin() sind reine Busnutzer und rufen selbst
  // kein Wire.begin() auf.
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setTimeOut(I2C_TIMEOUT_MS);

  // R1 INIT: rote LED als Diagnose-Anzeige (FR-TL-03), bis Sensoren bereit.
  // taskLifecycleAndTailLight() treibt die Duty ab dem ersten loop()-Durchlauf.
  drivers::attach(PIN_BRAKE_LIGHT);
  drivers::attach(PIN_BLINK_LEFT);
  drivers::attach(PIN_BLINK_RIGHT);

  // R1-Eingang "kritische Sensoren bereit" (FR-STA-01): IMU-Init. Ergebnis
  // wird pro Tick per imuIsReady() an lifecycleFsm.update() gereicht
  // (s. taskLifecycleAndTailLight()).
  const bool imu_ready = drivers::imuBegin();
  Serial.printf("[boot] IMU ready=%d\n", (int)imu_ready);
  t_motion_prev_ms = millis();

  // Optionaler Sensor (FR-STA-05): Ausfall beeinflusst Licht/Blinker nicht.
  const bool baro_ready = drivers::bmp280Begin();
  Serial.printf("[boot] BMP280 ready=%d\n", (int)baro_ready);

  drivers::rfBegin();  // FR-RF-01: kontinuierlicher Empfang an PIN_RF_DATA

  // Optionaler Sensor (FR-STA-05): Ausfall/kein Fix beeinflusst Licht/Blinker
  // nicht. UART2, 9600 Bd (Bible Kap. 4.1); TinyGPSPlus parst in gnssPump().
  const bool gnss_ready = drivers::gnssBegin();
  Serial.printf("[boot] GNSS ready=%d\n", (int)gnss_ready);

  // FR-TEL-01/FR-SYS-04: BLE-Telemetrie-Server (Notify-only). Ausfall/
  // Trennung beeinflusst Licht/Blinker nicht (FR-SAF-04) -- taskTelemetry()
  // puffert bei fehlender Verbindung nur in den RAM-Ringpuffer. Am
  // Ersatzboard (Espressif ESP32-DevKitC-32E) unter Volllast (alle Sensoren/
  // Aktoren + BLE gleichzeitig) hardware-verifiziert, kein Brownout mehr
  // (s. docs/ble_brownout_fallstudie.md) -- Normalbetrieb.
  drivers::bleBegin();

  // FR-SAF-03: Task-Watchdog. Dieser Core initialisiert den TWDT bereits
  // vor setup() (sdkconfig: CONFIG_ESP_TASK_WDT_INIT=y, Default 5 s) --
  // esp_task_wdt_init() wuerde daher ESP_ERR_INVALID_STATE liefern.
  // esp_task_wdt_reconfigure() passt stattdessen den bereits laufenden TWDT
  // auf WATCHDOG_TIMEOUT_MS an; idle_core_mask=1 erhaelt die bestehende
  // Core-0-Idle-Ueberwachung. Fallback auf esp_task_wdt_init(), falls der
  // TWDT (z. B. nach einer Core-Konfigurationsaenderung) doch nicht
  // vorinitialisiert sein sollte.
  esp_task_wdt_config_t wdt_cfg = {WATCHDOG_TIMEOUT_MS, /*idle_core_mask=*/1, /*trigger_panic=*/true};
  if (esp_task_wdt_reconfigure(&wdt_cfg) != ESP_OK) {
    esp_task_wdt_init(&wdt_cfg);
  }
  // Registriert loopTaskHandle beim TWDT UND laesst den Arduino-Core dessen
  // eigenen loopTask-Wrapper vor jedem loop()-Aufruf automatisch
  // esp_task_wdt_reset() ausfuehren (esp32-hal-misc.c) -- kein manueller
  // Reset-Aufruf in unserem loop() noetig.
  enableLoopWDT();

  // TODO(FR-CFG-03): Konfiguration aus NVS laden (Defaults bei leerem NVS).
}

void loop() {
  // NFR-RT-04-Messung (v3, s. docs/Validierung/bench_run_notes.md
  // "Geltungsbereich der Zeitstatistik"): misst den realen kooperativen
  // Scheduler, nicht den BENCH_MODE-Busy-Wait-Harness. Absichtlich bis kurz
  // vor delay(1) gemessen -- der Idle-Schlaf ist gewollte CPU-Abgabe
  // (NFR-PWR-01), keine Verarbeitungszeit, die NFR-RT-04 beantworten soll.
  const uint32_t loop_start_us = micros();
  const uint32_t now = millis();

  // Jeden Durchlauf draint (kein eigener Timer): bei 9600 Bd und loop() bei
  // ~100 Hz+ (delay(1), keine Task blockiert lange) bleibt der Serial2-RX-
  // Puffer so sicher leer, ohne dass ein Ueberlauf droht.
  drivers::gnssPump();

  // Feste Reihenfolge, sicherheitsrelevantes zuerst (NFR-RT-03).
  if (now - t_imu  >= PERIOD_IMU_MS)  { t_imu  = now; taskLifecycleAndTailLight(); }
  taskRf();
  taskBlinker();
  if (now - t_baro >= PERIOD_BARO_MS) { t_baro = now; taskBaro(); }
  if (now - t_gnss >= PERIOD_GNSS_MS) { t_gnss = now; taskGnss(); }
  if (now - t_tele >= PERIOD_TELE_MS) { t_tele = now; taskTelemetry(); }
  taskConfigConsole();

  // Fuer den naechsten loop()-Durchlauf vorhalten (s. Kommentar bei der
  // Deklaration): der aktuelle Durchlauf ist erst hier abgeschlossen, kann
  // sich selbst also nicht mehr in sein eigenes 100-Hz-Fenster einreihen.
  lastLoopDurationUs = micros() - loop_start_us;

  // FR-SAF-03: kein manueller Watchdog-Reset noetig -- der Arduino-Core
  // fuettert den TWDT automatisch vor jedem loop()-Aufruf (s. setup()).
  // Optionaler CPU-Idle (NFR-PWR-01); haelt 100 Hz problemlos.
  delay(1);
}

#else  // BENCH_MODE ---------------------------------------------------------
// Serial-Bench zur Validierung der Bremslicht-Logik (NFR-TST-02). Eigene
// PlatformIO-Env (esp32dev_bench, s. platformio.ini) -- die Standard-Env
// esp32dev und damit Normalbetrieb/Sicherheitslogik bleiben unveraendert
// (obiger Zweig ist textidentisch zum bisherigen Code). Treibt tail_light_fsm
// + imu_health -- dieselben, unveraenderten Logik-Klassen wie im Normalbetrieb
// -- mit einem synthetischen decel_ms2-Profil statt echter IMU-Werte an
// derselben Stelle, an der im Normalbetrieb motion_filter seinen Ausgang
// liefert (s. taskLifecycleAndTailLight() oben). Kein delay()-Verbot hier:
// BENCH_MODE ist ein Offline-Diagnosewerkzeug, kein Fahrbetrieb (NFR-RT-03/04
// gelten fuer den Normalbetrieb).
namespace bench {

#ifndef FIRMWARE_GIT_HASH
#define FIRMWARE_GIT_HASH "unknown"  // per -D FIRMWARE_GIT_HASH="..." gesetzt
#endif

// Experiment A/C: linear 0->6.0 m/s^2 in 15 s, 1 s halten, linear 6.0->0 in 15 s.
float rampProfile(uint32_t t_ms) {
  if (t_ms <= 15000u) return 6.0f * (static_cast<float>(t_ms) / 15000.0f);
  if (t_ms <= 16000u) return 6.0f;
  if (t_ms <= 31000u) return 6.0f * (1.0f - static_cast<float>(t_ms - 16000u) / 15000.0f);
  return 0.0f;
}

// Experiment B: 1 s bei 0, Sprung auf 6.0 m/s^2 fuer 0,5 s, dann 0 (2 s Nachlauf).
float stepProfile(uint32_t t_ms) {
  if (t_ms < 1000u) return 0.0f;
  if (t_ms < 1500u) return 6.0f;
  return 0.0f;
}

// dt_s-Statistik je Experiment (min/max/mean ueber den real gemessenen
// Abtastabstand der Bench-Schleife): Nachweis, dass die Busy-Wait-Taktung
// tatsaechlich nahe 10 ms bleibt -- und Vergleichsbasis fuer den Nachweis,
// dass MOTION_DIAG_LOG_ENABLED die Taktung nicht messbar verfaelscht
// (zwei Bench-Captures, einmal mit/ohne Flag, s. stufe1_normgate.md).
struct DtStats {
  float dt_min_s = 1e9f;
  float dt_max_s = 0.0f;
  float dt_sum_s = 0.0f;
  uint32_t n = 0;
  uint32_t t_prev_ms = 0;

  void begin(uint32_t t0_ms) { t_prev_ms = t0_ms; }
  void sample(uint32_t t_now_ms) {
    const float dt_s = (t_now_ms - t_prev_ms) / 1000.0f;
    t_prev_ms = t_now_ms;
    if (n == 0) { ++n; return; }  // erstes Delta (t_prev==t0) nicht werten
    if (dt_s < dt_min_s) dt_min_s = dt_s;
    if (dt_s > dt_max_s) dt_max_s = dt_s;
    dt_sum_s += dt_s;
    ++n;
  }
  void print(const char* name) const {
    const uint32_t deltas = (n > 0) ? (n - 1) : 0;
    Serial.printf("# DT_STATS name=%s min_ms=%.3f max_ms=%.3f mean_ms=%.3f n=%lu diag_log=%d\n",
                  name, dt_min_s * 1000.0f, dt_max_s * 1000.0f,
                  deltas ? (dt_sum_s / deltas) * 1000.0f : 0.0f,
                  static_cast<unsigned long>(deltas), static_cast<int>(MOTION_DIAG_LOG_ENABLED));
  }
};

// Fuehrt ein Experiment aus: frische TailLightFsm/ImuHealth-Instanzen (jedes
// Experiment reproduzierbar aus definiertem Startzustand), 100-Hz-CSV ueber
// Serial zwischen "# BEGIN name" und "# END name".
//   force_imu_failed: read_ok=false fuer die gesamte Dauer (Experiment C);
//   ein nicht geloggter Vorlauf treibt imu_health VOR dem ersten geloggten
//   Sample bereits in FAILED (IMU_FAIL_LIMIT + IMU_RECOVERY_MAX_ATTEMPTS + 1
//   = 5+3+1 = 9 Zyklen genuegen rechnerisch, s. imu_health.cpp; 30 Zyklen
//   Marge).
void runExperiment(const char* name, uint32_t duration_ms,
                    float (*decel_fn)(uint32_t), bool force_imu_failed) {
  logic::TailLightFsm fsm;
  logic::ImuHealth health;

  if (force_imu_failed) {
    for (int i = 0; i < 30; ++i) {
      health.update(/*read_ok=*/false, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    }
  }

  Serial.printf("# META firmware=%s;board=ESP32-DevKitC-32E;fr_tl_07=disabled\n",
                FIRMWARE_GIT_HASH);
  Serial.printf("# BEGIN %s\n", name);
  Serial.println("t_ms,decel_ms2,brake_light_pct,fsm_state,imu_health");

  const uint32_t t0 = millis();
  uint32_t next_tick = t0;
  DtStats dt_stats;
  dt_stats.begin(t0);
  for (uint32_t t_ms = 0; t_ms <= duration_ms; t_ms += 10u) {
    // Feste 10-ms-Taktung (100 Hz) per Busy-Wait statt delay(): vermeidet
    // Drift ueber die bis zu 31 s lange Aufzeichnung. esp_task_wdt_reset()
    // manuell noetig, weil dieser Block komplett in setup() laeuft und die
    // automatische Fuetterung sonst erst zwischen loop()-Aufrufen griffe
    // (s. Kommentar im Normalbetrieb-loop() oben).
    while (static_cast<int32_t>(millis() - next_tick) < 0) {
      esp_task_wdt_reset();
    }
    esp_task_wdt_reset();
    next_tick += 10u;
    dt_stats.sample(millis());

    const float decel = decel_fn(t_ms);
    const bool read_ok = !force_imu_failed;
    // Synthetische, plausible Rohwerte (nur ausgewertet wenn read_ok, s.
    // imu_health.cpp): minimale Alternation an az verhindert die
    // Frozen-Erkennung (IMU_FROZEN_LIMIT), bleibt weit innerhalb der
    // Betrags-/Sprung-Schwellen -- Schwerkraftvektor im Stand (ax=ay=gx=0).
    const float az = 9.81f + (((t_ms / 10u) % 2u == 0u) ? 0.001f : -0.001f);
    const logic::ImuHealthOutput health_out =
        health.update(read_ok, 0.0f, 0.0f, az, 0.0f, t_ms);

    // Identische Gate-Logik wie taskLifecycleAndTailLight() im Normalbetrieb
    // (FR-STA-04): decel_ms2 wird nur bei read_ok && plausible && vertrauter
    // Eskalation durchgereicht, sonst 0 -- die geloggte decel_ms2-Spalte
    // zeigt trotzdem den rohen Profilwert (s. Feld-Kommentar telemetry_frame.h).
    const bool healthy_this_cycle = read_ok && health_out.plausible;
    const float tail_input =
        (healthy_this_cycle && health_out.escalation_trusted) ? decel : 0.0f;
    const logic::TailLightOutput tl = fsm.update(tail_input, logic::SystemState::Run, t_ms);

    drivers::setDutyPercent(PIN_BRAKE_LIGHT, tl.duty_pct);  // visuelle Kontrolle am realen LED-Pin

    Serial.printf("%lu,%.3f,%u,%d,%d\n", static_cast<unsigned long>(t_ms), decel,
                  static_cast<unsigned>(tl.duty_pct), static_cast<int>(tl.state),
                  static_cast<int>(health_out.state));
  }

  Serial.printf("# END %s\n", name);
  dt_stats.print(name);
}

// ---- Experimente D/E/F: voller Signalpfad ueber motion_filter ------------
// Im Gegensatz zu A/B/C (die direkt ein decel_ms2-Profil in brake_curve/
// tail_light_fsm einspeisen und damit motion_filter/lifecycle_fsm bewusst
// umgehen, s. Datei-Kopfkommentar) speisen D/E/F synthetische ax/ay/az/
// gyro_x-Rohwerte an genau der Stelle ein, an der im Normalbetrieb
// drivers::imuRead() liefert -- durchlaeuft also motion_filter real.
struct ImuProfileSample {
  float ax, ay, az, gyro_x;
};

constexpr float kBenchG = 9.80665f;

// Minimale Alternation an az verhindert IMU_FROZEN_LIMIT (ImuHealth stuft
// 30 bit-identische Zyklen als eingefrorenen Sensor ein, s.
// imu_health.cpp) -- bei einem synthetisch konstant gehaltenen Profil
// (Ruhe-/Halteplateaus in D/E/F) passiert das real nie, wuerde hier aber
// nach ~300 ms escalation_trusted=false erzwingen und die Kettenmessung
// verfaelschen. Identischer Kunstgriff wie in runExperiment() oben.
//
// BEWUSSTES TESTARTEFAKT, keine reale Sensor-Simulation -- Amplitude
// +-0,001 m/s^2 (Peak-Peak 0,002), alterniert jedes Sample. Sicherheits-
// abstand gegen die Regime-Schwellen (STATIC-Baseline ax=ay~0, az=g+-0,001):
//   accel_norm schwankt dadurch um +-0,001 m/s^2 (dominiert von az bei
//     ay~0) -- Faktor 120 unter MOTION_NORM_STATIC_BAND=0,12.
//   Jerk zwischen aufeinanderfolgenden Samples: |Delta_norm|/dt_s*0,01
//     = 0,002/0,01*0,01 = 0,002 m/s^2/10ms -- Faktor 1000 unter
//     MOTION_NORM_JERK_DELTA=2,0.
// Beide Sicherheitsabstaende >> Faktor 10 -- der Kunstgriff beeinflusst
// weder Regime-Klassifikation noch den Filterausgang messbar.
//
// Die Alternationsamplitude von +-0,001 m/s^2 liegt unterhalb der
// Sensoraufloesung: Der MPU6050 liefert im +-16-g-Bereich 2048 LSB/g, ein
// LSB entspricht 0,00479 m/s^2. Das Artefakt betraegt somit ca. 0,21 LSB
// und ist auf realer Hardware nicht darstellbar -- es ist ein rein
// rechnerischer Kunstgriff im Testpfad. Daraus folgt zugleich, dass die
// Freeze-Erkennung in ImuHealth auf den skalierten Float-Werten arbeitet
// und nicht auf den Rohregistern.
//
// Ein Fehlausloesen der Freeze-Erkennung im Feld ist dadurch nicht zu
// erwarten: Das Rauschen des MPU6050 liegt bei rund 400 ug/sqrt(Hz), bei
// 100 Hz Bandbreite also bei etwa 0,04 m/s^2 effektiv -- rund acht LSB. Der
// Sensor dithert im Stillstand ausreichend, um nie als eingefroren zu gelten.
float benchAzJitter(uint32_t t_ms) {
  return kBenchG + (((t_ms / 10u) % 2u == 0u) ? 0.001f : -0.001f);
}

// Experiment D: 2,5 s Ruhe, 4 s konstante Verzoegerung 4,0 m/s^2, 2 s Ruhe.
// Vorlauf 2500 ms statt urspruenglich 1000 ms (A6.1b): MOTION_ANCHOR_WINDOW_S
// =1,0 s verlangt 1,0 s zusammenhaengendes STATIC; ein Vorlauf von exakt
// 1000 ms hat null Marge gegen jede Taktschwankung (s. bench_run_notes.md,
// Schritt A -- dort dadurch bias_calibrated=0 statt 1). 2500 ms geben 150 %
// Reserve und lassen zusaetzlich die Stufe-1-Bias-Mittelung sauber
// abschliessen.
ImuProfileSample profileD(uint32_t t_ms) {
  const float az = benchAzJitter(t_ms);
  if (t_ms < 2500u) return {0.0f, 0.0f, az, 0.0f};
  if (t_ms < 6500u) return {0.0f, 4.0f, az, 0.0f};
  return {0.0f, 0.0f, az, 0.0f};
}

// Experiment E: Konstantfahrt mit drei eingestreuten Stoessen
// (15/30/45 m/s^2, je ~20-30 ms). Erster Stoss bei 2500 ms statt 1000 ms
// (A6.1b, identische Begruendung wie bei profileD), Abstand zwischen den
// Stoessen unveraendert bei 1000 ms.
ImuProfileSample profileE(uint32_t t_ms) {
  const float az = benchAzJitter(t_ms);
  if (t_ms >= 2500u && t_ms < 2525u) return {0.0f, 15.0f, az, 0.0f};
  if (t_ms >= 3500u && t_ms < 3530u) return {0.0f, 30.0f, az, 0.0f};
  if (t_ms >= 4500u && t_ms < 4520u) return {0.0f, 45.0f, az, 0.0f};
  return {0.0f, 0.0f, az, 0.0f};
}

// Experiment F: 8-Grad-Gefaelle ueber 6 s, davon in der Mitte 2 s zusaetzlich
// 3,5 m/s^2 Bremsung (t=2000..4000 ms).
ImuProfileSample profileF(uint32_t t_ms) {
  constexpr float kTiltRad = 0.13963f;  // 8 Grad
  const float ay_tilt = kBenchG * sinf(kTiltRad);
  const float az_tilt = kBenchG * cosf(kTiltRad) + (benchAzJitter(t_ms) - kBenchG);
  if (t_ms >= 2000u && t_ms < 4000u) return {0.0f, ay_tilt + 3.5f, az_tilt, 0.0f};
  return {0.0f, ay_tilt, az_tilt, 0.0f};
}

// Eingefrorene historische Momentaufnahme des VOR dieser Stufe verwendeten
// Filters (fester alpha=0,98, keine Regime-Klassifikation, keine
// Verankerung) -- AUSSCHLIESSLICH fuer den Vorher-Nachher-Vergleich in
// Experiment D (Nachweis "Scheinneigung", s. stufe1_normgate.md). Repliziert
// absichtlich den durch den Feldtest 06.08.2026 belegten Fehler; bewusst
// NICHT Teil von lib/logic (dort bleibt nur die neue, korrigierte Fassung).
struct LegacyFilterState {
  float pitch_rad = 0.0f;
};
float legacyFilterUpdate(LegacyFilterState& s, float ay, float az, float gyro_x, float dt_s) {
  constexpr float kAlpha = 0.98f;
  const float pitch_from_accel = atan2f(ay, az);
  s.pitch_rad = kAlpha * (s.pitch_rad + gyro_x * dt_s) + (1.0f - kAlpha) * pitch_from_accel;
  const float gravity_y = kBenchG * sinf(s.pitch_rad);
  const float linear = ay - gravity_y;
  return linear > 0.0f ? linear : 0.0f;
}

// Analog runExperiment(), aber durch die volle Kette motion_filter ->
// tail_light_fsm (mit imu_health dazwischen, wie im Normalbetrieb). Das
// primaere CSV-Format bleibt identisch zu A/B/C (derselbe Python-Splitter
// funktioniert unveraendert); MOTION_DIAG_LOG_ENABLED und der Legacy-
// Vergleich haengen als "#"-praefigierte Zusatzzeilen daneben (vom
// Splitter-Regex ^\d+... bereits ausgefiltert, s. bench_run_notes.md).
void runFullChainExperiment(const char* name, uint32_t duration_ms,
                             ImuProfileSample (*profile_fn)(uint32_t),
                             bool with_legacy_comparison) {
  logic::MotionFilter motion;
  logic::ImuHealth health;
  logic::TailLightFsm fsm;
  LegacyFilterState legacy;

  Serial.printf("# META firmware=%s;board=ESP32-DevKitC-32E;fr_tl_07=disabled;motion_diag_log=%d\n",
                FIRMWARE_GIT_HASH, static_cast<int>(MOTION_DIAG_LOG_ENABLED));
  Serial.printf("# BEGIN %s\n", name);
  Serial.println("t_ms,decel_ms2,brake_light_pct,fsm_state,imu_health");

  const uint32_t t0 = millis();
  // A6.1a: next_tick MUSS beim ersten Tick bereits 10 ms voraus liegen.
  // Vorher stand hier next_tick=t0 -- die Busy-Wait-Bedingung
  // (millis()-next_tick<0) war dann beim allerersten Schleifendurchlauf
  // sofort erfuellt (kein Warten), t_real lag praktisch bei t0, und das
  // erste dt_s=(t_real-t_prev_real) mass dadurch real nur ~0..1 ms statt
  // der nominellen 10 ms -- eine ~9-ms-Luecke im allerersten Sample. Bei
  // Profilen mit einer STATIC-Vorlaufzeit ohne Marge gegen
  // MOTION_ANCHOR_WINDOW_S=1,0 s (urspruenglich D/E, s. bench_run_notes.md
  // Schritt A) reichte das, um die Verankerung genau zu verfehlen. Mit
  // next_tick=t0+10 wartet auch der erste Durchlauf reguraer bis zum
  // ersten 10-ms-Tick, dt_s ist dadurch von Anfang an korrekt.
  uint32_t next_tick = t0 + 10u;
  uint32_t t_prev_real = t0;
  DtStats dt_stats;
  dt_stats.begin(t0);
  uint32_t dt_clamp_count = 0;      // A4: wie oft hat das 1..50-ms-Clamping gegriffen?
  uint32_t sample_index = 0;        // A6.5: Sample-Index fuer Clamp-Ereignis-Zuordnung
  bool bias_calib_seen = false;     // A2: Zeitpunkt/Wert der Stufe-1-Kalibrierung
  uint32_t bias_calib_t_real_ms = 0;
  for (uint32_t t_ms = 0; t_ms <= duration_ms; t_ms += 10u, ++sample_index) {
    while (static_cast<int32_t>(millis() - next_tick) < 0) {
      esp_task_wdt_reset();
    }
    esp_task_wdt_reset();
    const uint32_t t_real = millis();
    next_tick += 10u;
    dt_stats.sample(t_real);

    float dt_s = (t_real - t_prev_real) / 1000.0f;
    const uint32_t dt_real_ms = t_real - t_prev_real;  // A6.4: reales dt fuer Std/Histogramm
    if (dt_s < 0.001f) {
      dt_s = 0.001f;
      ++dt_clamp_count;
      Serial.printf("# DT_CLAMP_EVENT name=%s idx=%lu t_ms=%lu dt_real_ms=%lu reason=low\n", name,
                    static_cast<unsigned long>(sample_index), static_cast<unsigned long>(t_ms),
                    static_cast<unsigned long>(dt_real_ms));
    }
    if (dt_s > 0.050f) {
      dt_s = 0.050f;
      ++dt_clamp_count;
      Serial.printf("# DT_CLAMP_EVENT name=%s idx=%lu t_ms=%lu dt_real_ms=%lu reason=high\n", name,
                    static_cast<unsigned long>(sample_index), static_cast<unsigned long>(t_ms),
                    static_cast<unsigned long>(dt_real_ms));
    }
    t_prev_real = t_real;

    const ImuProfileSample s = profile_fn(t_ms);
    const float decel = motion.update({s.ax, s.ay, s.az, s.gyro_x, dt_s});
    const logic::ImuHealthOutput health_out = health.update(true, s.ax, s.ay, s.az, s.gyro_x, t_ms);
    const bool healthy_this_cycle = health_out.plausible;
    const float tail_input = (healthy_this_cycle && health_out.escalation_trusted) ? decel : 0.0f;
    const logic::TailLightOutput tl = fsm.update(tail_input, logic::SystemState::Run, t_ms);

    drivers::setDutyPercent(PIN_BRAKE_LIGHT, tl.duty_pct);

    if (!bias_calib_seen && motion.biasCalibrated()) {  // A2: Zeitpunkt der Stufe-1-Kalibrierung
      bias_calib_seen = true;
      bias_calib_t_real_ms = t_real;
    }

    Serial.printf("%lu,%.3f,%u,%d,%d\n", static_cast<unsigned long>(t_ms), decel,
                  static_cast<unsigned>(tl.duty_pct), static_cast<int>(tl.state),
                  static_cast<int>(health_out.state));

    if (MOTION_DIAG_LOG_ENABLED) {
      // dt_real_ms (A6.4): reales, ungeglaettetes Abtastintervall dieses
      // Samples -- ermoeglicht Std/Histogramm offline aus dem Capture, ohne
      // dass das Board selbst eine Rohserie mitfuehren muss.
      Serial.printf("# DIAG %lu,%.3f,%.3f,%.3f,%.4f,%.3f,%.3f,%d,%.4f,%lu\n",
                    static_cast<unsigned long>(t_ms), s.ax, s.ay, s.az, s.gyro_x,
                    motion.accel_norm(), motion.normDelta(), static_cast<int>(motion.regime()),
                    motion.pitch_rad(), static_cast<unsigned long>(dt_real_ms));
    }
    if (with_legacy_comparison) {
      const float legacy_decel = legacyFilterUpdate(legacy, s.ay, s.az, s.gyro_x, dt_s);
      Serial.printf("# LEGACY %lu,%.3f\n", static_cast<unsigned long>(t_ms), legacy_decel);
    }
  }

  Serial.printf("# END %s\n", name);
  dt_stats.print(name);
  // A4: wie oft hat das dt_s-Clamping (1..50 ms, s. taskLifecycleAndTailLight()
  // im Normalbetrieb-Zweig oben) in diesem Experiment gegriffen?
  Serial.printf("# DT_CLAMP name=%s count=%lu\n", name, static_cast<unsigned long>(dt_clamp_count));
  // A2: bias_calibrated_ nur bench-seitig sichtbar machen (noch kein BLE-
  // Telemetriefeld) -- tau_slow wird hier extern repliziert statt ueber
  // einen neuen Getter, da MotionFilter bereits alle dafuer noetigen Werte
  // oeffentlich macht (gyro_bias_enabled ist compile-zeit-konstant per
  // MOTION_GYRO_BIAS_ENABLED, biasCalibrated() ist bereits ein Getter) --
  // identische Bedingung wie in motion_filter.cpp update().
  const float active_tau_slow = (MOTION_GYRO_BIAS_ENABLED && !motion.biasCalibrated())
                                     ? MOTION_COMPL_TAU_SLOW_UNCAL_S
                                     : MOTION_COMPL_TAU_SLOW_S;
  Serial.printf(
      "# BIAS_CAL name=%s calibrated=%d t_real_ms=%lu bias_rad_s=%.5f bias_deg_s=%.4f tau_slow_s=%.1f\n",
      name, static_cast<int>(motion.biasCalibrated()), static_cast<unsigned long>(bias_calib_t_real_ms),
      motion.gyroBiasRads(), motion.gyroBiasRads() * (180.0f / 3.14159265f), active_tau_slow);
}

}  // namespace bench

void setup() {
  // 921600 Baud (statt 115200 im Normalbetrieb, s. platformio.ini
  // monitor_speed je Environment): das volle Diagnose-Log
  // (MOTION_DIAG_LOG_ENABLED) sowie die zusaetzlichen D/E/F-Spalten
  // erhoehen die Datenrate spuerbar; 115200 waere hier ein Flaschenhals.
  Serial.begin(921600);
  delay(200);  // USB-Serial-CDC-Enumeration abwarten, bevor die erste Zeile geht

  drivers::attach(PIN_BRAKE_LIGHT);  // realer LED-Pin, fuer visuelle Kontrolle waehrend der Bench

  bench::runExperiment("bench_A_kennlinie_rampe", 31000u, bench::rampProfile,
                        /*force_imu_failed=*/false);
  bench::runExperiment("bench_B_zeitverhalten_sprung", 3500u, bench::stepProfile,
                        /*force_imu_failed=*/false);
  bench::runExperiment("bench_C_failsafe", 31000u, bench::rampProfile,
                        /*force_imu_failed=*/true);

  // D/E/F: voller Signalpfad ueber motion_filter (s. Kommentar oben) --
  // Nachweis der Regime-/Verankerungslogik am realen MCU, nicht nur host-
  // seitig. Experiment D zusaetzlich mit Legacy-Vergleich (Vorher-Nachher).
  // Dauer an die A6.1b-Vorlaufzeit (2500 statt 1000 ms) angepasst: D
  // 2500+4000+2000=8500 ms, E 2500+3x(1000 ms Abstand)+1000 ms Nachlauf
  // =5500 ms.
  bench::runFullChainExperiment("bench_D_scheinneigung", 8500u, bench::profileD,
                                 /*with_legacy_comparison=*/true);
  bench::runFullChainExperiment("bench_E_stossunterdrueckung", 5500u, bench::profileE,
                                 /*with_legacy_comparison=*/false);
  bench::runFullChainExperiment("bench_F_neigung_plus_bremsung", 6000u, bench::profileF,
                                 /*with_legacy_comparison=*/false);

  Serial.println("# BENCH DONE");
}

void loop() {
  delay(1000);  // Bench beendet -- Idle, kein Normalbetrieb in diesem Build
}
#endif  // BENCH_MODE
