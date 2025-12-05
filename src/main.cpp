#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <time.h>

#include "Config.hpp"
#include "LockController.hpp"
#include "TelegramLockBot.hpp"
#include "AccessManager.hpp"
#include "DoorSensor.hpp"
#include "Buzzer.hpp"
#include "FingerprintAuth.hpp"
#include "SafeCamera.hpp"
#include "RfidReader.hpp"
#include "MotionSensor.hpp"

HardwareSerial FingerSerial(2);

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

SafeCamera safeCamera(bot);
LockController lockController(LOCK_PIN, LOCK_OPEN_TIME);
DoorSensor doorSensor(DOOR_SENSOR_PIN, DOOR_OPEN_LEVEL);
Buzzer buzzer(BUZZER_PIN, BUZZER_ACTIVE_LEVEL);
RfidReader rfid;
MotionSensor motionSensor;

AccessManager accessManager(
    ADMIN_CHAT_IDS,  NUM_ADMIN_CHATS,
    USER_CHAT_IDS,   NUM_USER_CHATS,
    VIEWER_CHAT_IDS, NUM_VIEWER_CHATS
);

TelegramLockBot telegramLockBot(
    bot,
    lockController,
    doorSensor,
    accessManager,
    buzzer,
    BOT_UPDATE_INTERVAL
);

FingerprintAuth fingerprintAuth(
    FingerSerial,
    33,
    32,
    lockController,
    buzzer,
    &bot,
    (NUM_ADMIN_CHATS > 0 ? ADMIN_CHAT_IDS[0] : nullptr)
);

void connectWiFi() {
    Serial.print("Подключение к Wi-Fi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint8_t tryCount = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (++tryCount == 60) {
            Serial.println("\nНе удалось подключиться к Wi-Fi, перезагрузка...");
            ESP.restart();
        }
    }

    Serial.println("\nWi-Fi подключен!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void setupTimeAndTLS() {
    Serial.println("Синхронизация времени через NTP...");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");

    time_t now = time(nullptr);
    int retries = 0;
    while (now < 24 * 3600 && retries < 30) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
        retries++;
    }
    Serial.println();

    if (now < 24 * 3600) {
        Serial.println("⚠️ Не удалось синхронизировать время, TLS может не работать.");
    } else {
        Serial.print("Текущее время (UTC): ");
        Serial.println(ctime(&now));
    }

    secured_client.setInsecure();
    Serial.println("TLS: установлен корневой сертификат Telegram.");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LOCK_PIN, OUTPUT);
    digitalWrite(LOCK_PIN, LOW);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, !BUZZER_ACTIVE_LEVEL);

    Serial.println();
    Serial.println("[BOOT] Старт устройства умного замка");

    lockController.begin();
    doorSensor.begin();
    buzzer.begin();
    fingerprintAuth.begin();

    connectWiFi();
    setupTimeAndTLS();

    telegramLockBot.begin();

    bool rfidOk = rfid.begin();
    if (!rfidOk) {
        Serial.println("[RFID] PN532 не инициалирован, RFID пока работать не будет.");
    }

    bool accOk = motionSensor.begin();
    if (!accOk) {
        Serial.println("[ACC] Акселерометр не инициализировался, датчик движения не работает.");
    }

    bool camOk = safeCamera.begin();
    if (!camOk) {
        Serial.println("[CAM] Камера не инициализировалась, /photo будет выдавать ошибку.");
    }

    if (NUM_ADMIN_CHATS > 0) {
        bot.sendMessage(ADMIN_CHAT_IDS[0],
                        "✅ Бот умного замка запущен.\n"
                        "Устройство онлайн и готово к работе.",
                        "");
    }
}

void loop() {
    if (motionSensor.loop(1.5f)) {
        telegramLockBot.notifyAdmins(
            "🚨 Обнаружено сильное движение/удар по сейфу!\n"
            "Проверьте, всё ли в порядке."
        );
    }
    lockController.update();
    buzzer.update();
    fingerprintAuth.update();
    telegramLockBot.update();

    String uid;
    if (rfid.readCard(uid)) {
        Serial.print("[RFID] UID карты: ");
        Serial.println(uid);

        if (rfid.isAuthorized(uid)) {
            Serial.println("[RFID] Авторизованная карта, открываем замок.");

            if (!lockController.isOpen()) {
                lockController.open();
                buzzer.beep(2);
            } else {
                Serial.println("[RFID] Замок уже открыт.");
            }
        } else {
            Serial.println("[RFID] НЕавторизованная карта.");
            // Можно тут сделать короткий один писк как “ошибка”
            // buzzer.beep(1);
        }
    }

    delay(5);
}
