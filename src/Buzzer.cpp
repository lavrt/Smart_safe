#include "Buzzer.hpp"

Buzzer::Buzzer(int pin, bool activeLevel)
    : _pin(pin),
      _activeLevel(activeLevel),
      _isBeeping(false),
      _beepEndTime(0) {}

void Buzzer::begin() {
    pinMode(_pin, OUTPUT);
    setOutput(false);
}

void Buzzer::setOutput(bool on) {
    digitalWrite(_pin, on ? _activeLevel : !_activeLevel);
}

void Buzzer::beep(unsigned long durationMs) {
    _isBeeping = true;
    _beepEndTime = millis() + durationMs;
    setOutput(true);
}

void Buzzer::update() {
    if (_isBeeping && (long)(millis() - _beepEndTime) >= 0) {
        _isBeeping = false;
        setOutput(false);
    }
}

void Buzzer::stop() {
    _isBeeping = false;
    setOutput(false);
}
