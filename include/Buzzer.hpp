#pragma once
#include <Arduino.h>

class Buzzer {
public:
    Buzzer(int pin, bool activeLevel = HIGH);

    void begin();
    void beep(unsigned long durationMs = 100);
    void update();
    void stop();

private:
    int _pin;
    bool _activeLevel;

    bool _isBeeping;
    unsigned long _beepEndTime;

    void setOutput(bool on);
};
