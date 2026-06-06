#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct NetPlayerProfile {
    uint32_t id = 0;           // Теперь всегда 0 по умолчанию
    std::string name = "Unknown";
    std::string password = "Unknown";
    uint32_t coins = 0;
    uint32_t wins = 0;
    uint32_t losses = 0;
    std::vector<uint32_t> unlocked; // Автоматически пустой
    std::vector<uint32_t> friends;  // Автоматически пустой

    
};

//struct sLoginResult
//{
//    bool success;
//    std::string reason;
//};