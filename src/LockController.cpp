#include "LockController.hpp"

LockController::LockController(int pin, unsigned long openDurationMs)
    : _pin(pin),
      _openDurationMs(openDurationMs),
      _isOpen(false),
      _openedAt(0) {}

void LockController::begin() {
    pinMode(_pin, OUTPUT);
    close();  // на старте закрываем
}

void LockController::open() {
    digitalWrite(_pin, HIGH);   // подстрой под своё реле (HIGH/LOW)
    _isOpen = true;
    _openedAt = millis();
    Serial.println("[LOCK] Замок открыт");
}

void LockController::close() {
    digitalWrite(_pin, LOW);
    _isOpen = false;
    Serial.println("[LOCK] Замок закрыт");
}

void LockController::update() {
    if (_isOpen && (millis() - _openedAt >= _openDurationMs)) {
        close();
    }
}

bool LockController::isOpen() const {
    return _isOpen;
}

String LockController::getStatusText() const {
    String status = "Статус замка:\n";
    status += _isOpen ? "🔓 ОТКРЫТ\n" : "🔒 ЗАКРЫТ\n";
    status += "Время авто-закрытия: " + String(_openDurationMs / 1000) + " секунд\n";
    return status;
}
