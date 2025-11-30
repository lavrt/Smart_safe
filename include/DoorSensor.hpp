#pragma once
#include <Arduino.h>

class DoorSensor {
public:
    // doorOpenLevel — уровень на пине, который считаем "дверь ОТКРЫТА"
    DoorSensor(int pin, bool doorOpenLevel = HIGH);

    void begin();           // настроить пин
    bool isOpen() const;    // читаем текущее состояние
    String getStatusText() const; // текст для Telegram

private:
    int _pin;
    bool _doorOpenLevel;
};
