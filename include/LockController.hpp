#pragma once
#include <Arduino.h>

class LockController {
public:
    LockController(int pin, unsigned long openDurationMs);

    void begin();   // Инициализация пина, закрываем замок
    void open();    // Открыть замок на заданное время
    void close();   // Принудительно закрыть замок
    void update();  // Вызывать в loop() для авто-закрытия

    bool isOpen() const;
    String getStatusText() const;

private:
    int _pin;
    unsigned long _openDurationMs;

    bool _isOpen;
    unsigned long _openedAt;
};
