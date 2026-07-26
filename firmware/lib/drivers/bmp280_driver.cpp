// bmp280_driver.cpp — Umsetzung BMP280-Treiber. Hardwareabhaengig (Arduino.h).
#include "bmp280_driver.h"
#include <Arduino.h>
#include <Adafruit_BMP280.h>
#include "config.h"

namespace drivers {

namespace {
Adafruit_BMP280 bmp;  // Default-Konstruktor bindet an das globale Wire (I2C)
bool ready = false;

// Sampling-Profil "Weather Monitoring" aus dem Bosch-BMP280-Datenblatt
// (Kap. 3.5) -- genau unser Fall: langsamer, vereinzelter Messtakt statt
// hoher Dynamik. Oversampling x1/x1 minimiert die Selbsterwaermung pro
// Schuss (Grund fuer FORCED statt NORMAL: +2,6 °C Eigenerwaermung im
// gemessenen Dauerbetrieb). IIR-Filter AUS, weil er nur bei fortlaufenden
// NORMAL-Samples wirkt -- bei vereinzelten FORCED-Schuessen gibt es keine
// Historie zu glaetten. Standby-Wert ist im FORCED-Mode wirkungslos (Sensor
// schlaeft nach jeder Messung automatisch), API verlangt aber einen Wert.
//
// Konversionszeit lt. Datenblatt-Formel (max.):
//   1,25 + 2,3*osrs_t + (2,3*osrs_p + 0,575) ms = 1,25+2,3+(2,3+0,575) = 6,4 ms
// Das ist so viel kuerzer als BARO_FORCED_CYCLE_MS (1000 ms), dass Trigger
// und Read gefahrlos auf zwei aufeinanderfolgende Zyklen verteilt werden
// koennen, ohne je auf das Ergebnis warten zu muessen.
void triggerForcedMeasurement() {
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,
                   Adafruit_BMP280::SAMPLING_X1,   // Temperatur-Oversampling
                   Adafruit_BMP280::SAMPLING_X1,   // Druck-Oversampling
                   Adafruit_BMP280::FILTER_OFF,
                   Adafruit_BMP280::STANDBY_MS_1);  // in FORCED ohne Wirkung
}

}  // namespace

bool bmp280Begin() {
  ready = bmp.begin(BMP280_I2C_ADDR);
  if (ready) {
    triggerForcedMeasurement();  // konfiguriert FORCED-Sampling + 1. Messung
  }
  return ready;
}

bool bmp280IsReady() {
  return ready;
}

void bmp280TriggerMeasurement() {
  triggerForcedMeasurement();
}

BaroSample bmp280Read() {
  return BaroSample{ bmp.readPressure(), bmp.readTemperature() };
}

}  // namespace drivers
