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

*Nächste Entscheidungen ab hier eintragen (nach Freigabe).*
