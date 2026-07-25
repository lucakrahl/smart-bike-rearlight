// imu_driver.h — MPU6050-Treiber (I2C 0x68, Adafruit-Bibliothek, FR-SNS-01/02)
// HARDWARE-TREIBER: kapselt nur die I2C-/Adafruit-API, enthaelt keine
// Zustands-/Filterlogik (die liegt in firmware/lib/logic/motion_filter.h).
// Liefert rohe Beschleunigung [m/s^2, inkl. g] und Drehrate [rad/s].
#pragma once

namespace drivers {

struct ImuSample {
  float accel_x_ms2, accel_y_ms2, accel_z_ms2;  // inkl. Erdbeschleunigung g
  float gyro_x_rads;                             // Drehrate um die Nickachse (X)
};

// Initialisiert den MPU6050 (I2C, Adresse aus config.h). I2C-Zugriffe
// zeitbegrenzt ueber Wire-Timeout (FR-SNS-03). Rueckgabe: true = Init
// erfolgreich ("IMU ready"). Blockierend, nur fuer setup() gedacht.
bool imuBegin();

// Letztes Ergebnis von imuBegin().
bool imuIsReady();

// Liest ein Rohsample. TODO(FR-SNS-04/05, M3-Folgeschritt): I2C-Recovery bei
// Bus-Fehlern und Plausibilitaetspruefung (Wertebereich/Eingefroren-Erkennung)
// folgen als eigener Schritt; aktuell keine Fehlerbehandlung bei Lesefehlern.
ImuSample imuRead();

}  // namespace drivers
