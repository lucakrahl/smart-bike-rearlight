# Decision Log

> **Freigabepflichtig** (siehe `CLAUDE.md`): Neue Entscheidungen hier nur nach
> ausdrücklicher Freigabe des Nutzers ergänzen. Format je Eintrag: Entscheidung
> · Begründung · verworfene Alternative(n). Kanonische Fassung: Project Bible Kap. 10.

Auszug der zentralen Architekturentscheidungen (Vollständige Liste: Bible Kap. 10):

| Entscheidung | Begründung | Verworfene Alternative |
|---|---|---|
| Rechenlast in die Web-App (Variante 2) | ESP32 deterministisch, geringer RAM/CPU | Firmware rechnet alle Kennzahlen |
| Zustandsmodell als 4 parallele Regionen | additive Zustandsanzahl, testbar | flache kombinierte FSM |
| Kooperativer `millis()`-Scheduler | deterministisch, testbar, dokumentierbar | eigene FreeRTOS-App-Tasks im MVP |
| Rote LED = Schluss- + Bremslicht | § 67-konform + Bremslicht-Mehrwert | binäres Bremslicht |
| Notbrems-Blinken (ESS) experimentell/deaktiviert | Sicherheit vs. § 67 Abs. 4 | aktiv ausliefern (unzulässig) |
| Warnblinker per Langdruck (≥ 5 s) | ASK-Fernbedienung ohne Kombisignal | gleichzeitiges Tastendrücken |
| Trennung Logik ↔ Hardware | Host-Unit-Tests möglich | Logik an Treiber gekoppelt |
| No-OTA-Partition + NVS-Konfig | BLE-Firmware passt; NVS reicht | OTA-Schema / LittleFS |
| Build-Umgebung: PlatformIO mit pioarduino-Plattform (Arduino-ESP32-Core 3.3.x) | Core 3.x nötig für `ledcAttach` (CON-02); offizielle espressif32-Plattform liefert nur Core 2.0.17 | Arduino IDE / offizielle espressif32-Plattform (Core 2.x) |
| Zentrale I²C-Bus-Init (Anwendungsebene) | Modularität, Unabhängigkeit optionaler Sensoren, keine Reihenfolge-Abhängigkeit | `Wire.begin()` im Sensor-Treiber |
| BMP280 FORCED-Mode (Weather Monitoring ×1/×1/IIR aus) | geringes Rauschen @1 Hz, geringere Stromaufnahme, Bosch-Empfehlung | NORMAL-Dauerbetrieb ×16 |
| Keine feste Temperatur-Korrektur in der Firmware | Offset noch nicht abschließend charakterisiert, umgebungs-/lastabhängig; Rohdaten-Integrität; verfälscht sonst Druckkompensation | fester Offset im Code |
| Bremslicht nur bei Verzögerung in Fahrtrichtung (`MOTION_BRAKE_SIGN`, feldkalibriert) | Sprint/Beschleunigung darf kein Bremslicht auslösen; reale Einbaulage | \|a\| via `fabs()` (richtungsblind) |
| Getrennte IMU-Health-/Lifecycle-Init-Signale | `lifecycle_fsm.degraded` bleibt INIT-Ergebnis (FR-STA-06: kein Rücksprung nach RUN); Laufzeit-Sensorausfall gated nur R2 über ein eigenes Signal (`imu_health`) | `degraded` in `lifecycle_fsm` um Laufzeit-Gesundheit erweitern |
| WHO_AM_I-Liveness-Check statt `getEvent()`-Rückgabewert | Adafruit-Wrapper reicht I²C-Fehler nicht zuverlässig durch | `getEvent()`-Rückgabewert vertrauen |
| IMU-FSR ±16 g (statt Library-Default) | Fahrrad-Stöße bis ~20 g möglich, engeres FSR sättigt fälschlich und triggert die Plausibilitätsprüfung | Default-Bereich (±8 g) belassen |
| Sprung-Plausibilität + Eskalations-Vertrauen (N konsekutive plausible Zyklen) | einzelnes Müll-aber-in-Range-Sample darf keine Bremseskalation auslösen (Fehlerinjektionstest SDA-Kurzschluss) | einzelnem plausiblen Sample sofort vertrauen |
| I²C-Bus-Recovery über rohe `gpio_*`-Calls statt `pinMode()`/`digitalWrite()` | PeriMan blockiert bei hängendem I²C-Bus selbst (`i2c_del_master_bus()`-Fehlschlag verhindert Pin-Freigabe, verifiziert im Core-Quelltext) | Arduino-`pinMode()`/`digitalWrite()` |
| Board-Tausch auf Espressif ESP32-DevKitC-32E (WROOM-32E) | BLE-Brownout-Bootloop bei `NimBLEDevice::init()` auf zehn systematischen Tests eingegrenzt; Root Cause = Regler des Altboards liefert die RF-Kalibrierungs-Transiente nicht (Details `docs/ble_brownout_fallstudie.md`); neues Board Referenz-Design, robusterer Regler, pin-kompatibel (38-Pin-DevKitC-Layout), kein Neuverkabeln nötig | Beim Altboard bleiben (Ursache nicht behebbar); WiFi statt BLE |
| Entkopplungskondensatoren (1000 µF an 3V3, 1000 µF an Vin) trotz Wirkungslosigkeit gegen den BLE-Brownout beibehalten | verbessern allgemeine Versorgungsstabilität/Transienten-Robustheit (EMV, Lastspitzen); robustes Stromversorgungsdesign; weniger Rework-Risiko durch erneutes Auslöten | Kondensatoren wieder entfernen |
| WiFi statt BLE verworfen | teilt sich denselben 2,4-GHz-Funk/dieselbe RF-Kalibrierung, zieht mehr Strom → gleiches/stärkeres Brownout-Risiko; widerspricht BLE-App-Architektur & NFR-PWR-01 (WiFi aus) | WiFi als Telemetrie-Transport |
| Bench-Validierung der Bremslicht-Logik über NFR-TST-02-Einspeisung (synthetische Verzögerungsprofile, 100-Hz-Serial-Log) | reproduzierbar/präzise, löst die schnellen Effekte (300-ms-Halten, < 50-ms-Anstieg) auf | nur physische Verzögerung / BLE-Telemetrie @ 10 Hz (zu grob) |

*Nächste Entscheidungen ab hier eintragen (nach Freigabe).*
