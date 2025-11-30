#include "AccessManager.hpp"

AccessManager::AccessManager(const char* const* adminIds,  size_t adminCount,
                             const char* const* userIds,   size_t userCount,
                             const char* const* viewerIds, size_t viewerCount)
    : _adminIds(adminIds),
      _adminCount(adminCount),
      _userIds(userIds),
      _userCount(userCount),
      _viewerIds(viewerIds),
      _viewerCount(viewerCount) {}

Role AccessManager::getRole(const String& chatId) const {
    // Сначала ищем в админах
    for (size_t i = 0; i < _adminCount; i++) {
        if (chatId == _adminIds[i]) {
            return Role::Admin;
        }
    }

    // Потом обычные пользователи
    for (size_t i = 0; i < _userCount; i++) {
        if (chatId == _userIds[i]) {
            return Role::User;
        }
    }

    // Потом только просмотр
    for (size_t i = 0; i < _viewerCount; i++) {
        if (chatId == _viewerIds[i]) {
            return Role::Viewer;
        }
    }

    return Role::Unknown;
}
