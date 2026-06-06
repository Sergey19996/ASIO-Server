#pragma once
#include <vector>
#include <string>
#include "GameplayTypes.h"

struct PlayerProfile
{
    std::string name;
    uint32_t coins;
    uint32_t wins;
    uint32_t losses;
    std::vector<uint32_t> unlocked;
    std::vector<uint32_t> friends;

    bool isDirty = false; // например: UI изменил имя
   // PlayerClass selectedclass;

   
};
