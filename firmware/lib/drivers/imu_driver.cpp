// imu_driver.cpp — Umsetzung MPU6050-Treiber. Hardwareabhaengig (Arduino.h).
#include "imu_driver.h"
#include <Arduino.h>
#include <Wire.h>
#include <driver/gpio.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "pins.h"
#include "config.h"
#include "imu_mount_orientation.h"

namespace drivers {

namespace {
Adafruit_MPU6050 mpu;
bool ready = false;

// WHO_AM_I-Check auf Wire-Ebene: liefert im Gegensatz zu getEvent()
// zuverlaessig false, wenn der Sensor am Bus nicht (mehr) antwortet
// (endTransmission()-Status != 0) oder ein falscher/kein Slave antwortet.
bool whoAmIOk() {
  Wire.beginTransmission(MPU6050_I2C_ADDR);
  Wire.write(MPU6050_WHOAMI_REG);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(MPU6050_I2C_ADDR, (uint8_t)1) != 1) {
    return false;
  }
  return Wire.read() == MPU6050_WHOAMI_VAL;
}

// I2C-Standard-Bus-Recovery: SCL manuell takten, bis SDA wieder freigegeben
// ist (haengender Slave mitten in einer Transaktion), dann eine manuelle
// STOP-Bedingung erzeugen. Bounded auf 9 Takte (max. Bytebreite); jeder Puls
// wenige Mikrosekunden -> Gesamtdauer << 10 ms (NFR-RT-04). Das ist
// Bus-Timing, kein Warten auf Anwendungslogik, daher kein delay()-Verstoss
// gegen NFR-RT-03.
//
// Bewusst rohe ESP-IDF-GPIO-Calls (gpio_*) statt pinMode()/digitalWrite():
// letztere gehen ueber den PeriMan-Pin-Besitz (esp32-hal-periman.c). Solange
// der I2C-Treiber die Pins noch als "besetzt" fuehrt (genau der Fall, wenn
// der Bus haengt -- s. imuRecoverStage()), ruft perimanSetPinBus() zuerst
// den I2C-Deinit-Callback auf, scheitert am selben zugrunde liegenden
// Fehler und pinMode() bricht dann KOMMENTARLOS ab (kein Effekt auf den
// Pin). Verifiziert im installierten Core (esp32-hal-gpio.c/-periman.c).
// gpio_set_direction()/gpio_set_level() umgehen PeriMan vollstaendig.
void releaseStuckScl() {
  gpio_set_direction(static_cast<gpio_num_t>(PIN_I2C_SCL), GPIO_MODE_OUTPUT);
  gpio_set_direction(static_cast<gpio_num_t>(PIN_I2C_SDA), GPIO_MODE_INPUT);
  gpio_pullup_en(static_cast<gpio_num_t>(PIN_I2C_SDA));

  for (int i = 0; i < 9 && gpio_get_level(static_cast<gpio_num_t>(PIN_I2C_SDA)) == 0; ++i) {
    gpio_set_level(static_cast<gpio_num_t>(PIN_I2C_SCL), 0);
    delayMicroseconds(5);
    gpio_set_level(static_cast<gpio_num_t>(PIN_I2C_SCL), 1);
    delayMicroseconds(5);
  }
  // Manuelle STOP-Bedingung: SDA von LOW nach HIGH, waehrend SCL HIGH ist.
  gpio_set_direction(static_cast<gpio_num_t>(PIN_I2C_SDA), GPIO_MODE_OUTPUT);
  gpio_set_level(static_cast<gpio_num_t>(PIN_I2C_SDA), 0);
  delayMicroseconds(5);
  gpio_set_level(static_cast<gpio_num_t>(PIN_I2C_SCL), 1);
  delayMicroseconds(5);
  gpio_set_level(static_cast<gpio_num_t>(PIN_I2C_SDA), 1);
  delayMicroseconds(5);
}

// Direkter Registerzugriff fuer CONFIG (DLPF)/SMPLRT_DIV: Adafruit_MPU6050
// bietet dafuer keine API, und die Bibliothek selbst schreibt beide
// Register nie -- der Power-On-Reset-Default bleibt sonst stehen (260 Hz
// Accel-Bandbreite bei 8 kHz interner Rate). Bei 100 Hz Firmware-
// Auslesung (PERIOD_IMU_MS) bedeutet das Unterabtastung OHNE Anti-Alias-
// Filterung -- am realen Board per Registerauslesung nachgewiesen
// (B-FW.11, s. docs/Validierung/bench_run_notes.md) und als plausible
// Hauptursache der ueberhoehten norm_delta-Streuung identifiziert
// (gemessen ±0,6-0,7 m/s^2 im Ruhezustand statt der aus dem Datenblatt-
// Rauschen (400 ug/sqrt(Hz)) erwarteten ±0,24 m/s^2).
//
// DLPF_CFG=3 -> Accel-Bandbreite 44 Hz (< 50 Hz = halbe Auslesefrequenz,
// damit kein Aliasing mehr) UND Gyro-Basisrate wechselt von 8 kHz auf
// 1 kHz. SMPLRT_DIV=4 -> Sensor-Sample-Rate = 1000/(1+4) = 200 Hz --
// bewusst NICHT 100 Hz (Deckungsgleichheit mit der 100-Hz-Auslesung
// wuerde eine Schwebung zwischen Sensor- und Auslesetakt riskieren, falls
// beide Takte minimal auseinanderlaufen; 200 Hz liefert der Firmware bei
// jeder zweiten Auslesung sicher ein frisches Sample).
//
// Zusaetzliche Gruppenlaufzeit durch den 44-Hz-Filter: 4,9 ms (Datenblatt)
// gegen NFR-RT-01 (<=50 ms Bremslicht-Reaktionszeit) -- 4,9 ms sind ein
// Zehntel des Budgets, lassen komfortable Reserve fuer I2C-Lesezeit,
// motion_filter/tail_light_fsm-Verarbeitung und PWM-Ausgabe.
constexpr uint8_t kRegConfig = 0x1A;
constexpr uint8_t kRegSmplrtDiv = 0x19;
constexpr uint8_t kDlpfCfg44Hz = 0x03;
constexpr uint8_t kSmplrtDiv200Hz = 4;

uint8_t readReg(uint8_t reg) {
  Wire.beginTransmission(MPU6050_I2C_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_I2C_ADDR, static_cast<uint8_t>(1));
  return Wire.read();
}

void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU6050_I2C_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

// Schreibt CONFIG/SMPLRT_DIV und liest sie zurueck, um die tatsaechlich
// wirksame Konfiguration zu verifizieren (nicht nur den Schreibaufruf
// anzunehmen -- I2C-Schreibfehler waeren sonst unbemerkt). Rueckgabe:
// true, wenn beide Register den Soll-Wert tragen.
bool configureDlpfAndSampleRate() {
  writeReg(kRegConfig, kDlpfCfg44Hz);
  writeReg(kRegSmplrtDiv, kSmplrtDiv200Hz);
  const uint8_t config_readback = readReg(kRegConfig);
  const uint8_t smplrt_readback = readReg(kRegSmplrtDiv);
  const bool ok = (config_readback == kDlpfCfg44Hz) && (smplrt_readback == kSmplrtDiv200Hz);
  if (DEBUG_SERIAL) {
    Serial.printf("[IMU] DLPF/SMPLRT_DIV gesetzt+zurueckgelesen: CONFIG=0x%02X (soll 0x%02X) "
                  "SMPLRT_DIV=%u (soll %u) -> %s\n",
                  config_readback, kDlpfCfg44Hz, smplrt_readback, kSmplrtDiv200Hz,
                  ok ? "OK" : "FEHLER");
  }
  return ok;
}

}  // namespace

bool imuBegin() {
  // Bus (Wire.begin()/setTimeOut(), FR-SNS-03) wird zentral in main.cpp/
  // setup() initialisiert; dieser Treiber ist reiner Busnutzer.
  ready = mpu.begin(MPU6050_I2C_ADDR, &Wire);
  if (ready) {
    // +-16 g statt Library-Default: Fahrrad-Stoesse (Bordstein, Schlagloch)
    // koennen ~20 g erreichen; ein engeres FSR wuerde dort saettigen und
    // die Plausibilitaetspruefung (imu_health, IMU_ACCEL_MAX_MAGNITUDE_MS2)
    // faelschlich ansprechen lassen.
    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
    // DLPF/Sample-Rate fest konfigurieren (s. Kommentar oben) -- mpu.begin()
    // fuehrt intern einen Register-Reset durch, DLPF/SMPLRT_DIV muessen
    // deshalb NACH begin() gesetzt werden, sonst wirkt der Reset ueberschreibend.
    ready = configureDlpfAndSampleRate();
  }
  return ready;
}

bool imuIsReady() {
  return ready;
}

bool imuRead(ImuSample& out) {
  if (!whoAmIOk()) {
    return false;
  }
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);
  // Einbaulage-Rueckabbildung an der Treibergrenze (s. IMU_MOUNT_SIGN_* in
  // config.h) -- alle nachgelagerten Komponenten (motion_filter, ...) sehen
  // weiterhin die urspruengliche, validierte Achsenkonvention.
  logic::RawImuSample raw{
    accel.acceleration.x,
    accel.acceleration.y,
    accel.acceleration.z,
    gyro.gyro.x,
    gyro.gyro.y,
    gyro.gyro.z,
  };
  logic::applyMountOrientation(raw);
  out = ImuSample{
    raw.accel_x_ms2,
    raw.accel_y_ms2,
    raw.accel_z_ms2,
    raw.gyro_x_rads,
    raw.gyro_y_rads,
    raw.gyro_z_rads,
  };
  return true;
}

void imuRecoverStage(int stage) {
  if (stage == 2) {
    // Muss VOR Wire.end() laufen: der ESP-IDF-Treiber deinitialisiert
    // (i2c_del_master_bus()) nur zuverlaessig, wenn der Bus physisch frei
    // ist. Bei einem haengenden/kurzgeschlossenen Bus kann das sonst
    // fehlschlagen -- der Treiber bleibt dann als "initialisiert" markiert
    // und der folgende Wire.begin() wird zum wirkungslosen No-op (verifiziert
    // im installierten Core: Wire.cpp/esp32-hal-i2c-ng.c).
    releaseStuckScl();
  }
  const bool end_ok = Wire.end();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setTimeOut(I2C_TIMEOUT_MS);
  const bool begin_ok = imuBegin();
  // Diagnose fuer den Fehlerinjektionstest: macht sichtbar, ob der I2C-
  // Treiber diesmal wirklich deinitialisiert wurde, auch waehrend der
  // physische Fehler (z. B. SDA-Kurzschluss) noch ansteht.
  if (DEBUG_SERIAL) {
    Serial.printf("[IMU-Recovery] stage=%d wire_end_ok=%d imu_begin_ok=%d\n", stage,
                  (int)end_ok, (int)begin_ok);
  }
}

}  // namespace drivers
