#pragma once

#include <string>

struct sLoginRequest
{
    char password[32];
    char firstName[32];
    uint64_t sessionToken; // 0 при первом входе, реальное число при реконнекте
};

struct sLoginResult
{
    bool success = false;
    char reason[64] = { 0 }; // Фиксированный размер вместо std::string
    uint64_t assignedToken;
    uint32_t assignedID; // Твой стабильный pid
    char errorMessage[64] = { 0 }; // Например: "Wrong password" или "Name taken"
};

