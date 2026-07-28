// imu_driver.h — MPU6050-Treiber (I2C 0x68, Adafruit-Bibliothek, FR-SNS-01/02)
// HARDWARE-TREIBER: kapselt nur die I2C-/Adafruit-API, enthaelt keine
// Zustands-/Filterlogik (die liegt in firmware/lib/logic/motion_filter.h)
// und keine Gesundheitsbewertung (die liegt in firmware/lib/logic/imu_health.h).
// Liefert rohe Beschleunigung [m/s^2, inkl. g] und Drehrate [rad/s].
#pragma once

namespace drivers {

struct ImuSample {
  float accel_x_ms2, accel_y_ms2, accel_z_ms2;  // inkl. Erdbeschleunigung g
  // Drehrate [rad/s]. Nur gyro_x wird von motion_filter genutzt (Nickachse);
  // gyro_y/gyro_z werden zusaetzlich erfasst, weil FR-TEL-03 alle 6 Achsen
  // im Telemetrie-Frame verlangt (s. lib/logic/telemetry_frame.h).
  float gyro_x_rads, gyro_y_rads, gyro_z_rads;
};

// Initialisiert den MPU6050 (I2C, Adresse aus config.h, FSR +-16 g wegen
// Fahrrad-Stoessen bis ~20 g) auf dem zentral in main.cpp/setup()
// eingerichteten Bus (Wire.begin()/setTimeOut(), FR-SNS-03 — dieser Treiber
// ruft selbst kein Wire.begin() auf). Rueckgabe: true = Init erfolgreich
// ("IMU ready"). Blockierend, nur fuer setup() gedacht.
bool imuBegin();

// Letztes Ergebnis von imuBegin().
bool imuIsReady();

// Liest ein Rohsample nach "out". Rueckgabe = Erfolg: verlaesst sich NICHT
// auf den getEvent()-Rueckgabewert der Adafruit-Bibliothek (reicht I2C-
// Fehler nicht zuverlaessig durch), sondern prueft den Bus selbst per
// WHO_AM_I-Register (endTransmission()-Status + Fixwert, FR-SNS-04/05).
// false ⇒ "out" bleibt unveraendert und darf vom Aufrufer nicht verwendet
// werden.
bool imuRead(ImuSample& out);

// Fuehrt GENAU EINEN Recovery-Schritt aus (nicht-blockierend, Pacing/
// Eskalation liegt in firmware/lib/logic/imu_health.h):
//   Stufe 1: Soft-Reinit (Wire.end()/begin() + imuBegin()).
//   Stufe 2: SCL-Clock-Release (haengenden I2C-Slave loesen) + Soft-Reinit.
// Wire.end()/begin() ist bus-weit und betrifft daher auch den BMP280
// (gemeinsamer Bus) — unkritisch, da optionaler Sensor (FR-STA-05).
void imuRecoverStage(int stage);

}  // namespace drivers
