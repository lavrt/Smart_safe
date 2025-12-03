#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include <UniversalTelegramBot.h>

class SafeCamera {
public:
    explicit SafeCamera(UniversalTelegramBot& bot);

    bool begin();

    bool sendPhoto(const String& chatId);

private:
    UniversalTelegramBot& _bot;
    bool _initialized;

    bool initCamera();
    bool sendJpegToTelegram(const String& chatId,
                            const uint8_t* jpgBuf,
                            size_t jpgLen);
};
