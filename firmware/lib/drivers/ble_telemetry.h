// ble_telemetry.h — BLE-Notify-Transport fuer den Telemetrie-Frame
// (FR-TEL-01, FR-SYS-04). Treiber (kapselt NimBLE-Arduino), main.cpp
// verdrahtet nur. Ein Service, eine Notify-Characteristic, keine
// Applikations-Steuerung ueber BLE (FR-SYS-04: nur ESP32 -> App).
#pragma once
#include <cstddef>
#include <cstdint>

namespace drivers {

// Initialisiert NimBLE, Server/Service/Characteristic und startet das
// Advertising. Einmalig aus setup() aufrufen.
void bleBegin();

// true, solange genau ein Client verbunden ist (CONFIG_BT_NIMBLE_MAX_
// CONNECTIONS=1, s. platformio.ini).
bool bleIsConnected();

// Sendet eine Notification mit den gegebenen Bytes an den verbundenen
// Client. Nicht-blockierend (NFR-RT-04): liefert sofort false zurueck, wenn
// nicht verbunden oder die interne Sende-Queue des BLE-Stacks voll ist --
// kein Retry, kein Warten.
bool bleNotify(const uint8_t* data, size_t len);

}  // namespace drivers
