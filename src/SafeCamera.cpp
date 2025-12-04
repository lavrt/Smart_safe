#include "SafeCamera.hpp"
#include "CameraPins.hpp"
#include "Config.hpp"
#include "img_converters.h"

static WiFiClientSecure camClient;

SafeCamera::SafeCamera(UniversalTelegramBot& bot)
    : _bot(bot),
      _initialized(false)
{}

bool SafeCamera::initCamera() {
    camera_config_t config;
    memset(&config, 0, sizeof(config));

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;

    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;

    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;

    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_YUV422;   // 👈 вместо RGB565

    if (psramFound()) {
        Serial.println("[CAM] PSRAM найден.");
        config.frame_size   = FRAMESIZE_VGA;   // 640x480
        config.jpeg_quality = 12;             // для frame2jpg всё равно, но пусть будет
        config.fb_count     = 2;
        config.fb_location  = CAMERA_FB_IN_PSRAM;
    } else {
        Serial.println("[CAM] PSRAM НЕ найден.");
        config.frame_size   = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count     = 1;
        config.fb_location  = CAMERA_FB_IN_DRAM;
    }

    config.grab_mode = CAMERA_GRAB_LATEST;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("[CAM] Ошибка esp_camera_init: 0x%x\n", err);
        _initialized = false;
        return false;
    }

    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        // гарантия размера кадра
        s->set_framesize(s, config.frame_size);
        // дальше можно чуть подкрутить картинку:
        s->set_brightness(s, 0);
        s->set_contrast(s, 1);
        s->set_saturation(s, 0);
        s->set_gainceiling(s, GAINCEILING_8X);
        s->set_exposure_ctrl(s, 1);
        s->set_aec2(s, 1);
        s->set_ae_level(s, 0);
        s->set_whitebal(s, 1);
        s->set_awb_gain(s, 1);
        s->set_wb_mode(s, 0);
        s->set_lenc(s, 1);
        // s->set_vflip(s, 1);  // если нужно
        // s->set_hmirror(s, 1);
    }

    _initialized = true;
    Serial.println("[CAM] Камера инициализирована (PIXFORMAT_YUV422).");
    return true;
}


bool SafeCamera::begin() {
    esp_camera_deinit();
    _initialized = false;
    return initCamera();
}

bool SafeCamera::sendPhoto(const String& chatId) {
    if (!_initialized) {
        Serial.println("[CAM] sendPhoto: камера не инициализирована, пробуем initCamera().");
        if (!initCamera()) {
            _bot.sendMessage(chatId,
                             "❌ Камера не инициализирована.",
                             "");
            return false;
        }
    }

    Serial.println("[CAM] Снимаю фото (YUV422)...");

    // промываем 1–2 кадра
    for (int i = 0; i < 2; ++i) {
        camera_fb_t* flushFb = esp_camera_fb_get();
        if (!flushFb) break;
        esp_camera_fb_return(flushFb);
    }

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("[CAM] fb == nullptr, пробуем переинициализировать.");

        esp_camera_deinit();
        _initialized = false;

        if (!initCamera()) {
            _bot.sendMessage(chatId,
                             "❌ Камера не смогла получить кадр (ошибка инициализации).",
                             "");
            return false;
        }

        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("[CAM] fb == nullptr повторно.");
            _bot.sendMessage(chatId,
                             "❌ Камера не смогла получить кадр (fb == nullptr).",
                             "");
            return false;
        }
    }

    Serial.printf("[CAM] Кадр: %dx%d, %u байт, формат=%d\n",
                  fb->width, fb->height, fb->len, fb->format);

    // ждём YUV422 или хотя бы НЕ JPEG (JPEG мы здесь не ждём)
    if (fb->format == PIXFORMAT_JPEG) {
        // теоретически может оказаться JPEG — тогда можно сразу слать,
        // но в твоём случае, скорее всего, будет YUV422
        Serial.println("[CAM] Неожиданный JPEG — можно слать напрямую.");
    }

    uint8_t* jpgBuf = nullptr;
    size_t jpgLen   = 0;

    bool convOk = frame2jpg(
        fb,
        90,          // качество
        &jpgBuf,
        &jpgLen
    );

    esp_camera_fb_return(fb);

    if (!convOk || !jpgBuf || jpgLen == 0) {
        Serial.println("[CAM] frame2jpg: не удалось сконвертировать в JPEG.");
        _bot.sendMessage(chatId,
                         "❌ Не удалось преобразовать кадр в JPEG.",
                         "");
        if (jpgBuf) {
            free(jpgBuf);
        }
        return false;
    }

    Serial.printf("[CAM] JPEG готов: %u байт\n", (unsigned)jpgLen);

    bool ok = sendJpegToTelegram(chatId, jpgBuf, jpgLen);

    free(jpgBuf);

    if (!ok) {
        _bot.sendMessage(chatId,
                         "❌ Не удалось отправить фото в Telegram.",
                         "");
    }

    return ok;
}

bool SafeCamera::sendJpegToTelegram(const String& chatId,
                                    const uint8_t* jpgBuf,
                                    size_t jpgLen)
{
    const char* host = "api.telegram.org";
    const uint16_t port = 443;

    String path = "/bot";
    path += BOT_TOKEN;
    path += "/sendPhoto";

    String boundary = "----ESP32CamSafeBoundary";

    String bodyStart;
    bodyStart  = "--" + boundary + "\r\n";
    bodyStart += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
    bodyStart += chatId;
    bodyStart += "\r\n";
    bodyStart += "--" + boundary + "\r\n";
    bodyStart += "Content-Disposition: form-data; name=\"photo\"; filename=\"photo.jpg\"\r\n";
    bodyStart += "Content-Type: image/jpeg\r\n\r\n";

    String bodyEnd;
    bodyEnd  = "\r\n--" + boundary + "--\r\n";

    size_t contentLength = bodyStart.length() + jpgLen + bodyEnd.length();

    Serial.printf("[CAM] Отправка JPEG в Telegram (%u байт, Content-Length=%u)...\n",
                  (unsigned)jpgLen, (unsigned)contentLength);

    camClient.stop();
    camClient.setTimeout(15000);

    camClient.setInsecure();

    if (!camClient.connect(host, port)) {
        Serial.println("[CAM] Не удалось подключиться к api.telegram.org:443");
        return false;
    }

    camClient.print("POST ");
    camClient.print(path);
    camClient.println(" HTTP/1.1");
    camClient.print("Host: ");
    camClient.println(host);
    camClient.println("User-Agent: ESP32SafeCam/1.0");
    camClient.print("Content-Type: multipart/form-data; boundary=");
    camClient.println(boundary);
    camClient.print("Content-Length: ");
    camClient.println(contentLength);
    camClient.println("Connection: close");
    camClient.println();

    camClient.print(bodyStart);

    const size_t chunkSize = 1024;
    size_t remaining = jpgLen;
    size_t offset = 0;

    while (remaining > 0) {
        size_t toSend = (remaining > chunkSize) ? chunkSize : remaining;
        size_t written = camClient.write(jpgBuf + offset, toSend);
        if (written == 0) {
            Serial.println("[CAM] Ошибка: camClient.write() вернул 0.");
            camClient.stop();
            return false;
        }
        offset    += written;
        remaining -= written;
    }

    camClient.print(bodyEnd);

    unsigned long start = millis();
    while (camClient.connected() && !camClient.available() &&
           (millis() - start < 15000)) {
        delay(10);
    }

    String response;
    while (camClient.available()) {
        char c = camClient.read();
        response += c;
    }

    camClient.stop();

    Serial.println("[CAM] Ответ Telegram:");
    Serial.println(response);

    bool httpOk = response.indexOf(" 200 OK") >= 0;
    bool jsonOk = response.indexOf("\"ok\":true") >= 0;

    if (!httpOk || !jsonOk) {
        Serial.println("[CAM] Ответ Telegram не содержит 200 OK или \"ok\":true");
        return false;
    }

    Serial.println("[CAM] Фото успешно принято Telegram.");
    return true;
}
