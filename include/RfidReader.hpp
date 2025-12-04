#pragma once

#include <Arduino.h>

// жёстко говорим библиотеке, что хотим I2C
#define PN532_I2C
#include <Adafruit_PN532.h>

class RfidReader {
public:
    // Используем общий I2C с камерой: SDA=26, SCL=27
    RfidReader(uint8_t sdaPin = 26, uint8_t sclPin = 27);

    // Вызвать в setup()
    bool begin();

    // Неблокирующее чтение карты:
    //   true  -> есть карта, UID в uidHex (например "04AABBCCDD")
    //   false -> карты нет
    bool readCard(String& uidHex);

    // Очень простой список разрешённых карт
    bool isAuthorized(const String& uidHex) const;

private:
    uint8_t _sdaPin;
    uint8_t _sclPin;

    Adafruit_PN532 _nfc;

    static constexpr size_t MAX_AUTH_UIDS = 4;
    String _authUids[MAX_AUTH_UIDS];
    size_t _authCount = 0;

    void loadDefaultAuthUids();
};
