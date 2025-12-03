#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include <Adafruit_Fingerprint.h>
#include <UniversalTelegramBot.h>

#include "LockController.hpp"
#include "Buzzer.hpp"

class FingerprintAuth {
public:
    FingerprintAuth(HardwareSerial& serial,
                    int rxPin,
                    int txPin,
                    LockController& lock,
                    Buzzer& buzzer,
                    UniversalTelegramBot* bot = nullptr,
                    const char* notifyChat = nullptr);

    void begin();
    void update();
    bool enrollSimple(uint16_t id, const String& chatId, const String& label);

private:
    HardwareSerial& _serial;
    int _rxPin;
    int _txPin;

    LockController& _lock;
    Buzzer& _buzzer;

    UniversalTelegramBot* _bot;
    const char* _notifyChatId;

    Adafruit_Fingerprint _finger;

    unsigned long _lastCheckMs;
    const unsigned long _checkIntervalMs = 300;

    uint16_t _lastMatchedId;
    unsigned long _lastMatchMs;
    const unsigned long _sameFingerCooldownMs = 2000;

    void handleMatch(uint16_t fingerId);
    void handleNoMatch();

    static const size_t MAX_LABELS = 16;

    struct FingerLabel {
        uint16_t id;
        String name;
    };

    FingerLabel _labels[MAX_LABELS];
    size_t _labelCount = 0;

    String labelForFinger(uint16_t id) const;
    void setUserLabel(uint16_t id, const String& name);
};
