#include "FingerprintAuth.hpp"

FingerprintAuth::FingerprintAuth(HardwareSerial& serial,
                                 int rxPin,
                                 int txPin,
                                 LockController& lock,
                                 Buzzer& buzzer,
                                 UniversalTelegramBot* bot,
                                 const char* notifyChat)
  : _serial(serial),
    _rxPin(rxPin),
    _txPin(txPin),
    _lock(lock),
    _buzzer(buzzer),
    _bot(bot),
    _notifyChatId(notifyChat),
    _finger(&_serial),
    _lastCheckMs(0),
    _lastMatchedId(0),
    _lastMatchMs(0)
{
}

void FingerprintAuth::begin() {
    _serial.begin(57600, SERIAL_8N1, _rxPin, _txPin);
    delay(100);

    _finger.begin(57600);
    delay(5);

    Serial.println("[FP] Инициализация сканера отпечатков...");

    if (_finger.verifyPassword()) {
        Serial.println("[FP] Сканер найден и отвечает.");
    } else {
        Serial.println("[FP] ❌ Не удалось связаться со сканером. Проверь питание и TX/RX.");
        return;
    }

    if (_finger.getTemplateCount() == FINGERPRINT_OK) {
        Serial.print("[FP] Количество записанных отпечатков: ");
        Serial.println(_finger.templateCount);
    } else {
        Serial.println("[FP] Не удалось получить количество шаблонов.");
    }
}

void FingerprintAuth::handleMatch(uint16_t fingerId) {
    unsigned long now = millis();

    if (fingerId == _lastMatchedId && (now - _lastMatchMs) < _sameFingerCooldownMs) {
        Serial.println("[FP] Тот же палец слишком быстро, игнорируем повтор.");
        return;
    }

    _lastMatchedId = fingerId;
    _lastMatchMs   = now;

    Serial.print("[FP] Найден отпечаток с ID: ");
    Serial.print(fingerId);
    Serial.print(", confidence = ");
    Serial.println(_finger.confidence);

    if (!_lock.isOpen()) {
        _lock.open();
        _buzzer.beep(150);
        Serial.println("[FP] Замок открыт по отпечатку.");

        if (_bot && _notifyChatId) {
            String label = labelForFinger(fingerId);

            String msg = "🔓 Замок открыт по отпечатку #";
            msg += fingerId;
            if (label.length() > 0) {
                msg += " (";
                msg += label;
                msg += ")";
            }
            msg += ".\nУверенность: ";
            msg += _finger.confidence;

            _bot->sendMessage(_notifyChatId, msg, "");
        }
    } else {
        Serial.println("[FP] Замок уже открыт, повторное открытие не требуется.");
    }
}

void FingerprintAuth::handleNoMatch() {
    Serial.println("[FP] Отпечаток не найден в базе.");
    _buzzer.beep(40);
}

void FingerprintAuth::update() {
    unsigned long now = millis();
    if (now - _lastCheckMs < _checkIntervalMs) {
        return;
    }
    _lastCheckMs = now;

    uint8_t p = _finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
        return;
    }
    if (p != FINGERPRINT_OK) {
        Serial.print("[FP] Ошибка getImage(): ");
        Serial.println(p);
        return;
    }

    p = _finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        Serial.print("[FP] Ошибка image2Tz(): ");
        Serial.println(p);
        return;
    }

    p = _finger.fingerFastSearch();
    if (p == FINGERPRINT_OK) {
        handleMatch(_finger.fingerID);
    } else if (p == FINGERPRINT_NOTFOUND) {
        handleNoMatch();
    } else {
        Serial.print("[FP] Ошибка fingerFastSearch(): ");
        Serial.println(p);
    }
}

bool FingerprintAuth::enrollSimple(uint16_t id, const String& chatId, const String& label) {
        if (!_bot) {
        Serial.println("[FP] enrollSimple: bot == nullptr, не можем слать сообщения.");
        return false;
    }

    _buzzer.stop();

    Serial.print("[FP] Начинаем регистрацию отпечатка в слот #");
    Serial.println(id);

    _bot->sendMessage(chatId,
                      "📝 Регистрация отпечатка в слот #" + String(id) +
                      ".\nШаг 1: приложите палец к датчику.",
                      "");

    unsigned long start = millis();
    uint8_t p;

    while (true) {
        p = _finger.getImage();
        if (p == FINGERPRINT_OK) break;
        if (p != FINGERPRINT_NOFINGER && p != FINGERPRINT_PACKETRECIEVEERR) {
            Serial.print("[FP] getImage(1) error: ");
            Serial.println(p);
            _bot->sendMessage(chatId,
                              "❌ Ошибка при чтении первого образца (getImage). Код: " + String(p),
                              "");
            return false;
        }
        if (millis() - start > 30000) {
            _bot->sendMessage(chatId,
                              "⏰ Таймаут ожидания первого пальца. Попробуйте ещё раз.",
                              "");
            return false;
        }
        delay(200);
    }

    p = _finger.image2Tz(1);
    if (p != FINGERPRINT_OK) {
        Serial.print("[FP] image2Tz(1) error: ");
        Serial.println(p);
        _bot->sendMessage(chatId,
                          "❌ Ошибка преобразования первого образца (image2Tz1). Код: " + String(p),
                          "");
        return false;
    }

    _bot->sendMessage(chatId,
                      "👌 Первый образец снят.\nУберите палец с датчика.",
                      "");

    start = millis();
    while (true) {
        p = _finger.getImage();
        if (p == FINGERPRINT_NOFINGER) break;
        if (millis() - start > 10000) {
            _bot->sendMessage(chatId,
                              "⏰ Таймаут: палец не убрали с датчика.",
                              "");
            return false;
        }
        delay(200);
    }

    _bot->sendMessage(chatId,
                      "Шаг 2: снова приложите тот же палец.",
                      "");

    start = millis();
    while (true) {
        p = _finger.getImage();
        if (p == FINGERPRINT_OK) break;
        if (p != FINGERPRINT_NOFINGER && p != FINGERPRINT_PACKETRECIEVEERR) {
            Serial.print("[FP] getImage(2) error: ");
            Serial.println(p);
            _bot->sendMessage(chatId,
                              "❌ Ошибка при чтении второго образца (getImage). Код: " + String(p),
                              "");
            return false;
        }
        if (millis() - start > 30000) {
            _bot->sendMessage(chatId,
                              "⏰ Таймаут ожидания второго пальца. Попробуйте ещё раз.",
                              "");
            return false;
        }
        delay(200);
    }

    p = _finger.image2Tz(2);
    if (p != FINGERPRINT_OK) {
        Serial.print("[FP] image2Tz(2) error: ");
        Serial.println(p);
        _bot->sendMessage(chatId,
                          "❌ Ошибка преобразования второго образца (image2Tz2). Код: " + String(p),
                          "");
        return false;
    }

    p = _finger.createModel();
    if (p != FINGERPRINT_OK) {
        Serial.print("[FP] createModel error: ");
        Serial.println(p);
        _bot->sendMessage(chatId,
                          "❌ Ошибка создания модели (createModel). Код: " + String(p),
                          "");
        return false;
    }

    p = _finger.storeModel(id);
    if (p != FINGERPRINT_OK) {
        Serial.print("[FP] storeModel error: ");
        Serial.println(p);
        _bot->sendMessage(chatId,
                          "❌ Ошибка сохранения в слот #" + String(id) +
                          " (storeModel). Код: " + String(p),
                          "");
        return false;
    }

    _bot->sendMessage(chatId,
                      "✅ Отпечаток успешно записан в слот #" + String(id) + ".",
                      "");

    if (label.length() > 0) {
        setUserLabel(id, label);
        Serial.print("[FP] Записали метку для слота #");
        Serial.print(id);
        Serial.print(": ");
        Serial.println(label);
    }

    Serial.print("[FP] Отпечаток успешно записан в слот #");
    Serial.println(id);

    return true;
}

String FingerprintAuth::labelForFinger(uint16_t fingerId) const {
    for (size_t i = 0; i < _labelCount; i++) {
        if (_labels[i].id == fingerId) {
            return _labels[i].name;
        }
    }
    return String();
}

void FingerprintAuth::setUserLabel(uint16_t fingerId, const String& name) {
    for (size_t i = 0; i < _labelCount; i++) {
        if (_labels[i].id == fingerId) {
            _labels[i].name = name;
            return;
        }
    }

    if (_labelCount < MAX_LABELS) {
        _labels[_labelCount].id = fingerId;
        _labels[_labelCount].name = name;
        _labelCount++;
    } else {
        Serial.println("[FP] Внимание: таблица имён отпечатков заполнена, новый не сохранён.");
    }
}
