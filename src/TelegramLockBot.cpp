#include "TelegramLockBot.hpp"

TelegramLockBot::TelegramLockBot(
    UniversalTelegramBot& bot,
    LockController& lock,
    DoorSensor& door,
    AccessManager& accessManager,
    Buzzer& buzzer,
    unsigned long updateIntervalMs
)
    : _bot(bot),
      _lock(lock),
      _door(door),
      _accessManager(accessManager),
      _buzzer(buzzer),
      _updateIntervalMs(updateIntervalMs),
      _lastUpdateMs(0) {}

void TelegramLockBot::begin() {
    Serial.println("[BOT] TelegramLockBot готов к работе");
}

String TelegramLockBot::roleToText(Role role) const {
    switch (role) {
        case Role::Admin:  return "Admin";
        case Role::User:   return "User";
        case Role::Viewer: return "Viewer";
        default:           return "Unknown";
    }
}

String TelegramLockBot::buildStatusText() const {
    String s;
    s += _lock.getStatusText();
    s += _door.getStatusText();
    return s;
}

void TelegramLockBot::sendHelp(const String& chat_id, Role role) {
    String help = "Доступные команды:\n";
    help += "/start  - информация о боте и ваша роль\n";
    help += "/status - статус замка и двери\n";
    help += "/help   - это сообщение\n";

    if (_accessManager.canOpen(role)) {
        help += "/open   - открыть замок\n";
    }

    _bot.sendMessage(chat_id, help, "Markdown");
}

void TelegramLockBot::handleNewMessages(int numNewMessages) {
    Serial.println("[BOT] Новых сообщений: " + String(numNewMessages));

    for (int i = 0; i < numNewMessages; i++) {
        String chat_id   = _bot.messages[i].chat_id;
        String text      = _bot.messages[i].text;
        String from_name = _bot.messages[i].from_name;

        Role role = _accessManager.getRole(chat_id);

        Serial.println("Сообщение от " + from_name + " (" + chat_id + "): " + text +
                       " | роль: " + roleToText(role));

        if (role == Role::Unknown) {
            _bot.sendMessage(chat_id,
                             "🚫 У вас нет прав управлять этим замком.",
                             "");
            continue;
        }

        if (text == "/start") {
            String msg = "Привет, " + from_name + "!\n"
                         "Это бот для управления умным замком.\n\n";
            msg += "Ваша роль: *" + roleToText(role) + "*\n\n";
            msg += buildStatusText();
            msg += "\nНапиши /help для списка команд.";
            _bot.sendMessage(chat_id, msg, "Markdown");
        }
        else if (text == "/help") {
            sendHelp(chat_id, role);
        }
        else if (text == "/status") {
            if (_accessManager.canViewStatus(role)) {
                _bot.sendMessage(chat_id, buildStatusText(), "Markdown");
            } else {
                _bot.sendMessage(chat_id,
                                 "🚫 У вас нет прав смотреть статус.",
                                 "");
            }
        }
        else if (text == "/open") {
            if (!_accessManager.canOpen(role)) {
                _bot.sendMessage(chat_id,
                                 "🚫 У вас нет прав открывать замок.",
                                 "");
                continue;
            }

            if (!_lock.isOpen()) {
                _lock.open();
                _buzzer.beep(120);  // короткий писк при успешном открытии

                String msg = "🔓 Замок открыт.";
                _bot.sendMessage(chat_id, msg, "");
            } else {
                _bot.sendMessage(chat_id, "Замок уже открыт.", "");
            }
        }
        else {
            _bot.sendMessage(chat_id,
                             "Неизвестная команда. Напиши /help.",
                             "");
        }
    }
}

void TelegramLockBot::update() {
    if (millis() - _lastUpdateMs < _updateIntervalMs) {
        return;
    }
    _lastUpdateMs = millis();

    int numNewMessages = _bot.getUpdates(_bot.last_message_received + 1);

    if (numNewMessages < 0) {
        Serial.println("[BOT] getUpdates FAILED");
        return;
    }

    while (numNewMessages) {
        handleNewMessages(numNewMessages);
        numNewMessages = _bot.getUpdates(_bot.last_message_received + 1);
        if (numNewMessages < 0) {
            Serial.println("[BOT] getUpdates FAILED внутри цикла");
            break;
        }
    }
}
