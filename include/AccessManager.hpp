#pragma once
#include <Arduino.h>

enum class Role {
    Unknown = 0,
    Viewer,
    User,
    Admin
};

class AccessManager {
public:
    AccessManager(const char* const* adminIds,  size_t adminCount,
                  const char* const* userIds,   size_t userCount,
                  const char* const* viewerIds, size_t viewerCount);

    Role getRole(const String& chatId) const;

    bool canViewStatus(Role role) const {
        return role == Role::Viewer || role == Role::User || role == Role::Admin;
    }

    bool canOpen(Role role) const {
        return role == Role::User || role == Role::Admin;
    }

    bool canConfigure(Role role) const {
        return role == Role::Admin;
    }

private:
    const char* const* _adminIds;
    size_t _adminCount;
    const char* const* _userIds;
    size_t _userCount;
    const char* const* _viewerIds;
    size_t _viewerCount;
};
