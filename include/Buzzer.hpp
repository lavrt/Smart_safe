#pragma once
#include <Arduino.h>

class Buzzer {
public:
    Buzzer(int pin, bool activeLevel = HIGH);

    void begin();
    void beep(unsigned long durationMs = 100); // запустить короткий писк
    void update();                             // вызывать в loop()

private:
    int _pin;
    bool _activeLevel;
    bool _isBeeping;
    unsigned long _beepEndTime;

    void setOutput(bool on);
};
