#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include "LockController.hpp"
#include "AccessManager.hpp"
#include "DoorSensor.hpp"
#include "Buzzer.hpp"

class TelegramLockBot {
public:
    TelegramLockBot(
        UniversalTelegramBot& bot,
        LockController& lock,
        DoorSensor& door,
        AccessManager& accessManager,
        Buzzer& buzzer,
        unsigned long updateIntervalMs
    );

    void begin();
    void update();
    void notifyAdmins(const String& message);

private:
    UniversalTelegramBot& _bot;
    LockController& _lock;
    DoorSensor& _door;
    AccessManager& _accessManager;
    Buzzer& _buzzer;
    unsigned long _updateIntervalMs;
    unsigned long _lastUpdateMs;

    void handleNewMessages(int numNewMessages);
    void sendHelp(const String& chat_id, Role role);

    String roleToText(Role role) const;
    String buildStatusText() const;
};
