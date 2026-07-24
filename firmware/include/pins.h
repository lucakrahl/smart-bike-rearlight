// pins.h — GPIO-Belegung (Bible Kap. 4.2, [gesichert])
// Struktureller/sicherheitsrelevanter Wert -> fest im Code (FR-CFG-01).
#pragma once

// I2C-Bus (BMP280 0x76 + MPU6050 0x68)
constexpr int PIN_I2C_SDA   = 21;
constexpr int PIN_I2C_SCL   = 22;

// UART2 -> Quectel L86 GNSS (9600 Bd)
constexpr int PIN_GNSS_RX   = 16;   // <- L86 TXD1
constexpr int PIN_GNSS_TX   = 17;   // -> L86 RXD1

// 433-MHz-Empfaenger SRX882S (kein Strapping-Pin)
constexpr int PIN_RF_DATA   = 4;

// LED-Kanaele ueber IRLZ44N (Gate 100R, Pull-Down 10k). PWM 5 kHz (CON-02).
constexpr int PIN_BLINK_LEFT  = 25;   // gelbe LED links  (Q1)
constexpr int PIN_BRAKE_LIGHT = 26;   // rote LED  Schluss-/Bremslicht (Q2)
constexpr int PIN_BLINK_RIGHT = 27;   // gelbe LED rechts (Q3)

// Hinweis: MPU6050-INT ungenutzt (GPIO4 durch RF belegt) -> Polling.
