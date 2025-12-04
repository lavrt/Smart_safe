#include "RfidReader.hpp"
#include <Wire.h>

// ВНИМАНИЕ: PN532 требует два аргумента в конструкторе (irq, reset),
// но в I2C-режиме библиотека с ними не работает как с реальными пинами.
// Мы просто передаём фиктивные значения 0 и 0 — физически НИЧЕГО к этим
// пинам ESP32 не подключаем.
#define PN532_FAKE_IRQ   0
#define PN532_FAKE_RESET 0

RfidReader::RfidReader(uint8_t sdaPin, uint8_t sclPin)
    : _sdaPin(sdaPin),
      _sclPin(sclPin),
      _nfc(PN532_FAKE_IRQ, PN532_FAKE_RESET, &Wire)
{
}

void RfidReader::loadDefaultAuthUids() {
    _authCount = 0;

    _authUids[_authCount++] = "E7E56B06";
}

bool RfidReader::begin() {
    // общий I2C с камерой: SDA=26, SCL=27
    Wire.begin(_sdaPin, _sclPin);

    _nfc.begin();

    uint32_t versiondata = _nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("[RFID] Не найден PN532. Проверь питание, I2C-режим и SDA/SCL (26/27).");
        return false;
    }

    Serial.print("[RFID] PN5");
    Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print("[RFID] Chip: ");
    Serial.println((versiondata >> 16) & 0xFF, HEX);
    Serial.print("[RFID] FW ver. ");
    Serial.print((versiondata >> 8) & 0xFF);
    Serial.print('.');
    Serial.println(versiondata & 0xFF);

    // Нормальный режим, слушаем карты
    _nfc.SAMConfig();

    loadDefaultAuthUids();

    Serial.println("[RFID] PN532 инициализирован (I2C, без доп. пинов).");
    return true;
}

bool RfidReader::readCard(String& uidHex) {
    uidHex = "";

    uint8_t uid[7];
    uint8_t uidLength = 0;

    // Таймаут маленький, чтобы не блокировать весь loop (50 мс)
    bool success = _nfc.readPassiveTargetID(
        PN532_MIFARE_ISO14443A,
        uid,
        &uidLength,
        50
    );

    if (!success) {
        // карты нет
        return false;
    }

    // Конвертация UID в строку HEX
    char buf[3];
    for (uint8_t i = 0; i < uidLength; ++i) {
        sprintf(buf, "%02X", uid[i]);
        uidHex += buf;
    }

    Serial.print("[RFID] Найдена карта, UID = ");
    Serial.println(uidHex);

    return true;
}

bool RfidReader::isAuthorized(const String& uidHex) const {
    for (size_t i = 0; i < _authCount; ++i) {
        if (uidHex.equalsIgnoreCase(_authUids[i])) {
            return true;
        }
    }
    return false;
}
