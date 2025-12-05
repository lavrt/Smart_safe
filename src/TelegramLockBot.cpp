#include "TelegramLockBot.hpp"
#include "FingerprintAuth.hpp"
#include "SafeCamera.hpp"
#include "Config.hpp"

extern SafeCamera safeCamera;
extern FingerprintAuth fingerprintAuth;

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
    help += "/photo  - получение фотографии\n";

    if (_accessManager.canOpen(role)) {
        help += "/open   - открыть замок\n";
    }

    _bot.sendMessage(chat_id, help, "Markdown");
}

void TelegramLockBot::notifyAdmins(const String& message) {
    for (size_t i = 0; i < NUM_ADMIN_CHATS; ++i) {
        String adminChatId = ADMIN_CHAT_IDS[i];
        _bot.sendMessage(adminChatId, message, "");
    }
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
                _buzzer.beep(2);

                String msg = "🔓 Замок открыт.";
                _bot.sendMessage(chat_id, msg, "");
            } else {
                _bot.sendMessage(chat_id, "Замок уже открыт.", "");
            }
        } else if (text.startsWith("/enroll")) {
            if (!_accessManager.canConfigure(role)) {
                _bot.sendMessage(chat_id,
                                "🚫 Только администратор может регистрировать отпечатки.",
                                "");
                continue;
            }

            if (_lock.isOpen()) {
                _bot.sendMessage(chat_id,
                                "❌ Нельзя регистрировать отпечаток, пока сейф открыт.\n"
                                "Сначала закройте сейф, затем повторите команду /enroll.",
                                "");
                continue;
            }

            int firstSpace = text.indexOf(' ');
            if (firstSpace < 0 || firstSpace == (int)text.length() - 1) {
                _bot.sendMessage(chat_id,
                                "Использование: /enroll <id> [имя]\nНапример: /enroll 5 Alex",
                                "");
                continue;
            }

            String rest = text.substring(firstSpace + 1);
            rest.trim();

            int secondSpace = rest.indexOf(' ');
            String idStr;
            String label;

            if (secondSpace < 0) {
                idStr = rest;
                label = "";
            } else {
                idStr = rest.substring(0, secondSpace);
                label = rest.substring(secondSpace + 1);
                label.trim();
            }

            int id = idStr.toInt();
            if (id <= 0) {
                _bot.sendMessage(chat_id,
                                "ID должен быть положительным числом. Пример: /enroll 3",
                                "");
                continue;
            }

            _bot.sendMessage(chat_id,
                            "Запускаю регистрацию отпечатка в слот #" + String(id) +
                            ". Это может занять до 1 минуты.\n"
                            "Следуйте подсказкам.",
                            "");

            bool ok = fingerprintAuth.enrollSimple((uint16_t)id, chat_id, label);

            if (!ok) {
                _bot.sendMessage(chat_id,
                                "Регистрация отпечатка в слот #" + String(id) +
                                " завершилась с ошибкой.",
                                "");
            }
        } else if (text == "/photo") {
            if (!_accessManager.canConfigure(role)) {
                _bot.sendMessage(chat_id,
                                "🚫 Недостаточно прав для запроса фото.",
                                "");
                continue;
            }

            _bot.sendMessage(chat_id,
                            "📸 Делаю фото, подождите...",
                            "");

            bool ok = safeCamera.sendPhoto(chat_id);
            if (!ok) {
                _bot.sendMessage(chat_id,
                                "❌ Не удалось сделать или отправить фото.",
                                "");
            }
        } else {
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
