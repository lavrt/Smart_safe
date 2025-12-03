#pragma once
#include <Arduino.h>

class LockController {
public:
    LockController(int pin, unsigned long openDurationMs);

    void begin();
    void open();
    void close();
    void update();

    bool isOpen() const;
    String getStatusText() const;

private:
    int _pin;
    unsigned long _openDurationMs;

    bool _isOpen;
    unsigned long _openedAt;
};
