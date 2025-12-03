#pragma once
#include <Arduino.h>

extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

extern const char* BOT_TOKEN;

extern const int LOCK_PIN;
extern const unsigned long LOCK_OPEN_TIME;

extern const unsigned long BOT_UPDATE_INTERVAL;

extern const char* ADMIN_CHAT_IDS[];
extern const size_t NUM_ADMIN_CHATS;

extern const char* USER_CHAT_IDS[];
extern const size_t NUM_USER_CHATS;

extern const char* VIEWER_CHAT_IDS[];
extern const size_t NUM_VIEWER_CHATS;

extern const int DOOR_SENSOR_PIN;
extern const bool DOOR_OPEN_LEVEL;

extern const int BUZZER_PIN;
extern const bool BUZZER_ACTIVE_LEVEL;
