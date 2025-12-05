#pragma once

#include <Arduino.h>
/
#define PN532_I2C
#include <Adafruit_PN532.h>

class RfidReader {
public:
    RfidReader(uint8_t sdaPin = 26, uint8_t sclPin = 27);

    bool begin();

    bool readCard(String& uidHex);

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
