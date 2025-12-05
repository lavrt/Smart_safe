#pragma once

#include <Arduino.h>

class Adafruit_ADXL345_Unified;

class MotionSensor {
public:
    MotionSensor();

    bool begin();

    void printAxes();

    bool detectShock(float thresholdG = 1.5f);

    bool loop(float thresholdG = 1.5f);

private:
    Adafruit_ADXL345_Unified* _accel;

    unsigned long _lastCheckMs;
    unsigned long _checkIntervalMs;
};
