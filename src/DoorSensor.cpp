#include "DoorSensor.hpp"

DoorSensor::DoorSensor(int pin, bool doorOpenLevel)
    : _pin(pin),
      _doorOpenLevel(doorOpenLevel) {}

void DoorSensor::begin() {
    // Геркон -> пин + GND, используем подтяжку к VCC
    pinMode(_pin, INPUT_PULLUP);
}

bool DoorSensor::isOpen() const {
    int v = digitalRead(_pin);
    return (v == _doorOpenLevel);
}

String DoorSensor::getStatusText() const {
    String s = "🚪 Дверь: ";
    if (isOpen()) {
        s += "ОТКРЫТА\n";
    } else {
        s += "ЗАКРЫТА\n";
    }
    return s;
}
