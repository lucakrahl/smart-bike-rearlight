// imu_driver.cpp — Umsetzung MPU6050-Treiber. Hardwareabhaengig (Arduino.h).
#include "imu_driver.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "pins.h"
#include "config.h"

namespace drivers {

namespace {
Adafruit_MPU6050 mpu;
bool ready = false;
}  // namespace

bool imuBegin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setTimeOut(I2C_TIMEOUT_MS);  // FR-SNS-03: nicht-blockierend begrenzt

  ready = mpu.begin(MPU6050_I2C_ADDR, &Wire);
  return ready;
}

bool imuIsReady() {
  return ready;
}

ImuSample imuRead() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  return ImuSample{
    accel.acceleration.x,
    accel.acceleration.y,
    accel.acceleration.z,
    gyro.gyro.x,
  };
}

}  // namespace drivers
