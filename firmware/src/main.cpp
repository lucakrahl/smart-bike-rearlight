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
    const float dt_s = (now - t_motion_prev_ms) / 1000.0f;
    t_motion_prev_ms = now;
    decel_ms2 = motionFilter.update(
        {raw.accel_x_ms2, raw.accel_y_ms2, raw.accel_z_ms2, raw.gyro_x_rads, dt_s});
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

  // FR-SAF-03: kein manueller Watchdog-Reset noetig -- der Arduino-Core
  // fuettert den TWDT automatisch vor jedem loop()-Aufruf (s. setup()).
  // Optionaler CPU-Idle (NFR-PWR-01); haelt 100 Hz problemlos.
  delay(1);
}
