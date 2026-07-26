// gnss_driver.cpp — Umsetzung GNSS-Treiber. Hardwareabhaengig (Arduino.h).
#include "gnss_driver.h"
#include <Arduino.h>
#include <TinyGPSPlus.h>
#include "pins.h"
#include "config.h"

namespace drivers {

namespace {
TinyGPSPlus gps;
}  // namespace

bool gnssBegin() {
  // RX-Puffer VOR begin() vergroessern (Overflow-Reserve, GNSS_UART_RX_BUF):
  // Serial2 uebernimmt die Groesse erst beim naechsten begin().
  Serial2.setRxBufferSize(GNSS_UART_RX_BUF);
  Serial2.begin(GNSS_BAUD, SERIAL_8N1, PIN_GNSS_RX, PIN_GNSS_TX);
  return true;  // UART-Start liefert keinen Fehlercode; "bereit" != "hat Fix"
}

void gnssPump() {
  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
  }
}

GnssData gnssRead() {
  return GnssData{
    gps.location.lat(),
    gps.location.lng(),
    (float)gps.speed.kmph(),
    (float)gps.course.deg(),
    (float)gps.altitude.meters(),
    (float)gps.hdop.value() / 100.0f,  // TinyGPSDecimal: value() in Hundertsteln
    (uint8_t)gps.satellites.value(),
    (uint16_t)gps.date.year(),
    (uint8_t)gps.date.month(),
    (uint8_t)gps.date.day(),
    (uint8_t)gps.time.hour(),
    (uint8_t)gps.time.minute(),
    (uint8_t)gps.time.second(),
    gps.location.isValid(),
    gps.location.age(),
    gps.charsProcessed(),
  };
}

}  // namespace drivers
