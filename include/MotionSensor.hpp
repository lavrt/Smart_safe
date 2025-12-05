#pragma once

#include <Arduino.h>

// ВАЖНО:
// Здесь мы НЕ включаем Adafruit_ADXL345_U.h и Adafruit_Sensor.h,
// чтобы не тащить sensor_t в main.cpp и не конфликтовать с esp_camera.

// Вперёд объявляем класс, который реализован в Adafruit_ADXL345_U.h
class Adafruit_ADXL345_Unified;

// Лёгкий интерфейс, не зависящий от типов из Adafruit_Sensor.h
class MotionSensor {
public:
    MotionSensor();

    // Вызывать в setup() (после Wire.begin(26,27), который уже делает RfidReader)
    bool begin();

    // Разовая печать текущих XYZ в Serial (для отладки)
    void printAxes();

    // Проверка, есть ли "удар/движение" сильнее thresholdG (в g)
    bool detectShock(float thresholdG = 1.5f);

    // Периодическая проверка, вызывается из loop()
    bool loop(float thresholdG = 1.5f);

private:
    Adafruit_ADXL345_Unified* _accel;  // будет создан в .cpp через new

    unsigned long _lastCheckMs;
    unsigned long _checkIntervalMs;
};
