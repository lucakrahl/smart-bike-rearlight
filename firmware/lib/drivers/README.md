# lib/drivers — Hardware-Treiber

Kapselt die hardwareabhängigen Teile (Arduino/Espressif-APIs, Bibliotheken):

- `led_output` — PWM-Ausgabe rote/gelbe LEDs (`ledcAttach`/`ledcWrite`, CON-02).
- `sensors` — MPU6050, BMP280 (I²C, mit Timeout/Recovery FR-SNS-03/04), L86 (UART2).
- `rf_input` — RCSwitch auf GPIO4 (FR-RF-01).
- `telemetry_ble` — BLE-Notify (FR-TEL), unidirektional (FR-SYS-04).
- `nvs_config` — `Preferences`-Wrapper (FR-CFG).

**Regel:** Treiber liefern/nehmen einfache Werte und reichen sie an die
`logic`-Module weiter. So bleibt die Logik host-testbar. Treiber selbst werden
On-Target validiert (Bible Kap. 9).
