// imu_driver.cpp — Umsetzung MPU6050-Treiber. Hardwareabhaengig (Arduino.h).
#include "imu_driver.h"
#include <Arduino.h>
#include <Wire.h>
#include <driver/gpio.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "pins.h"
#include "config.h"

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
  out = ImuSample{
    accel.acceleration.x,
    accel.acceleration.y,
    accel.acceleration.z,
    gyro.gyro.x,
    gyro.gyro.y,
    gyro.gyro.z,
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
