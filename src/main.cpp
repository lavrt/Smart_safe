// #include <Arduino.h>
// #include <EEPROM.h>

// #include "rfid_module.hpp"
// #include "config.hpp"

// void setup() {
//     Serial.begin(115200);
//     Serial.println("\n=== RFID Access Control System ===");
//     Serial.println("Commands: ADD, LIST, REMOVE <UID>, ADMIN, EXIT");

//     pinMode(RELAY_PIN, OUTPUT);
//     pinMode(BUZZER_PIN, OUTPUT);
//     pinMode(LED_GREEN, OUTPUT);
//     pinMode(LED_RED, OUTPUT);

//     digitalWrite(RELAY_PIN, LOW);
//     digitalWrite(BUZZER_PIN, LOW);
//     digitalWrite(LED_GREEN, LOW);
//     digitalWrite(LED_RED, LOW);

//     SPI.begin();
//     mfrc522.PCD_Init();
//     EEPROM.begin(EEPROM_SIZE);

//     byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
//     if (v == 0x00 || v == 0xFF) {
//         Serial.println("ERROR: MFRC522 not found! Check wiring.");
//         while (true) {
//         digitalWrite(LED_RED, HIGH);
//         delay(500);
//         digitalWrite(LED_RED, LOW);
//         delay(500);
//         }
//     }

//     Serial.print("MFRC522 Firmware Version: 0x");
//     Serial.println(v, HEX);

//     Serial.println("System Ready! Waiting for cards...");
// }

// void loop() {
//     checkSerialCommands();

//     switch (currentState) {
//         case WAITING_CARD: {
//             handleWaitingState();
//             break;
//         }

//         case PROCESSING: {
//             handleProcessingState();
//             break;
//         }

//         case ACCESS_GRANTED: {
//             handleAccessGranted();
//             break;
//         }

//         case ACCESS_DENIED: {
//             handleAccessDenied();
//             break;
//         }

//         case ADMIN_MODE: {
//             handleAdminMode();
//             break;
//         }
//     }

//     updateLEDs();
// }

// -------------------------------------------------------------------------------------------------
// #include <WiFi.h>
// #include "telegram_config.hpp"
// #include "telegram_bot_manager.hpp"
// #include "access_manager.hpp"

// TelegramBotManager telegramBot;
// AccessManager accessManager;

// void setup() {
//     Serial.begin(115200);
//     Serial.println();
//     Serial.println("🚀 Запуск Умного Сейфа...");

//     // Настройка пинов
//     pinMode(2, OUTPUT);
//     digitalWrite(2, LOW);
//     Serial.println("✅ Пины инициализированы");

//     // Подключение к Wi-Fi
//     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//     Serial.print("🔗 Подключаемся к WiFi");

//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500);
//         Serial.print(".");
//     }

//     Serial.println();
//     Serial.println("✅ Подключено к WiFi!");
//     Serial.print("📡 IP адрес: ");
//     Serial.println(WiFi.localIP());

//     // Инициализация менеджеров
//     Serial.println("🛡️ Инициализация AccessManager...");
//     accessManager.begin();

//     Serial.println("🤖 Инициализация Telegram бота...");
//     telegramBot.setAccessManager(&accessManager);
//     telegramBot.begin();

//     Serial.println("=================================");
//     Serial.println("🚀 СИСТЕМА ПОЛНОСТЬЮ ЗАПУЩЕНА!");
//     Serial.println("=================================");
// }

// void loop() {
//     telegramBot.update();
//     delay(50);
// }
// -------------------------------------------------------------------------------------------------

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

// --- Глобальные объекты ---

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

LockController lockController(LOCK_PIN, LOCK_OPEN_TIME);
DoorSensor doorSensor(DOOR_SENSOR_PIN, DOOR_OPEN_LEVEL);
Buzzer buzzer(BUZZER_PIN, BUZZER_ACTIVE_LEVEL);

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

// --- Wi-Fi ---

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

// --- Время + TLS ---

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

    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    Serial.println("TLS: установлен корневой сертификат Telegram.");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("[BOOT] Старт устройства умного замка");

    lockController.begin();
    doorSensor.begin();
    buzzer.begin();

    connectWiFi();
    setupTimeAndTLS();

    telegramLockBot.begin();

    if (NUM_ADMIN_CHATS > 0) {
        bot.sendMessage(ADMIN_CHAT_IDS[0],
                        "✅ Бот умного замка запущен.\n"
                        "Устройство онлайн и готово к работе.",
                        "");
    }
}

void loop() {
    lockController.update();
    buzzer.update();
    telegramLockBot.update();

    delay(50);
}
