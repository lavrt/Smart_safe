#pragma once
#include <Arduino.h>

class DoorSensor {
public:
    DoorSensor(int pin, bool doorOpenLevel = HIGH);

    void begin();
    bool isOpen() const;
    String getStatusText() const;

private:
    int _pin;
    bool _doorOpenLevel;
};
