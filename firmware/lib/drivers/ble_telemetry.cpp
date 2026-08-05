// ble_telemetry.cpp — Umsetzung BLE-Notify-Transport (NimBLE-Arduino 2.5.0).
#include "ble_telemetry.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "config.h"
#include "telemetry_frame.h"  // logic::TELEMETRY_FRAME_SIZE, nur fuer die MTU-Warnschwelle

namespace drivers {

namespace {

NimBLECharacteristic* g_pTelemetryChar = nullptr;

// onConnect/onDisconnect/onMTUChange laufen im NimBLE-Host-Task, nicht in
// loop() -- deshalb nur einfache volatile Flags setzen, keine direkten
// Zugriffe auf TelemetryBuffer o.ae. aus BLE-Kontext (FR-SAF-04-Isolation,
// s. main.cpp-Kommentar zu taskTelemetry()).
volatile bool     g_connected = false;
volatile uint16_t g_negotiatedMtu = 23;  // BLE-ATT-Default vor jeder Aushandlung

// Mindest-MTU, damit ein Frame (TELEMETRY_FRAME_SIZE) in eine einzige
// Notification passt (Frame + 3 Byte ATT-Opcode/Handle-Overhead).
constexpr uint16_t kMinMtuForFrame = logic::TELEMETRY_FRAME_SIZE + 3;

class ServerCallbacks : public NimBLEServerCallbacks {
 public:
  void onConnect(NimBLEServer* /*pServer*/, NimBLEConnInfo& connInfo) override {
    g_connected = true;
    g_negotiatedMtu = connInfo.getMTU();
    if (DEBUG_SERIAL) {
      Serial.printf("[BLE] verbunden, initiales MTU=%u\n", g_negotiatedMtu);
    }
  }

  void onDisconnect(NimBLEServer* /*pServer*/, NimBLEConnInfo& /*connInfo*/, int reason) override {
    g_connected = false;
    if (DEBUG_SERIAL) {
      Serial.printf("[BLE] getrennt, reason=%d\n", reason);
    }
    // Automatisches Reconnect-faehig-Bleiben: advertiseOnDisconnect(true)
    // (s. bleBegin()) startet das Advertising bereits intern neu.
  }

  void onMTUChange(uint16_t mtu, NimBLEConnInfo& /*connInfo*/) override {
    g_negotiatedMtu = mtu;
    if (DEBUG_SERIAL) {
      Serial.printf("[BLE] MTU ausgehandelt=%u (Nutzlast<=%u)\n", mtu, mtu > 3 ? mtu - 3 : 0);
      if (mtu < kMinMtuForFrame) {
        Serial.printf(
            "[BLE][WARN] MTU %u < benoetigte %u fuer ein Frame in einer "
            "Notification -- s. open_issues.md\n",
            mtu, kMinMtuForFrame);
      }
    }
  }
};

ServerCallbacks g_serverCallbacks;

}  // namespace

void bleBegin() {
  // Bracket-Prints mit Serial.flush() um init() -- offener Befund
  // (open_issues.md "Brown-Out unter Lastspitzen"): der Brownout-Detektor
  // schlaegt bei marginaler Versorgung waehrend NimBLEDevice::init() selbst
  // zu (RF-Kalibrierungs-Stromspitze des Controllers), VOR jeder Stelle, an
  // der eine Sendeleistungs-Einstellung greifen koennte -- belegt dadurch,
  // dass "nach init()" nie geloggt wird, obwohl "vor init()" es immer wird
  // (Serial.flush() schliesst aus, dass der Print nur im Puffer verloren
  // ging). Bewusst NICHT entfernt: bleibt fuer einen Retest nach der
  // Hardware-Massnahme (Pufferkondensator) nuetzlich.
  if (DEBUG_SERIAL) {
    Serial.println(F("[BLE] vor NimBLEDevice::init()"));
    Serial.flush();
  }
  NimBLEDevice::init(BLE_DEVICE_NAME);
  if (DEBUG_SERIAL) {
    Serial.println(F("[BLE] nach NimBLEDevice::init()"));
    Serial.flush();
  }
  // Reduziert die Sendeleistung fuer Advertising/Notify NACH dem Hochlauf
  // (guenstiger fuer die Energiebilanz, NFR-PWR). Wirkt NICHT auf die
  // Stromspitze innerhalb von init() selbst (s. Kommentar oben) -- behebt
  // den Bootloop also nicht, ist aber unabhaengig davon sinnvoll.
  const bool tx_power_ok = NimBLEDevice::setPower(BLE_TX_POWER_DBM);
  if (DEBUG_SERIAL) {
    Serial.printf("[BLE] TX-Power=%d dBm gesetzt=%d\n", BLE_TX_POWER_DBM, (int)tx_power_ok);
  }
  NimBLEDevice::setMTU(BLE_PREFERRED_MTU);

  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(&g_serverCallbacks);
  // Default ist AUS (NimBLEServer-Quelle) -- ohne diesen Aufruf wuerde nach
  // einem Disconnect kein Advertising mehr laufen und FR-TEL-01 (erneute
  // Verbindung nach Trennung) waere verletzt.
  pServer->advertiseOnDisconnect(true);

  NimBLEService* pService = pServer->createService(BLE_SERVICE_UUID);
  g_pTelemetryChar = pService->createCharacteristic(BLE_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::NOTIFY);

  pServer->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  // NimBLEDevice::init(BLE_DEVICE_NAME) setzt nur den GAP-Geraetenamen
  // intern -- ins Advertising-Paket muss er separat eingetragen werden,
  // sonst ist das Geraet fuer namensbasierte Scans (z.B. nRF Connect)
  // unauffindbar, obwohl es korrekt advertised. Name in die Scan-Response
  // auslagern (eigenes 31-Byte-Budget): das primaere Advertising-Paket ist
  // mit Flags + 128-Bit-Service-UUID (3 + 18 Byte) schon fast voll, der
  // 19-Zeichen-Name wuerde dort ueberlaufen ("Data length exceeded").
  pAdvertising->enableScanResponse(true);
  pAdvertising->setName(BLE_DEVICE_NAME);
  pAdvertising->start();

  if (DEBUG_SERIAL) {
    Serial.println(F("[BLE] Advertising gestartet"));
  }
}

bool bleIsConnected() { return g_connected; }

uint16_t bleGetMtu() { return g_negotiatedMtu; }

bool bleNotify(const uint8_t* data, size_t len) {
  if (!g_connected || g_pTelemetryChar == nullptr) {
    return false;
  }
  return g_pTelemetryChar->notify(data, len);
}

}  // namespace drivers
