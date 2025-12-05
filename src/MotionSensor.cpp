#include "MotionSensor.hpp"

#include <Wire.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_Sensor.h>

MotionSensor::MotionSensor()
    : _accel(nullptr),
      _lastCheckMs(0),
      _checkIntervalMs(100)
{
}

bool MotionSensor::begin() {
    if (_accel == nullptr) {
        _accel = new Adafruit_ADXL345_Unified(12345);
    }

    if (!_accel->begin()) {
        Serial.println("[ACC] Не найден ADXL345. Проверьте подключение (I2C на 26/27).");
        return false;
    }

    Serial.println("[ACC] ADXL345 найден и инициализирован.");

    _accel->setRange(ADXL345_RANGE_4_G);

    return true;
}

void MotionSensor::printAxes() {
    if (_accel == nullptr) return;

    sensors_event_t event;
    _accel->getEvent(&event);

    Serial.print("[ACC] X: ");
    Serial.print(event.acceleration.x);
    Serial.print(" m/s^2, Y: ");
    Serial.print(event.acceleration.y);
    Serial.print(" m/s^2, Z: ");
    Serial.print(event.acceleration.z);
    Serial.println(" m/s^2");
}

bool MotionSensor::detectShock(float thresholdG) {
    if (_accel == nullptr) return false;

    sensors_event_t event;
    _accel->getEvent(&event);

    float ax = event.acceleration.x;
    float ay = event.acceleration.y;
    float az = event.acceleration.z;

    float a2 = ax*ax + ay*ay + az*az;
    float a  = sqrtf(a2);

    float g  = a / 9.81f;

    return (g > thresholdG);
}

bool MotionSensor::loop(float thresholdG) {
    if (_accel == nullptr) return false;

    unsigned long now = millis();
    if (now - _lastCheckMs < _checkIntervalMs) {
        return false;
    }
    _lastCheckMs = now;

    if (detectShock(thresholdG)) {
        Serial.println("[ACC] Обнаружено сильное движение/удар!");
        return true;
    }
    return false;
}
