// bmp280_driver.h — BMP280-Treiber (I2C 0x76, Adafruit-Bibliothek, FR-SNS-01/02)
// HARDWARE-TREIBER: kapselt nur die I2C-/Adafruit-API, enthaelt keine
// Zustandslogik. Liefert rohe Druck-/Temperaturwerte — FR-SYS-01/Bible 2.2:
// Hoehe/Hoehenmeter werden in der App berechnet, nicht in der Firmware.
//
// Optionaler Sensor (FR-STA-05): ein Ausfall darf Licht/Blinker nicht
// beeinflussen (die haengen nur an der IMU als kritischem Sensor, FR-STA-01).
//
// Nutzt den zentral in main.cpp/setup() initialisierten I2C-Bus
// (Wire.begin()/setTimeOut(), FR-SNS-03) — ruft selbst kein Wire.begin() auf.
#pragma once

namespace drivers {

struct BaroSample {
  float pressure_pa;    // Pa, roh
  float temperature_c;  // Grad C, roh
};

// Initialisiert den BMP280 (Adresse aus config.h) im FORCED-Mode mit
// explizitem, reduziertem Sampling (kein Bibliotheks-Default, s. .cpp) --
// FORCED statt NORMAL, weil Dauerbetrieb eine Selbsterwaermung von +2,6 °C
// verursacht hat. Loest ueber setSampling() bereits die erste Messung aus.
// Rueckgabe: true = Init erfolgreich ("ready"). Blockierend, nur fuer
// setup() gedacht.
bool bmp280Begin();

// Letztes Ergebnis von bmp280Begin().
bool bmp280IsReady();

// Stoesst eine neue FORCED-Messung an (schreibt ctrl_meas, Sensor startet die
// Konversion und schlaeft danach automatisch wieder ein). Kehrt sofort
// zurueck -- kein Warten auf das Ergebnis. Aufrufer liest das Ergebnis erst
// im naechsten Zyklus per bmp280Read() (s. taskBaro()).
void bmp280TriggerMeasurement();

// Liest die zuletzt abgeschlossene Messung (reiner Registerzugriff, kein
// Trigger, keine Wartezeit). Nur sinnvoll, nachdem seit dem letzten
// bmp280TriggerMeasurement() genug Zeit vergangen ist (Konversionszeit bei
// den gewaehlten Oversampling-Werten « BARO_FORCED_CYCLE_MS, s. .cpp).
// TODO(FR-SNS-04/05, Folgeschritt): I2C-Recovery und Plausibilitaetspruefung,
// analog zum offenen Punkt bei imu_driver.
BaroSample bmp280Read();

}  // namespace drivers
