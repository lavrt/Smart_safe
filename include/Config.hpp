#pragma once
#include <Arduino.h>

// Wi-Fi
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// Telegram
extern const char* BOT_TOKEN;

// Параметры замка
extern const int LOCK_PIN;
extern const unsigned long LOCK_OPEN_TIME;

// Параметры бота
extern const unsigned long BOT_UPDATE_INTERVAL;

// Роли пользователей

// Админы — полный доступ
extern const char* ADMIN_CHAT_IDS[];
extern const size_t NUM_ADMIN_CHATS;

// Обычные пользователи — открывать/смотреть статус
extern const char* USER_CHAT_IDS[];
extern const size_t NUM_USER_CHATS;

// Только просмотр статуса
extern const char* VIEWER_CHAT_IDS[];
extern const size_t NUM_VIEWER_CHATS;

// ==== ДАТЧИК ДВЕРИ ====
extern const int DOOR_SENSOR_PIN;
extern const bool DOOR_OPEN_LEVEL;

// ==== ЗУММЕР ====
extern const int BUZZER_PIN;
extern const bool BUZZER_ACTIVE_LEVEL;
